#pragma once
#pragma warning(disable:4251)
#pragma warning(disable:4018)
#include "BaseInclude.h"
#include <string>
using namespace std;
using namespace wt;

struct DBConnectionParam
{
	string m_strDBAddress;    //数据库服务器地址
	string m_strDBSchema;     //数据库名
	string m_strDBUserName;   //用户名
	string m_strDBUserPwd;    //用户密码
};

/*
	数据库连接类,主要是在CADODatabase的基础上加强重连机制
*/
class CDBConnection :
	public CADODatabase
{
public:
	CDBConnection(DBConnectionParam* pParam);
	CDBConnection(const char* szAddr, const char* szSchema, const char* szUserName, const char* szPwd);
	virtual ~CDBConnection(void);

	BOOL IsValid(); 
protected:
	DBConnectionParam m_connParam;

private:
	BOOL       m_bConnected;
};


class CAdoStoreProc
{
public:
	CAdoStoreProc(CADODatabase* pDB, const char* szProc);
	~CAdoStoreProc();

	string GetLastErrorString() { return m_strLastError; }
	int GetReturnValue();
	bool PutInt8(BYTE value);
	bool PutInt16(short value);
	bool PutInt32(int value);
	bool PutInt64(__int64 value);
	bool PutVarchar(const char* value);
	bool PutNVarchar(const wchar_t* value);
	bool PutVarbinary(char* pData, size_t len);

	bool MakeOutput(CADOParameter& param);
	bool Execute();
	CADOCommand* operator&() { return &m_adoCmd; }
private:
	CADOCommand m_adoCmd;
	string m_strLastError;
	CADOParameter m_paramRet;
};


class CAdoStoreProc2
{
public:
	CAdoStoreProc2(CADODatabase* pDB, const char* szProc):m_adoCmd(pDB, szProc),
		m_paramRet(CADORecordset::typeInteger, sizeof(int),CADOParameter::paramReturnValue)
	{
		
	}
	~CAdoStoreProc2()
	{

	}

private:
	CADOCommand m_adoCmd;
	string m_strLastError;
	CADOParameter m_paramRet;
};