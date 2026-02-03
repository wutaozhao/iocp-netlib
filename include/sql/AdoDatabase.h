
#pragma once

#include "Config.h"
#include "AdoTypeConvert.h"

#pragma warning (disable: 4146)

#ifdef X64
#import "C:\Program Files\Common Files\System\ADO\msado15.dll" rename("EOF", "EndOfFile")
#else
#import "C:\Program Files (x86)\Common Files\System\ADO\msado15.dll" rename("EOF", "EndOfFile")
#endif

using namespace ADODB;
using namespace std;

#pragma warning (default: 4146)

WT_BEGIN

class CADOCommand;

struct CADOFieldInfo
{
	char m_strName[30]; 
	short m_nType;
	long m_lSize; 
	long m_lDefinedSize;
	long m_lAttributes;
	short m_nOrdinalPosition;
	bool m_bRequired;   
	bool m_bAllowZeroLength; 
	long m_lCollatingOrder;  
};

string IntToStr(int nVal);

string LongToStr(long lVal);

string ULongToStr(unsigned long ulVal);

string DblToStr(double dblVal, int ndigits = 20);

string DblToStr(float fltVal);

class CADODatabase
{
public:
	enum cadoConnectModeEnum
    {	
		connectModeUnknown = adModeUnknown,
		connectModeRead = adModeRead,
		connectModeWrite = adModeWrite,
		connectModeReadWrite = adModeReadWrite,
		connectModeShareDenyRead = adModeShareDenyRead,
		connectModeShareDenyWrite = adModeShareDenyWrite,
		connectModeShareExclusive = adModeShareExclusive,
		connectModeShareDenyNone = adModeShareDenyNone
    };

	CADODatabase();
	
	virtual ~CADODatabase();
	
	bool Open(LPCTSTR lpstrConnection = "", LPCTSTR lpstrUserID = "", LPCTSTR lpstrPassword = "");
	_ConnectionPtr GetActiveConnection() 
	{
		return m_pConnection;
	}
	bool Execute(LPCTSTR lpstrExec);
	int GetRecordsAffected()
	{
		return m_nRecordsAffected;
	}
	DWORD GetRecordCount(_RecordsetPtr m_pRs);
	long BeginTransaction() 
	{
		return m_pConnection->BeginTrans();
	}
	long CommitTransaction() 
	{
		return m_pConnection->CommitTrans();
	}
	long RollbackTransaction() 
	{
		return m_pConnection->RollbackTrans();
	}
	bool IsOpen();
	void Close();
	void SetConnectionMode(cadoConnectModeEnum nMode)
	{
		m_pConnection->PutMode((enum ConnectModeEnum)nMode);
	}
	void SetConnectionString(LPCTSTR lpstrConnection)
	{
		m_strConnection = lpstrConnection;
	}
	string GetConnectionString()
	{
		return m_strConnection;
	}
	string GetLastErrorString() 
	{
		return m_strLastError;
	}
	DWORD GetLastError()
	{
		return m_dwLastError;
	}
	string GetErrorDescription() 
	{
		return m_strErrorDescription;
	}
	void SetConnectionTimeout(long nConnectionTimeout = 30)
	{
		m_nConnectionTimeout = nConnectionTimeout;
	}

protected:
	void dump_com_error(_com_error &e);

public:
	_ConnectionPtr m_pConnection;
	
protected:
	string m_strConnection;
	string m_strLastError;
	string m_strErrorDescription;
	DWORD m_dwLastError;
	int m_nRecordsAffected;
	long m_nConnectionTimeout;
};

class CADORecordset
{
public:
	bool Clone(CADORecordset& pRs);
	
	enum cadoOpenEnum
	{
		openUnknown = 0,
		openQuery = 1,
		openTable = 2,
		openStoredProc = 3
	};

	enum cadoEditEnum
	{
		dbEditNone = 0,
		dbEditNew = 1,
		dbEdit = 2
	};
	
	enum cadoPositionEnum
	{
	
		positionUnknown = -1,
		positionBOF = -2,
		positionEOF = -3
	};
	
	enum cadoSearchEnum
	{	
		searchForward = 1,
		searchBackward = -1
	};

