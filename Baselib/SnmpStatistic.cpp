#include "snmpstatistic.h"


SnmpItem::SnmpItem(const SnmpItem& right)
{
	this->m_value = right.m_value;
}
DWORD SnmpItem::operator++(int)
{
	return ::InterlockedIncrement((LONG*)&m_value);
}

DWORD SnmpItem::operator--(int)
{
	return ::InterlockedDecrement((LONG*)&m_value);
}

void SnmpItem::operator=(int value)
{
	::InterlockedExchange((LONG*)&m_value, value);
}

DWORD SnmpItem::GetValue() 
{
	return m_value;
}
void SnmpItem::Reset() 
{
	::InterlockedExchange((LONG*)&m_value, 0);
}



CNameList::CNameList(const char* szName,unsigned char times)
{
	if( times == 0 )
		m_lsNames.push_back(szName);
	else
	{
		for(unsigned char i = 0; i < times; ++i)
		{
			char szNameTmp[256] = { 0 };
			sprintf_s(szNameTmp, 255, "%s%d", szName, i);
			m_lsNames.push_back(szNameTmp);
		}
	}
}
CNameList::CNameList(const CNameList& right)
{
	m_lsNames.insert( m_lsNames.end(), 
		right.m_lsNames.begin(),
		right.m_lsNames.end());
}

CNameList& CNameList::operator+ (const CNameList& right)
{
	m_lsNames.insert( m_lsNames.end(), 
		right.m_lsNames.begin(),
		right.m_lsNames.end());

	for (CNameList::name_list_type::const_iterator it = m_lsNames.begin();
		it != m_lsNames.end(); ++it) {
	}

	return *this;
}




CSnmpStatistic::CSnmpStatistic()
{
	m_pStatisticBuf = NULL;
	m_nBufSize = 0;
}
CSnmpStatistic::~CSnmpStatistic()
{
	delete[] m_pStatisticBuf;
}

CSnmpStatistic& CSnmpStatistic::operator[] (const CNameList& nameList)
{
	//create new CSnmpStatistic
	//realloc memory buffer
	m_names.clear();
	m_mapStatistics.clear();
	delete[] m_pStatisticBuf; //it's ok to free 0 pointer
	m_pStatisticBuf = NULL;
	m_nBufSize = 0;
	size_t count = nameList.GetNameList().size();
	if( count != 0 )
	{
		m_pStatisticBuf = new char[count*sizeof(DWORD)];
		if( m_pStatisticBuf != NULL )
		{
			m_names.assign(nameList.GetNameList().begin(),  nameList.GetNameList().end());
			m_nBufSize = count*sizeof(DWORD);
			int index = 0;
			DWORD* pStat = (DWORD*)m_pStatisticBuf;
			for(CNameList::name_list_type::const_iterator it =  nameList.GetNameList().begin();
				it !=  nameList.GetNameList().end(); ++it, ++index)
			{
				m_mapStatistics[*it] = SnmpItem();
				pStat[index] = 0;
			}
		}
	}
	return *this;
}

SnmpItem& CSnmpStatistic::operator[] (const char* szName)
{
	statistic_type::iterator it = m_mapStatistics.find(szName);
	assert( it != m_mapStatistics.end() && "make sure you've been defined this key");
	//warn the final user.
	return it->second;
}

void CSnmpStatistic::Reset()
{
	int index = 0;
	DWORD* pStat = (DWORD*)m_pStatisticBuf;
	for(name_list_type::iterator it = m_names.begin();
		it != m_names.end(); ++it, ++index)
	{
		m_mapStatistics[*it].Reset();
		pStat[index] = 0;
	}
}

const char* CSnmpStatistic::Snapshot()
{
	int index = 0;
	DWORD* pBuffer = (DWORD*)m_pStatisticBuf;
	for(name_list_type::iterator it = m_names.begin();
		it != m_names.end(); ++it, ++index)
	{
		pBuffer[index] = m_mapStatistics[*it].GetValue();
	}
	return m_pStatisticBuf;
}

size_t CSnmpStatistic::GetSize() const
{
	return m_nBufSize;
}

void CSnmpStatistic::dump(std::string & strResult)
{
	strResult = "";
	int index = 0;
	DWORD* pStat = (DWORD*)m_pStatisticBuf;
	for(name_list_type::iterator it = m_names.begin();
		it != m_names.end(); ++it, ++index)
	{
		char szBuf[256] = { 0 };
		sprintf_s(szBuf, 255, "%s : %u\n", it->c_str(), pStat[index]);
		strResult += szBuf;
	}
}


