#include "DBConnection.h"

static volatile long nConnFailCount = 0;

CDBConnection::CDBConnection(DBConnectionParam* pParam)
{
	assert(pParam != NULL);
	m_connParam = *pParam;
	m_connParam.m_strDBAddress = pParam->m_strDBAddress;
	m_connParam.m_strDBSchema = pParam->m_strDBSchema;
	m_connParam.m_strDBUserName = pParam->m_strDBUserName;
	m_connParam.m_strDBUserPwd = pParam->m_strDBUserPwd;
}

CDBConnection::CDBConnection(const char* szAddr, const char* szSchema, const char* szUserName, const char* szPwd)
{
	m_connParam.m_strDBAddress = szAddr; //数据库服务器地址
	m_connParam.m_strDBSchema = szSchema; //数据库名
	m_connParam.m_strDBUserName = szUserName; //用户名
	m_connParam.m_strDBUserPwd = szPwd; //用户密码
}

CDBConnection::~CDBConnection(void)
{
}

BOOL CDBConnection::IsValid()
{
	if( CADODatabase::IsOpen() )
		return TRUE;
	char szConnStr[1024] = {0};
	_snprintf(szConnStr, 1023, "Provider=SQLOleDB;Data Source=%s;Initial Catalog=%s;User Id=%s;Password=%s",
		m_connParam.m_strDBAddress.c_str(),
		m_connParam.m_strDBSchema.c_str(), 
		m_connParam.m_strDBUserName.c_str(), 
		m_connParam.m_strDBUserPwd.c_str());

	m_bConnected = CADODatabase::Open(szConnStr);

	return m_bConnected;
}

CAdoStoreProc::CAdoStoreProc(CADODatabase* pDB, const char* szProc) :
m_adoCmd(pDB, szProc),
m_paramRet(CADORecordset::typeInteger, sizeof(int),CADOParameter::paramReturnValue)
{
	m_adoCmd.AddParameter(&m_paramRet);			
}

CAdoStoreProc::~CAdoStoreProc()
{
}

int CAdoStoreProc::GetReturnValue()
{
	int nRetVal = 0;
	if( !m_paramRet.GetValue(nRetVal) )
		return -1;
	return nRetVal;
}

bool CAdoStoreProc::PutInt8(BYTE value)
{
	variant_t vtVal;
	vtVal.vt = VT_UI1;
	vtVal.bVal = value;
	CADOParameter param(CADORecordset::typeTinyInt, sizeof(BYTE)); 
	if( !param.SetValue(vtVal) || !m_adoCmd.AddParameter(&param) ) 
	{ 
		char szErrorMsg[256] = { 0 };
		m_adoCmd.GetLastErrorString(szErrorMsg, 256);
		szErrorMsg[255] = '\0';
		m_strLastError = szErrorMsg;
		return false;  
	}
	return true;
}

bool CAdoStoreProc::PutInt16(short value)
{
	variant_t vtVal;
	vtVal.vt = VT_I2;
	vtVal.iVal = value;
	CADOParameter param(CADORecordset::typeSmallInt, sizeof(short)); 
	if( !param.SetValue(vtVal) || !m_adoCmd.AddParameter(&param) ) 
	{ 
		char szErrorMsg[256] = { 0 };
		m_adoCmd.GetLastErrorString(szErrorMsg, 256);
		szErrorMsg[255] = '\0';
		m_strLastError = szErrorMsg;
		return false;  
	}
	return true;
}

bool CAdoStoreProc::PutInt32(int value)
{
	variant_t vtVal;
	vtVal.vt = VT_I4;
	vtVal.lVal = value;
	CADOParameter param(CADORecordset::typeInteger, sizeof(int)); 
	if( !param.SetValue(vtVal) || !m_adoCmd.AddParameter(&param) ) 
	{ 
		char szErrorMsg[256] = { 0 };
		m_adoCmd.GetLastErrorString(szErrorMsg, 256);
		szErrorMsg[255] = '\0';
		m_strLastError = szErrorMsg;
		return false;  
	}
	return true;
}

bool CAdoStoreProc::PutInt64(__int64 value)
{
	variant_t vtVal;
	vtVal.vt = VT_I8;
	vtVal.llVal = value;
	CADOParameter param(CADORecordset::typeBigInt, sizeof(__int64)); 
	if( !param.SetValue(vtVal) || !m_adoCmd.AddParameter(&param) ) 
	{ 
		char szErrorMsg[256] = { 0 };
		m_adoCmd.GetLastErrorString(szErrorMsg, 256);
		szErrorMsg[255] = '\0';
		m_strLastError = szErrorMsg;
		return false;  
	}
	return true;
}

bool CAdoStoreProc::PutVarchar(const char* value)
{
	_bstr_t bst(value);
	variant_t vtVal(bst);

	CADOParameter param(CADORecordset::typeVarChar, bst.length() *sizeof(wchar_t) ); 
	if( !param.SetValue(vtVal) || !m_adoCmd.AddParameter(&param) ) 
	{ 
		char szErrorMsg[256] = { 0 };
		m_adoCmd.GetLastErrorString(szErrorMsg, 256);
		szErrorMsg[255] = '\0';
		m_strLastError = szErrorMsg;
		return false;  
	}
	return true;
}

bool CAdoStoreProc::PutNVarchar(const wchar_t* value)
{
	_bstr_t bst(value);
	variant_t vtVal(bst);
	
	CADOParameter param(CADORecordset::typeVarWChar, bst.length() ); 
	if( !param.SetValue(vtVal) || !m_adoCmd.AddParameter(&param) ) 
	{ 
		char szErrorMsg[256] = { 0 };
		m_adoCmd.GetLastErrorString(szErrorMsg, 256);
		szErrorMsg[255] = '\0';
		m_strLastError = szErrorMsg;
		return false;  
	}
	return true;
}

bool CAdoStoreProc::PutVarbinary(char* pData, size_t len)
{
	CADOParameter param(CADORecordset::typeVarBinary, len);
	_variant_t vt = MakeBinary(pData, len);
	if( !param.SetValue(vt) || !m_adoCmd.AddParameter(&param) ) 
	{ 
		char szErrorMsg[256] = { 0 };
		m_adoCmd.GetLastErrorString(szErrorMsg, 256);
		szErrorMsg[255] = '\0';
		m_strLastError = szErrorMsg;
		return false; 
	}
	return true;
}

bool CAdoStoreProc::MakeOutput(CADOParameter& paramOut)
{
	if(  !m_adoCmd.AddParameter(&paramOut) ) 
	{ 
		char szErrorMsg[256] = { 0 };
		m_adoCmd.GetLastErrorString(szErrorMsg, 256);
		szErrorMsg[255] = '\0';
		m_strLastError = szErrorMsg;
		return false; 
	}

	return true;
}

bool CAdoStoreProc::Execute()
{
	if( !m_adoCmd.Execute() )
	{
		char szErrorMsg[256] = { 0 };
		m_adoCmd.GetLastErrorString(szErrorMsg, 256);
		szErrorMsg[255] = '\0';
		m_strLastError = szErrorMsg;

		return false; 
	}

	return true;
}