	enum cadoDataType
	{
		typeEmpty = ADODB::adEmpty,
		typeTinyInt = ADODB::adTinyInt,
		typeSmallInt = ADODB::adSmallInt,
		typeInteger = ADODB::adInteger,
		typeBigInt = ADODB::adBigInt,
		typeUnsignedTinyInt = ADODB::adUnsignedTinyInt,
		typeUnsignedSmallInt = ADODB::adUnsignedSmallInt,
		typeUnsignedInt = ADODB::adUnsignedInt,
		typeUnsignedBigInt = ADODB::adUnsignedBigInt,
		typeSingle = ADODB::adSingle,
		typeDouble = ADODB::adDouble,
		typeCurrency = ADODB::adCurrency,
		typeDecimal = ADODB::adDecimal,
		typeNumeric = ADODB::adNumeric,
		typeBoolean = ADODB::adBoolean,
		typeError = ADODB::adError,
		typeUserDefined = ADODB::adUserDefined,
		typeVariant = ADODB::adVariant,
		typeIDispatch = ADODB::adIDispatch,
		typeIUnknown = ADODB::adIUnknown,
		typeGUID = ADODB::adGUID,
		typeDate = ADODB::adDate,
		typeDBDate = ADODB::adDBDate,
		typeDBTime = ADODB::adDBTime,
		typeDBTimeStamp = ADODB::adDBTimeStamp,
		typeBSTR = ADODB::adBSTR,
		typeChar = ADODB::adChar,
		typeVarChar = ADODB::adVarChar,
		typeLongVarChar = ADODB::adLongVarChar,
		typeWChar = ADODB::adWChar,
		typeVarWChar = ADODB::adVarWChar,
		typeLongVarWChar = ADODB::adLongVarWChar,
		typeBinary = ADODB::adBinary,
		typeVarBinary = ADODB::adVarBinary,
		typeLongVarBinary = ADODB::adLongVarBinary,
		typeChapter = ADODB::adChapter,
		typeFileTime = ADODB::adFileTime,
		typePropVariant = ADODB::adPropVariant,
		typeVarNumeric = ADODB::adVarNumeric,
		typeArray = ADODB::adVariant
	};
	
	enum cadoSchemaType 
	{
		schemaSpecific = adSchemaProviderSpecific,	
		schemaAsserts = adSchemaAsserts,
		schemaCatalog = adSchemaCatalogs,
		schemaCharacterSet = adSchemaCharacterSets,
		schemaCollections = adSchemaCollations,
		schemaColumns = adSchemaColumns,
		schemaConstraints = adSchemaCheckConstraints,
		schemaConstraintColumnUsage = adSchemaConstraintColumnUsage,
		schemaConstraintTableUsage  = adSchemaConstraintTableUsage,
		shemaKeyColumnUsage = adSchemaKeyColumnUsage,
		schemaTableConstraints = adSchemaTableConstraints,
		schemaColumnsDomainUsage = adSchemaColumnsDomainUsage,
		schemaIndexes = adSchemaIndexes,
		schemaColumnPrivileges = adSchemaColumnPrivileges,
		schemaTablePrivileges = adSchemaTablePrivileges,
		schemaUsagePrivileges = adSchemaUsagePrivileges,
		schemaProcedures = adSchemaProcedures,
		schemaTables = adSchemaTables,
		schemaProviderTypes = adSchemaProviderTypes,
		schemaViews = adSchemaViews,
		schemaViewTableUsage = adSchemaViewTableUsage,
		schemaProcedureParameters = adSchemaProcedureParameters,
		schemaForeignKeys = adSchemaForeignKeys,
		schemaPrimaryKeys = adSchemaPrimaryKeys,
		schemaProcedureColumns = adSchemaProcedureColumns,
		schemaDBInfoKeywords = adSchemaDBInfoKeywords,
		schemaDBInfoLiterals = adSchemaDBInfoLiterals,
		schemaCubes = adSchemaCubes,
		schemaDimensions = adSchemaDimensions,
		schemaHierarchies  = adSchemaHierarchies, 
		schemaLevels = adSchemaLevels,
		schemaMeasures = adSchemaMeasures,
		schemaProperties = adSchemaProperties,
		schemaMembers = adSchemaMembers,
	}; 

	template<typename T> bool SetFieldValue(LPCTSTR lpFieldName, T& tValue);

	template<typename T> bool SetFieldValue(int nIndex, T& tValue);

	bool SetFieldValue(int nIndex, COleDateTime time);

	bool SetFieldValue(LPCTSTR lpFieldName, COleDateTime time);

	bool SetFieldValue(int nIndex, _variant_t vtValue);

	bool SetFieldValue(LPCTSTR lpFieldName, _variant_t vtValue);

	bool SetFieldEmpty(int nIndex);

