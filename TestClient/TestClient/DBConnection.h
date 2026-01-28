#pragma once
#pragma warning(disable:4251)
#pragma warning(disable:4018)

#include "BaseInclude.h"

struct DBConnectionParam
{
	std::string m_strDBAddress; //数据库服务器地址
	std::string m_strDBSchema; //数据库名
	std::string m_strDBUserName; //用户名
	std::string m_strDBUserPwd; //用户密码

	DBConnectionParam()
	{
	}
};

/*
	数据库连接类,主要是在CADODatabase的基础上将强重连机制
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

template<class VARINT_T>
class MakeVarBinary : public VARINT_T
{
public:
	MakeVarBinary(char* pData, int nDataLen)
	{
		this->vt = VT_NULL;
		if( pData != NULL && nDataLen  > 0 )
		{
			do
			{
				long lngOffset = 0;
				UCHAR chData;
				SAFEARRAY FAR *psa = NULL;
				SAFEARRAYBOUND rgsabound[1];
				//Create a safe array to store the array of BYTES 
				rgsabound[0].lLbound = 0;
				rgsabound[0].cElements = nDataLen;
				psa = SafeArrayCreate(VT_UI1,1,rgsabound);
				if( psa == NULL )
					break;

				while(lngOffset < (long)nDataLen)
				{
					chData	= ((UCHAR*)pData)[lngOffset];
					SafeArrayPutElement(psa, &lngOffset, &chData);
					lngOffset++;
				}

				//Assign the Safe array  to a variant. 
				this->vt = VT_ARRAY|VT_UI1;
				this->parray = psa;
			}while(false);

		}
	}
};

class CAdoStoreProc
{
public:
	CAdoStoreProc(CADODatabase* pDB, const char* szProc);
	~CAdoStoreProc();

	std::string GetLastErrorString() { return m_strLastError; }
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
	std::string m_strLastError;
	CADOParameter m_paramRet;
};
