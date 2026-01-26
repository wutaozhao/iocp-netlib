#pragma once
#include <windows.h>
#include <list>
#include <map>
#include <string>
#include <vector>
#include <assert.h>

//==============================================================================
//统计信息支持类库
//usage
//CSnmpStatistic s
//s[
//	def("a1"),
//  def("a2"),
//  def("a3")
//]
// s["a1"]++;
// s.Snapshot()
// s.dump();
//==============================================================================

class CSnmpStatistic;
class SnmpItem
{
public:
	SnmpItem():m_value(0) {}
	SnmpItem(const SnmpItem& right);
	DWORD operator++(int);
	DWORD operator--(int);
	void operator=(int value);
	DWORD GetValue() ;
	void Reset();
private:
	volatile DWORD m_value;
};

class CNameList
{
public:
	typedef std::list<std::string> name_list_type;
	CNameList(const char* szName, unsigned char times);
	CNameList(const CNameList& right);
	CNameList& operator+(const CNameList& right);
	const name_list_type& GetNameList() const { return m_lsNames; }
private:
	name_list_type m_lsNames;
};

inline CNameList def(const char* szName, unsigned char times = 0)
{
	return CNameList(szName, times);
}


class CSnmpStatistic
{
public:
	typedef std::map<std::string, SnmpItem> statistic_type;
	typedef std::list<std::string> name_list_type;
	CSnmpStatistic();
	~CSnmpStatistic();
	CSnmpStatistic& operator[] (const CNameList& nameList);
	SnmpItem& operator[] (const char* szName);
	void Reset();
	const char* Snapshot(); //获取当前统计的快照
	size_t GetSize() const;
	//dump values in snapshot buffer
	void dump(std::string& strResult); //打印快照内容
private:
	statistic_type m_mapStatistics;
	name_list_type m_names;
	char* m_pStatisticBuf;
	size_t m_nBufSize;
};