	bool SetFieldEmpty(LPCTSTR lpFieldName);

	void CancelUpdate();

	bool Update();

	void Edit();

	bool AddNew();

	bool AddNew(CADORecordBinding &pAdoRecordBinding);

	bool Find(LPCTSTR lpFind, int nSearchDirection = CADORecordset::searchForward);

	bool FindFirst(LPCTSTR lpFind);

	bool FindNext();

	CADORecordset();

	CADORecordset(CADODatabase* pAdoDatabase);

	virtual ~CADORecordset()
	{
		Close();
		if(m_pRecordset)
			m_pRecordset.Release();
		if(m_pCmd)
			m_pCmd.Release();
		m_pRecordset = NULL;
		m_pCmd = NULL;
		m_pRecBinding = NULL;
		m_strQuery = "";
		m_strLastError = "";
		m_dwLastError = 0;
		m_nEditStatus = dbEditNone;
	}

	string GetQuery() 
	{
		return m_strQuery;
	}
	void SetQuery(LPCSTR strQuery) 
	{
		m_strQuery = strQuery;
	}
	bool RecordBinding(CADORecordBinding &pAdoRecordBinding);

	DWORD GetRecordCount();

	bool IsOpen();

	void Close();

	bool Open(_ConnectionPtr mpdb, LPCTSTR lpstrExec = "", int nOption = CADORecordset::openUnknown);

	bool Open(LPCTSTR lpstrExec = "", int nOption = CADORecordset::openUnknown);

	bool OpenSchema(int nSchema, LPCTSTR SchemaID = "");

	long GetFieldCount()
	{
		return m_pRecordset->Fields->GetCount();
	}

	template<typename T> bool GetFieldValue(LPCTSTR lpFieldName, T& tValue);

	template<typename T> bool GetFieldValue(int nIndex, T& tValue);

	bool GetFieldValue(LPCTSTR lpFieldName, string& strValue, string strDateFormat = "");

	bool GetFieldValue(int nIndex, string& strValue, string strDateFormat = "");

	bool GetFieldValue(LPCTSTR lpFieldName, char* pBuffer, int nBufferLen, string strDateFormat = "");

	bool GetFieldValue(int nIndex,  char* pBuffer, int nBufferLen, string strDateFormat = "");

	bool GetFieldValue(LPCTSTR lpFieldName, COleDateTime& time);

	bool GetFieldValue(int nIndex, COleDateTime& time);

	bool GetFieldValue(int nIndex, _variant_t& vtValue);

	bool GetFieldValue(LPCTSTR lpFieldName, _variant_t& vtValue);
	
	bool IsFieldNull(LPCTSTR lpFieldName);

	bool IsFieldNull(int nIndex);

	bool IsFieldEmpty(LPCTSTR lpFieldName);

	bool IsFieldEmpty(int nIndex);	

	bool IsEof()
	{
		return m_pRecordset->EndOfFile == VARIANT_TRUE;
	}
	bool IsBof()
	{
		return m_pRecordset->BOF == VARIANT_TRUE;
	}
	void MoveFirst() 
	{
		m_pRecordset->MoveFirst();
	}
	void MoveNext() 
	{
		m_pRecordset->MoveNext();
	}
	void MovePrevious() 
	{
		m_pRecordset->MovePrevious();
	}
	void MoveLast() 
	{
		m_pRecordset->MoveLast();
	}
	long GetAbsolutePage()
	{
		return m_pRecordset->GetAbsolutePage();
	}
	void SetAbsolutePage(int nPage)
	{
		m_pRecordset->PutAbsolutePage((enum PositionEnum)nPage);
	}
	long GetPageCount()
	{
		return m_pRecordset->GetPageCount();
	}
	long GetPageSize()
	{
		return m_pRecordset->GetPageSize();
	}
	void SetPageSize(int nSize)
	{
		m_pRecordset->PutPageSize(nSize);
	}
	long GetAbsolutePosition()
	{
		return m_pRecordset->GetAbsolutePosition();
	}
	void SetAbsolutePosition(int nPosition)
	{
		m_pRecordset->PutAbsolutePosition((enum PositionEnum)nPosition);
	}

	bool GetFieldInfo(LPCTSTR lpFieldName, CADOFieldInfo* fldInfo);

	bool GetFieldInfo(int nIndex, CADOFieldInfo* fldInfo);

	bool AppendChunk(LPCTSTR lpFieldName, LPVOID lpData, UINT nBytes);

	bool AppendChunk(int nIndex, LPVOID lpData, UINT nBytes);

	bool GetChunk(LPCTSTR lpFieldName, string& strValue);

	bool GetChunk(int nIndex, string& strValue);
	
	bool GetChunk(LPCTSTR lpFieldName, LPVOID pData);

	bool GetChunk(int nIndex, LPVOID pData);

	string GetString(LPCTSTR lpCols, LPCTSTR lpRows, LPCTSTR lpNull, long numRows = 0);

	string GetLastErrorString() 
	{
		return m_strLastError;
	}
	DWORD GetLastError()
	{
		return m_dwLastError;
	}
	void GetBookmark()
	{
		m_varBookmark = m_pRecordset->Bookmark;
	}
	bool SetBookmark();

	bool Delete();

	bool IsConnectionOpen()
	{
		return m_pConnection != NULL && m_pConnection->GetState() != adStateClosed;
	}
	_RecordsetPtr GetRecordset()
	{
		return m_pRecordset;
	}
	_ConnectionPtr GetActiveConnection() 
	{
		return m_pConnection;
	}

	bool SetFilter(LPCTSTR strFilter);

	bool SetSort(LPCTSTR lpstrCriteria);

	bool SaveAsXML(LPCTSTR lpstrXMLFile);

	bool OpenXML(LPCTSTR lpstrXMLFile);

	bool Execute(CADOCommand* pCommand);

	bool Requery();

public:
	_RecordsetPtr m_pRecordset;

	_CommandPtr m_pCmd;
	
protected:
	_ConnectionPtr m_pConnection;
	int m_nSearchDirection;
	string m_strFind;
	_variant_t m_varBookFind;
	_variant_t m_varBookmark;
	int m_nEditStatus;
	string m_strLastError;
	DWORD m_dwLastError;
	void dump_com_error(_com_error &e);
	IADORecordBinding *m_pRecBinding;
	string m_strQuery;

protected:
	bool PutFieldValue(LPCTSTR lpFieldName, _variant_t vtFld);
	bool PutFieldValue(_variant_t vtIndex, _variant_t vtFld);
	bool GetFieldInfo(FieldPtr pField, CADOFieldInfo* fldInfo);
	bool GetChunk(FieldPtr pField, string& strValue);
	bool GetChunk(FieldPtr pField, LPVOID lpData);
	bool AppendChunk(FieldPtr pField, LPVOID lpData, UINT nBytes);
		
};

template<typename T> 
bool CADORecordset::SetFieldValue(LPCTSTR lpFieldName, T& tValue)
{
	_variant_t vt;
	if( !TToVariant(tValue, vt) )
		return false;
	return SetFieldValue(lpFieldName, vt);
}

template<typename T> 
bool CADORecordset::SetFieldValue(int nIndex, T& tValue)
{
	_variant_t vt;
	if( !TToVariant(tValue, vt) )
		return false;
	return SetFieldValue(nIndex, vt);
}

template<typename T>
bool CADORecordset::GetFieldValue(LPCTSTR lpFieldName, T& tValue)
{
	_variant_t vt;
	if( !GetFieldValue(lpFieldName, vt) )
		return false;
	return VariantToT(vt, tValue);
}

template<typename T>
bool CADORecordset::GetFieldValue(int nIndex, T& tValue)
{
	_variant_t vt;
	if( !GetFieldValue(nIndex, vt) )
		return false;
	return VariantToT(vt, tValue);
}

class CADOParameter
{
public:

	enum cadoParameterDirection
	{
		paramUnknown = adParamUnknown,
		paramInput = adParamInput,
		paramOutput = adParamOutput,
		paramInputOutput = adParamInputOutput,
		paramReturnValue = adParamReturnValue 
	};

	CADOParameter(int nType, long lSize = 0, int nDirection = paramInput, string strName = "");
	
	virtual ~CADOParameter()
	{
		//::SysFreeString(m_pParameter->Name);
		m_pParameter.Release();
		m_pParameter = NULL;
		m_strName = "";
	}

	template <typename T> bool SetValue(T tValue)
	{	
		_variant_t vt;
		if( !TToVariant(tValue, vt) )
			return false;
		return SetValue(vt);
	}

	bool SetValue(string strValue);

	bool SetValue(COleDateTime time);

	bool SetValue(_variant_t vtValue);

	template <typename T> bool GetValue(T& tValue)
	{
		_variant_t vt;
		if( !GetValue(vt) )
			return false;
		return VariantToT(vt, tValue);
	}

	bool GetValue(string& strValue, string strDateFormat = "");

	bool GetValue(COleDateTime& time);

	bool GetValue(_variant_t& vtValue);

	void SetPrecision(int nPrecision)
	{
		m_pParameter->PutPrecision(nPrecision);
	}
	void SetScale(int nScale)
	{
		m_pParameter->PutNumericScale(nScale);
	}

	void SetName(string strName)
	{
		m_strName = strName;
	}
	string GetName()
	{
		return m_strName;
	}
	int GetType()
	{
		return m_nType;
	}
	_ParameterPtr GetParameter()
	{
		return m_pParameter;
	}

protected:
	void dump_com_error(_com_error &e);
	
protected:
	_ParameterPtr m_pParameter;
	string m_strName;
	int m_nType;
	string m_strLastError;
	DWORD m_dwLastError;
};

class CADOCommand
{
public:
	enum cadoCommandType
	{
		typeCmdText = adCmdText,
		typeCmdTable = adCmdTable,
		typeCmdTableDirect = adCmdTableDirect,
		typeCmdStoredProc = adCmdStoredProc,
		typeCmdUnknown = adCmdUnknown,
		typeCmdFile = adCmdFile
	};
	
	CADOCommand(CADODatabase* pAdoDatabase, string strCommandText = "", int nCommandType = typeCmdStoredProc);
		
	virtual ~CADOCommand()
	{
		/*::SysFreeString(m_pCommand->CommandText);*/
		m_pCommand.Release();
		m_pCommand = NULL;
		m_strCommandText = "";
	}

	void SetTimeout(long nTimeOut)
	{
		m_pCommand->PutCommandTimeout(nTimeOut);
	}
	void SetText(string strCommandText);

	void SetType(int nCommandType);

	int GetType()
	{
		return m_nCommandType;
	}
	bool AddParameter(CADOParameter* pAdoParameter);

	bool AddParameter(string strName, int nType, int nDirection, long lSize, int nValue);

	bool AddParameter(string strName, int nType, int nDirection, long lSize, long lValue);

	bool AddParameter(string strName, int nType, int nDirection, long lSize, double dblValue, int nPrecision = 0, int nScale = 0);

	bool AddParameter(string strName, int nType, int nDirection, long lSize, string strValue);

	bool AddParameter(string strName, int nType, int nDirection, long lSize, COleDateTime time);

	bool AddParameter(string strName, int nType, int nDirection, long lSize, _variant_t vtValue, int nPrecision = 0, int nScale = 0);

	string GetText()
	{
		return m_strCommandText;
	}
	bool Execute(int nCommandType = typeCmdStoredProc);
	int GetRecordsAffected()
	{
		return m_nRecordsAffected;
	}
	_CommandPtr GetCommand()
	{
		return m_pCommand;
	}

	string GetLastErrorString() { return m_strLastError; }

	void GetLastErrorString(char* pBuffer, int nBufferLen);

protected:
	void dump_com_error(_com_error &e);

protected:
	_CommandPtr m_pCommand;
	int m_nCommandType;
	int m_nRecordsAffected;
	string m_strCommandText;
	string m_strLastError;
	DWORD m_dwLastError;
};

/////////////////////////////////////////////////////////////////////
//
//		CADOException Class
//

class CADOException
{
public:
	CADOException() :
		m_lErrorCode(0),
		m_strError("")
		{
		}

	CADOException(long lErrorCode) :
		m_lErrorCode(lErrorCode),
		m_strError("")
		{
		}

	CADOException(long lErrorCode, const string& strError) :
		m_lErrorCode(lErrorCode),
		m_strError(strError)
		{
		}

	CADOException(const string& strError) :
		m_lErrorCode(0),
		m_strError(strError)
		{
		}

	CADOException(long lErrorCode, const char* szError) :
		m_lErrorCode(lErrorCode),
		m_strError(szError)
		{
		}

	CADOException(const char* szError) :
		m_lErrorCode(0),
		m_strError(szError)
		{
		}

	virtual ~CADOException()
		{
		}

	string GetErrorMessage() const
	{
		return m_strError;
	}
	void SetErrorMessage(LPCSTR lpstrError = "")
	{
		m_strError = lpstrError;
	}
	long GetError()
	{
		return m_lErrorCode;
	}
	void SetError(long lErrorCode = 0)
	{
		m_lErrorCode = lErrorCode;
	}

protected:
	string m_strError;
	long m_lErrorCode;
};

WT_END