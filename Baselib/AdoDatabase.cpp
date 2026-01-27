

#include "AdoDatabase.h"
#include "util.h"

WT_BEGIN

#define ChunkSize 100


///////////////////////////////////////////////////////
//
// CADODatabase Class
//

CADODatabase::CADODatabase()
{
	::CoInitialize(NULL);

	m_pConnection = NULL;
	m_strConnection = "";
	m_strLastError = "";
	m_dwLastError = 0;
	m_pConnection.CreateInstance(__uuidof(Connection));
	m_nRecordsAffected = 0;
	m_nConnectionTimeout = 0;
}

CADODatabase::~CADODatabase()
{
	Close();
	m_pConnection.Release();
	m_pConnection = NULL;
	m_strConnection = "";
	m_strLastError = "";
	m_dwLastError = 0;
	::CoUninitialize();
}

DWORD CADODatabase::GetRecordCount(_RecordsetPtr m_pRs)
{
	DWORD numRows = 0;

	numRows = m_pRs->GetRecordCount();

	if(numRows == -1)
	{
		if(m_pRs->EndOfFile != VARIANT_TRUE)
			m_pRs->MoveFirst();

		while(m_pRs->EndOfFile != VARIANT_TRUE)
		{
			numRows++;
			m_pRs->MoveNext();
		}
		if(numRows > 0)
			m_pRs->MoveFirst();
	}
	return numRows;
}

bool CADODatabase::Open(LPCTSTR lpstrConnection, LPCTSTR lpstrUserID, LPCTSTR lpstrPassword)
{
	HRESULT hr = S_OK;

	if(IsOpen())
		Close();

	if(strcmp(lpstrConnection, "") != 0)
		m_strConnection = lpstrConnection;

	//ASSERT(!m_strConnection.IsEmpty());

	try
	{
		if(m_nConnectionTimeout != 0)
			m_pConnection->PutConnectionTimeout(m_nConnectionTimeout);
		hr = m_pConnection->Open(_bstr_t(m_strConnection.c_str()), _bstr_t(lpstrUserID), _bstr_t(lpstrPassword), NULL);
		return hr == S_OK;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}

}

void CADODatabase::dump_com_error(_com_error &e)
{
	char szErrorInfo[4096] = {0};

	_bstr_t bstrSource(e.Source());
	_bstr_t bstrDescription(e.Description());
	sprintf_s(szErrorInfo, sizeof(szErrorInfo) - 1, "CADODataBase Error\n\tCode = %08lx\n\tCode meaning = %s\n\tSource = %s\n\tDescription = %s\n",
		e.Error(), e.ErrorMessage(), (LPCSTR)bstrSource, (LPCSTR)bstrDescription);

	m_strErrorDescription = (LPCSTR)bstrDescription ;
	m_strLastError = "Connection String = " + GetConnectionString() + '\n' + szErrorInfo;
	m_dwLastError = e.Error(); 

	//throw CADOException(e.Error(), e.Description());
}

bool CADODatabase::IsOpen()
{
	if(m_pConnection )
		return m_pConnection->GetState() != adStateClosed;
	return FALSE;
}

void CADODatabase::Close()
{
	if(IsOpen())
		m_pConnection->Close();
}


///////////////////////////////////////////////////////
//
// CADORecordset Class
//

CADORecordset::CADORecordset()
{
	m_pRecordset = NULL;
	m_pCmd = NULL;
	m_strQuery = "";
	m_strLastError = "";
	m_dwLastError = 0;
	m_pRecBinding = NULL;
	m_pRecordset.CreateInstance(__uuidof(Recordset));
	m_pCmd.CreateInstance(__uuidof(Command));
	m_nEditStatus = CADORecordset::dbEditNone;
	m_nSearchDirection = CADORecordset::searchForward;
}

CADORecordset::CADORecordset(CADODatabase* pAdoDatabase)
{
	m_pRecordset = NULL;
	m_pCmd = NULL;
	m_strQuery = "";
	m_strLastError = "";
	m_dwLastError = 0;
	m_pRecBinding = NULL;
	m_pRecordset.CreateInstance(__uuidof(Recordset));
	m_pCmd.CreateInstance(__uuidof(Command));
	m_nEditStatus = CADORecordset::dbEditNone;
	m_nSearchDirection = CADORecordset::searchForward;

	m_pConnection = pAdoDatabase->GetActiveConnection();
}

bool CADORecordset::Open(_ConnectionPtr mpdb, LPCTSTR lpstrExec, int nOption)
{	
	Close();

	if(strcmp(lpstrExec, "") != 0)
		m_strQuery = lpstrExec;

	//ASSERT(!m_strQuery.IsEmpty());

	if(m_pConnection == NULL)
		m_pConnection = mpdb;

	m_strQuery = StringTrimLeft(m_strQuery);
	string strSqlType = m_strQuery.substr(0, strlen("Select "));
	bool bIsSelect = strSqlType.compare("select ") == 0 && nOption == openUnknown;

	try
	{
		m_pRecordset->CursorType = adOpenStatic;
		m_pRecordset->CursorLocation = adUseClient;
		if(bIsSelect || nOption == openQuery || nOption == openUnknown)
			m_pRecordset->Open((LPCSTR)m_strQuery.c_str(), _variant_t((IDispatch*)mpdb, TRUE), 
			adOpenStatic, adLockOptimistic, adCmdUnknown);
		else if(nOption == openTable)
			m_pRecordset->Open((LPCSTR)m_strQuery.c_str(), _variant_t((IDispatch*)mpdb, TRUE), 
			adOpenKeyset, adLockOptimistic, adCmdTable);
		else if(nOption == openStoredProc)
		{
			m_pCmd->ActiveConnection = mpdb;
			m_pCmd->CommandText = _bstr_t(m_strQuery.c_str());
			m_pCmd->CommandType = adCmdStoredProc;
			m_pConnection->CursorLocation = adUseClient;

			m_pRecordset = m_pCmd->Execute(NULL, NULL, adCmdText);
		}
		else
		{
			return FALSE;
		}
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}

	return m_pRecordset != NULL && m_pRecordset->GetState()!= adStateClosed;
}

bool CADORecordset::Open(LPCTSTR lpstrExec, int nOption)
{
	//ASSERT(m_pConnection != NULL);
	//ASSERT(m_pConnection->GetState() != adStateClosed);
	return Open(m_pConnection, lpstrExec, nOption);
}

bool CADORecordset::OpenSchema(int nSchema, LPCTSTR SchemaID /*= ""*/)
{
	try
	{
		_variant_t vtSchemaID = vtMissing;

		if(strlen(SchemaID) != 0)
		{
			vtSchemaID = SchemaID;
			nSchema = adSchemaProviderSpecific;
		}

		m_pRecordset = m_pConnection->OpenSchema((enum SchemaEnum)nSchema, vtMissing, vtSchemaID);
		return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

bool CADORecordset::Requery()
{
	if(IsOpen())
	{
		try
		{
			m_pRecordset->Requery(adExecuteRecord);
		}
		catch(_com_error &e)
		{
			dump_com_error(e);
			return FALSE;
		}
	}
	return TRUE;
}

bool CADORecordset::GetFieldValue(LPCTSTR lpFieldName, string& strValue, string strDateFormat)
{
	string str;
	_variant_t vtFld;

	try
	{
		vtFld = m_pRecordset->Fields->GetItem(lpFieldName)->Value;
		switch(vtFld.vt) 
		{
		case VT_R4:
			str = DblToStr(vtFld.fltVal);
			break;
		case VT_R8:
			str = DblToStr(vtFld.dblVal);
			break;
		case VT_BSTR:
			str = _com_util::ConvertBSTRToString(vtFld.bstrVal);//vtFld.bstrVal;
			break;
		case VT_I2:
		case VT_UI1:
			str = IntToStr(vtFld.iVal);
			break;
		case VT_INT:
			str = IntToStr(vtFld.intVal);
			break;
		case VT_I4:
			str = LongToStr(vtFld.lVal);
			break;
		case VT_UI4:
			str = ULongToStr(vtFld.ulVal);
			break;
		case VT_DECIMAL:
			{
				//Corrected by Jos?Carlos Martínez Galán
				double val = vtFld.decVal.Lo32;
				val *= (vtFld.decVal.sign == 128)? -1 : 1;
				val /= pow((float)10, (int)vtFld.decVal.scale); 
				str = DblToStr(val);
			}
			break;
		case VT_DATE:
			{
				COleDateTime dt(vtFld);

				if(strDateFormat.empty())
					strDateFormat = "%Y-%m-%d %H:%M:%S";
				CString strFormatDate = dt.Format(strDateFormat.c_str());
				str = strFormatDate.GetBuffer();
				strFormatDate.ReleaseBuffer();
			}
			break;
		case VT_CY:		//Added by John Andy Johnson!!!
			{
				vtFld.ChangeType(VT_R8);

				char szFormat[32] = {0};
				sprintf_s(szFormat, sizeof(szFormat) - 1, "%f", vtFld.dblVal);

				char pszFormattedNumber[64] = {0};
				//	LOCALE_USER_DEFAULT
				if(GetCurrencyFormat(
					LOCALE_USER_DEFAULT,	// locale for which string is to be formatted 
					0,						// bit flag that controls the function's operation
					szFormat,					// pointer to input number string
					NULL,					// pointer to a formatting information structure
					//	NULL = machine default locale settings
					pszFormattedNumber,		// pointer to output buffer
					63))					// size of output buffer
				{
					str = pszFormattedNumber;
				}
			}
			break;
		case VT_EMPTY:
		case VT_NULL:
			str = "";
			break;
		case VT_BOOL:
			str = (vtFld.boolVal == VARIANT_TRUE? 'T':'F');
			break;
		default:
			str = "";
			return FALSE;
		}
		strValue = str;
		return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

bool CADORecordset::GetFieldValue(int nIndex, string& strValue, string strDateFormat)
{
	string str;
	_variant_t vtFld;
	_variant_t vtIndex;

	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;

	try
	{
		vtFld = m_pRecordset->Fields->GetItem(vtIndex)->Value;
		switch(vtFld.vt) 
		{
		case VT_R4:
			str = DblToStr(vtFld.fltVal);
			break;
		case VT_R8:
			str = DblToStr(vtFld.dblVal);
			break;
		case VT_BSTR:
			str =  _com_util::ConvertBSTRToString(vtFld.bstrVal);//vtFld.bstrVal;
			break;
		case VT_I2:
		case VT_UI1:
			str = IntToStr(vtFld.iVal);
			break;
		case VT_INT:
			str = IntToStr(vtFld.intVal);
			break;
		case VT_I4:
			str = LongToStr(vtFld.lVal);
			break;
		case VT_UI4:
			str = ULongToStr(vtFld.ulVal);
			break;
		case VT_DECIMAL:
			{
				double val = vtFld.decVal.Lo32;
				val *= (vtFld.decVal.sign == 128)? -1 : 1;
				val /= pow((float)10, (int)vtFld.decVal.scale); 
				str = DblToStr(val);
			}
			break;
		case VT_DATE:
			{
				COleDateTime dt(vtFld);

				if(strDateFormat.empty())
					strDateFormat = "%Y-%m-%d %H:%M:%S";
				CString strFormatDate = dt.Format(strDateFormat.c_str());
				str = strFormatDate.GetBuffer();
				strFormatDate.ReleaseBuffer();
			}
			break;
		case VT_CY:		//Added by John Andy Johnson!!!
			{
				vtFld.ChangeType(VT_R8);

				char szFormat[32] = {0};
				sprintf_s(szFormat, sizeof(szFormat) - 1, "%f", vtFld.dblVal);

				char pszFormattedNumber[64] = {0};

				//	LOCALE_USER_DEFAULT
				if(GetCurrencyFormat(
					LOCALE_USER_DEFAULT,	// locale for which string is to be formatted 
					0,						// bit flag that controls the function's operation
					szFormat,					// pointer to input number string
					NULL,					// pointer to a formatting information structure
					//	NULL = machine default locale settings
					pszFormattedNumber,		// pointer to output buffer
					63))					// size of output buffer
				{
					str = pszFormattedNumber;
				}
			}
			break;
		case VT_BOOL:
			str = (vtFld.boolVal == VARIANT_TRUE? 'T':'F');
			break;
		case VT_EMPTY:
		case VT_NULL:
			str = "";
			break;
		default:
			str = "";
			return FALSE;
		}
		strValue = str;
		return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

bool CADORecordset::GetFieldValue(LPCTSTR lpFieldName, char* pBuffer, int nBufferLen, string strDateFormat)
{
	if (pBuffer == NULL || nBufferLen <= 0)
		return FALSE;

	string str;
	_variant_t vtFld;

	try
	{
		vtFld = m_pRecordset->Fields->GetItem(lpFieldName)->Value;
		switch(vtFld.vt) 
		{
		case VT_R4:
			str = DblToStr(vtFld.fltVal);
			break;
		case VT_R8:
			str = DblToStr(vtFld.dblVal);
			break;
		case VT_BSTR:
			str = _com_util::ConvertBSTRToString(vtFld.bstrVal);//vtFld.bstrVal;
			break;
		case VT_I2:
		case VT_UI1:
			str = IntToStr(vtFld.iVal);
			break;
		case VT_INT:
			str = IntToStr(vtFld.intVal);
			break;
		case VT_I4:
			str = LongToStr(vtFld.lVal);
			break;
		case VT_UI4:
			str = ULongToStr(vtFld.ulVal);
			break;
		case VT_DECIMAL:
			{
				//Corrected by Jos?Carlos Martínez Galán
				double val = vtFld.decVal.Lo32;
				val *= (vtFld.decVal.sign == 128)? -1 : 1;
				val /= pow((float)10, (int)vtFld.decVal.scale); 
				str = DblToStr(val);
			}
			break;
		case VT_DATE:
			{
				COleDateTime dt(vtFld);

				if(strDateFormat.empty())
					strDateFormat = "%Y-%m-%d %H:%M:%S";
				CString strFormatDate = dt.Format(strDateFormat.c_str());
				str = strFormatDate.GetBuffer();
				strFormatDate.ReleaseBuffer();
			}
			break;
		case VT_CY:		//Added by John Andy Johnson!!!
			{
				vtFld.ChangeType(VT_R8);

				char szFormat[32] = {0};
				sprintf_s(szFormat, sizeof(szFormat) - 1, "%f", vtFld.dblVal);

				char pszFormattedNumber[64] = {0};
				//	LOCALE_USER_DEFAULT
				if(GetCurrencyFormat(
					LOCALE_USER_DEFAULT,	// locale for which string is to be formatted 
					0,						// bit flag that controls the function's operation
					szFormat,					// pointer to input number string
					NULL,					// pointer to a formatting information structure
					//	NULL = machine default locale settings
					pszFormattedNumber,		// pointer to output buffer
					63))					// size of output buffer
				{
					str = pszFormattedNumber;
				}
			}
			break;
		case VT_EMPTY:
		case VT_NULL:
			str = "";
			break;
		case VT_BOOL:
			str = (vtFld.boolVal == VARIANT_TRUE? 'T':'F');
			break;
		default:
			str = "";
			return FALSE;
		}

		memcpy(pBuffer, str.c_str(), min(str.length(), (size_t)nBufferLen));
		return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

bool CADORecordset::GetFieldValue(int nIndex,  char* pBuffer, int nBufferLen, string strDateFormat)
{
	if (pBuffer == NULL || nBufferLen <= 0)
		return FALSE;

	string str;
	_variant_t vtFld;
	_variant_t vtIndex;

	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;

	try
	{
		vtFld = m_pRecordset->Fields->GetItem(vtIndex)->Value;
		switch(vtFld.vt) 
		{
		case VT_R4:
			str = DblToStr(vtFld.fltVal);
			break;
		case VT_R8:
			str = DblToStr(vtFld.dblVal);
			break;
		case VT_BSTR:
			str =  _com_util::ConvertBSTRToString(vtFld.bstrVal);//vtFld.bstrVal;
			break;
		case VT_I2:
		case VT_UI1:
			str = IntToStr(vtFld.iVal);
			break;
		case VT_INT:
			str = IntToStr(vtFld.intVal);
			break;
		case VT_I4:
			str = LongToStr(vtFld.lVal);
			break;
		case VT_UI4:
			str = ULongToStr(vtFld.ulVal);
			break;
		case VT_DECIMAL:
			{
				double val = vtFld.decVal.Lo32;
				val *= (vtFld.decVal.sign == 128)? -1 : 1;
				val /= pow((float)10, (int)vtFld.decVal.scale); 
				str = DblToStr(val);
			}
			break;
		case VT_DATE:
			{
				COleDateTime dt(vtFld);

				if(strDateFormat.empty())
					strDateFormat = "%Y-%m-%d %H:%M:%S";
				CString strFormatDate = dt.Format(strDateFormat.c_str());
				str = strFormatDate.GetBuffer();
				strFormatDate.ReleaseBuffer();
			}
			break;
		case VT_CY:		//Added by John Andy Johnson!!!
			{
				vtFld.ChangeType(VT_R8);

				char szFormat[32] = {0};
				sprintf_s(szFormat, sizeof(szFormat) - 1, "%f", vtFld.dblVal);

				char pszFormattedNumber[64] = {0};

				//	LOCALE_USER_DEFAULT
				if(GetCurrencyFormat(
					LOCALE_USER_DEFAULT,	// locale for which string is to be formatted 
					0,						// bit flag that controls the function's operation
					szFormat,					// pointer to input number string
					NULL,					// pointer to a formatting information structure
					//	NULL = machine default locale settings
					pszFormattedNumber,		// pointer to output buffer
					63))					// size of output buffer
				{
					str = pszFormattedNumber;
				}
			}
			break;
		case VT_BOOL:
			str = (vtFld.boolVal == VARIANT_TRUE? 'T':'F');
			break;
		case VT_EMPTY:
		case VT_NULL:
			str = "";
			break;
		default:
			str = "";
			return FALSE;
		}

		memcpy(pBuffer, str.c_str(), min(str.length(), (size_t)nBufferLen));
		return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

bool CADORecordset::GetFieldValue(LPCTSTR lpFieldName, COleDateTime& time)
{
	_variant_t vtFld;

	try
	{
		vtFld = m_pRecordset->Fields->GetItem(lpFieldName)->Value;
		switch(vtFld.vt) 
		{
		case VT_DATE:
			{
				COleDateTime dt(vtFld);
				time = dt;
			}
			break;
		case VT_EMPTY:
		case VT_NULL:
			time.SetStatus(COleDateTime::null);
			break;
		default:
			return FALSE;
		}
		return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

bool CADORecordset::GetFieldValue(int nIndex, COleDateTime& time)
{
	_variant_t vtFld;
	_variant_t vtIndex;

	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;

	try
	{
		vtFld = m_pRecordset->Fields->GetItem(vtIndex)->Value;
		switch(vtFld.vt) 
		{
		case VT_DATE:
			{
				COleDateTime dt(vtFld);
				time = dt;
			}
			break;
		case VT_EMPTY:
		case VT_NULL:
			time.SetStatus(COleDateTime::null);
			break;
		default:
			return FALSE;
		}
		return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

/*bool CADORecordset::GetFieldValue(LPCTSTR lpFieldName, COleCurrency& cyValue)
{
_variant_t vtFld;

try
{
vtFld = m_pRecordset->Fields->GetItem(lpFieldName)->Value;
switch(vtFld.vt) 
{
case VT_CY:
cyValue = (CURRENCY)vtFld.cyVal;
break;
case VT_EMPTY:
case VT_NULL:
{
cyValue = COleCurrency();
cyValue.m_status = COleCurrency::null;
}
break;
default:
return FALSE;
}
return TRUE;
}
catch(_com_error &e)
{
dump_com_error(e);
return FALSE;
}
}

bool CADORecordset::GetFieldValue(int nIndex, COleCurrency& cyValue)
{
_variant_t vtFld;
_variant_t vtIndex;

vtIndex.vt = VT_I2;
vtIndex.iVal = nIndex;

try
{
vtFld = m_pRecordset->Fields->GetItem(vtIndex)->Value;
switch(vtFld.vt) 
{
case VT_CY:
cyValue = (CURRENCY)vtFld.cyVal;
break;
case VT_EMPTY:
case VT_NULL:
{
cyValue = COleCurrency();
cyValue.m_status = COleCurrency::null;
}
break;
default:
return FALSE;
}
return TRUE;
}
catch(_com_error &e)
{
dump_com_error(e);
return FALSE;
}
}*/

bool CADORecordset::GetFieldValue(LPCTSTR lpFieldName, _variant_t& vtValue)
{
	try
	{
		vtValue = m_pRecordset->Fields->GetItem(lpFieldName)->Value;
		return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

bool CADORecordset::GetFieldValue(int nIndex, _variant_t& vtValue)
{
	_variant_t vtIndex;

	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;

	try
	{
		vtValue = m_pRecordset->Fields->GetItem(vtIndex)->Value;
		return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

bool CADORecordset::IsFieldNull(LPCTSTR lpFieldName)
{
	_variant_t vtFld;

	try
	{
		vtFld = m_pRecordset->Fields->GetItem(lpFieldName)->Value;
		return vtFld.vt == VT_NULL;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

bool CADORecordset::IsFieldNull(int nIndex)
{
	_variant_t vtFld;
	_variant_t vtIndex;

	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;

	try
	{
		vtFld = m_pRecordset->Fields->GetItem(vtIndex)->Value;
		return vtFld.vt == VT_NULL;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

bool CADORecordset::IsFieldEmpty(LPCTSTR lpFieldName)
{
	_variant_t vtFld;

	try
	{
		vtFld = m_pRecordset->Fields->GetItem(lpFieldName)->Value;
		return vtFld.vt == VT_EMPTY || vtFld.vt == VT_NULL;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

bool CADORecordset::IsFieldEmpty(int nIndex)
{
	_variant_t vtFld;
	_variant_t vtIndex;

	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;

	try
	{
		vtFld = m_pRecordset->Fields->GetItem(vtIndex)->Value;
		return vtFld.vt == VT_EMPTY || vtFld.vt == VT_NULL;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

bool CADORecordset::SetFieldEmpty(LPCTSTR lpFieldName)
{
	_variant_t vtFld;
	vtFld.vt = VT_EMPTY;

	return PutFieldValue(lpFieldName, vtFld);
}

bool CADORecordset::SetFieldEmpty(int nIndex)
{
	_variant_t vtFld;
	vtFld.vt = VT_EMPTY;

	_variant_t vtIndex;

	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;

	return PutFieldValue(vtIndex, vtFld);
}


DWORD CADORecordset::GetRecordCount()
{
	DWORD nRows = 0;

	nRows = m_pRecordset->GetRecordCount();

	if(nRows == -1)
	{
		nRows = 0;
		if(m_pRecordset->EndOfFile != VARIANT_TRUE)
			m_pRecordset->MoveFirst();

		while(m_pRecordset->EndOfFile != VARIANT_TRUE)
		{
			nRows++;
			m_pRecordset->MoveNext();
		}
		if(nRows > 0)
			m_pRecordset->MoveFirst();
	}

	return nRows;
}

bool CADORecordset::IsOpen()
{
	if(m_pRecordset != NULL && IsConnectionOpen())
	{
		return m_pRecordset->GetState() != adStateClosed;
	}
	return FALSE;
}

void CADORecordset::Close()
{
	if(IsOpen())
	{
		if (m_nEditStatus != dbEditNone)
			CancelUpdate();

		m_pRecordset->PutSort("");
		m_pRecordset->Close();	
	}
}


bool CADODatabase::Execute(LPCTSTR lpstrExec)
{
	//ASSERT(m_pConnection != NULL);
	//ASSERT(strcmp(lpstrExec, "") != 0);
	_variant_t vRecords;

	m_nRecordsAffected = 0;

	try
	{
		m_pConnection->CursorLocation = adUseClient;
		m_pConnection->Execute(_bstr_t(lpstrExec), &vRecords, adExecuteNoRecords);
		m_nRecordsAffected = vRecords.iVal;
		return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;	
	}
}

bool CADORecordset::RecordBinding(CADORecordBinding &pAdoRecordBinding)
{
	HRESULT hr;
	m_pRecBinding = NULL;

	//Open the binding interface.
	if(FAILED(hr = m_pRecordset->QueryInterface(__uuidof(IADORecordBinding), (LPVOID*)&m_pRecBinding )))
	{
		_com_issue_error(hr);
		return FALSE;
	}

	//Bind the recordset to class
	if(FAILED(hr = m_pRecBinding->BindToRecordset(&pAdoRecordBinding)))
	{
		_com_issue_error(hr);
		return FALSE;
	}
	return TRUE;
}

bool CADORecordset::GetFieldInfo(LPCTSTR lpFieldName, CADOFieldInfo* fldInfo)
{
	FieldPtr pField = m_pRecordset->Fields->GetItem(lpFieldName);

	return GetFieldInfo(pField, fldInfo);
}

bool CADORecordset::GetFieldInfo(int nIndex, CADOFieldInfo* fldInfo)
{
	_variant_t vtIndex;

	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;

	FieldPtr pField = m_pRecordset->Fields->GetItem(vtIndex);

	return GetFieldInfo(pField, fldInfo);
}


bool CADORecordset::GetFieldInfo(FieldPtr pField, CADOFieldInfo* fldInfo)
{
	memset(fldInfo, 0, sizeof(CADOFieldInfo));

	strcpy_s(fldInfo->m_strName, sizeof(fldInfo->m_strName) - 1, (LPCTSTR)pField->GetName());
	fldInfo->m_lDefinedSize = pField->GetDefinedSize();
	fldInfo->m_nType = pField->GetType();
	fldInfo->m_lAttributes = pField->GetAttributes();
	if(!IsEof())
		fldInfo->m_lSize = pField->GetActualSize();
	return TRUE;
}

bool CADORecordset::GetChunk(LPCTSTR lpFieldName, string& strValue)
{
	FieldPtr pField = m_pRecordset->Fields->GetItem(lpFieldName);

	return GetChunk(pField, strValue);
}

bool CADORecordset::GetChunk(int nIndex, string& strValue)
{
	_variant_t vtIndex;

	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;

	FieldPtr pField = m_pRecordset->Fields->GetItem(vtIndex);

	return GetChunk(pField, strValue);
}


bool CADORecordset::GetChunk(FieldPtr pField, string& strValue)
{
	string str;
	long lngSize, lngOffSet = 0;
	_variant_t varChunk;

	lngSize = pField->ActualSize;

	str = "";
	while(lngOffSet < lngSize)
	{ 
		try
		{
			varChunk = pField->GetChunk(ChunkSize);

			str += _com_util::ConvertBSTRToString(varChunk.bstrVal);//varChunk.bstrVal;
			lngOffSet += ChunkSize;
		}
		catch(_com_error &e)
		{
			dump_com_error(e);
			return FALSE;
		}
	}

	lngOffSet = 0;
	strValue = str;
	return TRUE;
}

bool CADORecordset::GetChunk(LPCTSTR lpFieldName, LPVOID lpData)
{
	FieldPtr pField = m_pRecordset->Fields->GetItem(lpFieldName);

	return GetChunk(pField, lpData);
}

bool CADORecordset::GetChunk(int nIndex, LPVOID lpData)
{
	_variant_t vtIndex;

	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;

	FieldPtr pField = m_pRecordset->Fields->GetItem(vtIndex);

	return GetChunk(pField, lpData);
}

bool CADORecordset::GetChunk(FieldPtr pField, LPVOID lpData)
{
	long lngSize, lngOffSet = 0;
	_variant_t varChunk;    
	UCHAR chData;
	HRESULT hr;
	long lBytesCopied = 0;

	lngSize = pField->ActualSize;

	while(lngOffSet < lngSize)
	{ 
		try
		{
			varChunk = pField->GetChunk(ChunkSize);

			//Copy the data only upto the Actual Size of Field.  
			for(long lIndex = 0; lIndex <= (ChunkSize - 1); lIndex++)
			{
				hr= SafeArrayGetElement(varChunk.parray, &lIndex, &chData);
				if(SUCCEEDED(hr))
				{
					//Take BYTE by BYTE and advance Memory Location
					//hr = SafeArrayPutElement((SAFEARRAY FAR*)lpData, &lBytesCopied ,&chData); 
					((UCHAR*)lpData)[lBytesCopied] = chData;
					lBytesCopied++;
				}
				else
					break;
			}
			lngOffSet += ChunkSize;
		}
		catch(_com_error &e)
		{
			dump_com_error(e);
			return FALSE;
		}
	}

	lngOffSet = 0;
	return TRUE;
}

bool CADORecordset::AppendChunk(LPCTSTR lpFieldName, LPVOID lpData, UINT nBytes)
{

	FieldPtr pField = m_pRecordset->Fields->GetItem(lpFieldName);

	return AppendChunk(pField, lpData, nBytes);
}


bool CADORecordset::AppendChunk(int nIndex, LPVOID lpData, UINT nBytes)
{
	_variant_t vtIndex;

	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;

	FieldPtr pField = m_pRecordset->Fields->GetItem(vtIndex);

	return AppendChunk(pField, lpData, nBytes);
}

bool CADORecordset::AppendChunk(FieldPtr pField, LPVOID lpData, UINT nBytes)
{
	HRESULT hr;
	_variant_t varChunk;
	long lngOffset = 0;
	UCHAR chData;
	SAFEARRAY FAR *psa = NULL;
	SAFEARRAYBOUND rgsabound[1];

	try
	{
		//Create a safe array to store the array of BYTES 
		rgsabound[0].lLbound = 0;
		rgsabound[0].cElements = nBytes;
		psa = SafeArrayCreate(VT_UI1,1,rgsabound);

		while(lngOffset < (long)nBytes)
		{
			chData	= ((UCHAR*)lpData)[lngOffset];
			hr = SafeArrayPutElement(psa, &lngOffset, &chData);

			if(FAILED(hr))
				return FALSE;

			lngOffset++;
		}
		lngOffset = 0;

		//Assign the Safe array  to a variant. 
		varChunk.vt = VT_ARRAY|VT_UI1;
		varChunk.parray = psa;

		hr = pField->AppendChunk(varChunk);

		if(SUCCEEDED(hr)) return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}

	return FALSE;
}

string CADORecordset::GetString(LPCTSTR lpCols, LPCTSTR lpRows, LPCTSTR lpNull, long numRows)
{
	_bstr_t varOutput;
	_bstr_t varNull("");
	_bstr_t varCols("\t");
	_bstr_t varRows("\r");

	if(strlen(lpCols) != 0)
		varCols = _bstr_t(lpCols);

	if(strlen(lpRows) != 0)
		varRows = _bstr_t(lpRows);

	if(numRows == 0)
		numRows =(long)GetRecordCount();			

	varOutput = m_pRecordset->GetString(adClipString, numRows, varCols, varRows, varNull);

	return (LPCTSTR)varOutput;
}

string IntToStr(int nVal)
{
	string strRet;
	char buff[12];

	_itoa_s(nVal, buff, 11, 10);
	strRet = buff;
	return strRet;
}

string LongToStr(long lVal)
{
	string strRet;
	char buff[20];

	_ltoa_s(lVal, buff, 19, 10);
	strRet = buff;
	return strRet;
}

string ULongToStr(unsigned long ulVal)
{
	string strRet;
	char buff[20];

	_ultoa_s(ulVal, buff, 19, 10);
	strRet = buff;
	return strRet;

}


string DblToStr(double dblVal, int ndigits)
{
	string strRet;
	char buff[50];

	//_gcvt_s(dblVal, 49, ndigits, buff);
	_gcvt_s(buff, 49, dblVal, ndigits);
	strRet = buff;
	return strRet;
}

string DblToStr(float fltVal)
{
	string strRet;
	char buff[50];

	//_gcvt_s(fltVal, 49, 10, buff);
	_gcvt_s(buff, 49, fltVal, 10);
	strRet = buff;
	return strRet;
}

void CADORecordset::Edit()
{
	m_nEditStatus = dbEdit;
}

bool CADORecordset::AddNew()
{
	m_nEditStatus = dbEditNone;
	if(m_pRecordset->AddNew() != S_OK)
		return FALSE;

	m_nEditStatus = dbEditNew;
	return TRUE;
}

bool CADORecordset::AddNew(CADORecordBinding &pAdoRecordBinding)
{
	try
	{
		if(m_pRecBinding->AddNew(&pAdoRecordBinding) != S_OK)
		{
			return FALSE;
		}
		else
		{
			m_pRecBinding->Update(&pAdoRecordBinding);
			return TRUE;
		}

	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}	
}

bool CADORecordset::Update()
{
	bool bret = TRUE;

	if(m_nEditStatus != dbEditNone)
	{

		try
		{
			if(m_pRecordset->Update() != S_OK)
				bret = FALSE;
		}
		catch(_com_error &e)
		{
			dump_com_error(e);
			bret = FALSE;
		}

		if(!bret)
			m_pRecordset->CancelUpdate();
		m_nEditStatus = dbEditNone;
	}
	return bret;
}

void CADORecordset::CancelUpdate()
{
	m_pRecordset->CancelUpdate();
	m_nEditStatus = dbEditNone;
}

bool CADORecordset::SetFieldValue(int nIndex, COleDateTime time)
{
	_variant_t vtFld;
	vtFld.vt = VT_DATE;
	vtFld.date = time;

	_variant_t vtIndex;

	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;

	return PutFieldValue(vtIndex, vtFld);
}

bool CADORecordset::SetFieldValue(LPCTSTR lpFieldName, COleDateTime time)
{
	_variant_t vtFld;
	vtFld.vt = VT_DATE;
	vtFld.date = time;

	return PutFieldValue(lpFieldName, vtFld);
}

bool CADORecordset::SetFieldValue(int nIndex, _variant_t vtValue)
{
	_variant_t vtIndex;

	vtIndex.vt = VT_I2;
	vtIndex.iVal = nIndex;

	return PutFieldValue(vtIndex, vtValue);
}

bool CADORecordset::SetFieldValue(LPCTSTR lpFieldName, _variant_t vtValue)
{	
	return PutFieldValue(lpFieldName, vtValue);
}


bool CADORecordset::SetBookmark()
{
	if(m_varBookmark.vt != VT_EMPTY)
	{
		m_pRecordset->Bookmark = m_varBookmark;
		return TRUE;
	}
	return FALSE;
}

bool CADORecordset::Delete()
{
	if(m_pRecordset->Delete(adAffectCurrent) != S_OK)
		return FALSE;

	if(m_pRecordset->Update() != S_OK)
		return FALSE;

	m_nEditStatus = dbEditNone;
	return TRUE;
}

bool CADORecordset::Find(LPCTSTR lpFind, int nSearchDirection)
{
	m_strFind = lpFind;
	m_nSearchDirection = nSearchDirection;

	if(!m_strFind.empty())
		return FALSE;

	if(m_nSearchDirection == searchForward)
	{
		m_pRecordset->Find(_bstr_t(m_strFind.c_str()), 0, adSearchForward, "");
		if(!IsEof())
		{
			m_varBookFind = m_pRecordset->Bookmark;
			return TRUE;
		}
	}
	else if(m_nSearchDirection == searchBackward)
	{
		m_pRecordset->Find(_bstr_t(m_strFind.c_str()), 0, adSearchBackward, "");
		if(!IsBof())
		{
			m_varBookFind = m_pRecordset->Bookmark;
			return TRUE;
		}
	}
	else
	{
		m_nSearchDirection = searchForward;
	}
	return FALSE;
}

bool CADORecordset::FindFirst(LPCTSTR lpFind)
{
	m_pRecordset->MoveFirst();
	return Find(lpFind);
}

bool CADORecordset::FindNext()
{
	if(m_nSearchDirection == searchForward)
	{
		m_pRecordset->Find(_bstr_t(m_strFind.c_str()), 1, adSearchForward, m_varBookFind);
		if(!IsEof())
		{
			m_varBookFind = m_pRecordset->Bookmark;
			return TRUE;
		}
	}
	else
	{
		m_pRecordset->Find(_bstr_t(m_strFind.c_str()), 1, adSearchBackward, m_varBookFind);
		if(!IsBof())
		{
			m_varBookFind = m_pRecordset->Bookmark;
			return TRUE;
		}
	}
	return FALSE;
}

bool CADORecordset::PutFieldValue(LPCTSTR lpFieldName, _variant_t vtFld)
{
	if(m_nEditStatus == dbEditNone)
		return FALSE;

	try
	{
		m_pRecordset->Fields->GetItem(lpFieldName)->Value = vtFld; 
		return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;	
	}
}


bool CADORecordset::PutFieldValue(_variant_t vtIndex, _variant_t vtFld)
{
	if(m_nEditStatus == dbEditNone)
		return FALSE;

	try
	{
		m_pRecordset->Fields->GetItem(vtIndex)->Value = vtFld;
		return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

bool CADORecordset::Clone(CADORecordset &pRs)
{
	try
	{
		pRs.m_pRecordset = m_pRecordset->Clone(adLockUnspecified);
		return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

bool CADORecordset::SetFilter(LPCTSTR strFilter)
{
	//ASSERT(IsOpen());

	try
	{
		m_pRecordset->PutFilter(strFilter);
		return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

bool CADORecordset::SetSort(LPCTSTR strCriteria)
{
	//ASSERT(IsOpen());

	try
	{
		m_pRecordset->PutSort(strCriteria);
		return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

bool CADORecordset::SaveAsXML(LPCTSTR lpstrXMLFile)
{
	HRESULT hr;

	//ASSERT(IsOpen());

	try
	{
		hr = m_pRecordset->Save(lpstrXMLFile, adPersistXML);
		return hr == S_OK;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
	return TRUE;
}

bool CADORecordset::OpenXML(LPCTSTR lpstrXMLFile)
{
	HRESULT hr = S_OK;

	if(IsOpen())
		Close();

	try
	{
		hr = m_pRecordset->Open(lpstrXMLFile, "Provider=MSPersist;", adOpenForwardOnly, adLockOptimistic, adCmdFile);
		return hr == S_OK;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

bool CADORecordset::Execute(CADOCommand* pAdoCommand)
{
	if(IsOpen())
		Close();

	//ASSERT(!pAdoCommand->GetText().IsEmpty());
	try
	{
		m_pConnection->CursorLocation = adUseClient;
		m_pRecordset = pAdoCommand->GetCommand()->Execute(NULL, NULL, pAdoCommand->GetType());
		return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

void CADORecordset::dump_com_error(_com_error &e)
{
	char szErrorInfo[4096] = {0};

	_bstr_t bstrSource(e.Source());
	_bstr_t bstrDescription(e.Description());

	sprintf_s(szErrorInfo, sizeof(szErrorInfo) - 1, "CADORecordset Error\n\tCode = %08lx\n\tCode meaning = %s\n\tSource = %s\n\tDescription = %s\n",
		e.Error(), e.ErrorMessage(), (LPCSTR)bstrSource, (LPCSTR)bstrDescription);

	m_strLastError = "Query = " + GetQuery() + '\n' + szErrorInfo;
	m_dwLastError = e.Error();	

	//throw CADOException(e.Error(), e.Description());
}


///////////////////////////////////////////////////////
//
// CADOCommad Class
//

CADOCommand::CADOCommand(CADODatabase* pAdoDatabase, string strCommandText, int nCommandType)
{
	m_pCommand = NULL;
	m_pCommand.CreateInstance(__uuidof(Command));
	m_strCommandText = strCommandText;
	m_pCommand->CommandText = m_strCommandText.c_str();//_com_util::ConvertStringToBSTR(m_strCommandText.c_str());
	m_nCommandType = nCommandType;
	m_pCommand->CommandType = (CommandTypeEnum)m_nCommandType;
	m_pCommand->ActiveConnection = pAdoDatabase->GetActiveConnection();	
	m_nRecordsAffected = 0;
}

void CADOCommand::GetLastErrorString(char* pBuffer, int nBufferLen)
{
	if (pBuffer == NULL || nBufferLen == 0)
		return;

	memcpy(pBuffer, m_strLastError.c_str(), min((size_t)(nBufferLen - 1), m_strLastError.length()));
}

bool CADOCommand::AddParameter(CADOParameter* pAdoParameter)
{
	//ASSERT(pAdoParameter->GetParameter() != NULL);

	try
	{
		m_pCommand->Parameters->Append(pAdoParameter->GetParameter());
		return TRUE;
	}
	catch(_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

bool CADOCommand::AddParameter(string strName, int nType, int nDirection, long lSize, int nValue)
{
	_variant_t vtValue;

	vtValue.vt = VT_I2;
	vtValue.iVal = nValue;

	return AddParameter(strName, nType, nDirection, lSize, vtValue);
}

bool CADOCommand::AddParameter(string strName, int nType, int nDirection, long lSize, long lValue)
{
	_variant_t vtValue;

	vtValue.vt = VT_I4;
	vtValue.lVal = lValue;

	return AddParameter(strName, nType, nDirection, lSize, vtValue);
}

bool CADOCommand::AddParameter(string strName, int nType, int nDirection, long lSize, double dblValue, int nPrecision, int nScale)
{
	_variant_t vtValue;

	vtValue.vt = VT_R8;
	vtValue.dblVal = dblValue;

	return AddParameter(strName, nType, nDirection, lSize, vtValue, nPrecision, nScale);
}

bool CADOCommand::AddParameter(string strName, int nType, int nDirection, long lSize, string strValue)
{
	_variant_t vtValue;

	vtValue.vt = VT_BSTR;
	vtValue.bstrVal = _com_util::ConvertStringToBSTR(strValue.c_str());//SysAllocString(strValue.c_str());

	return AddParameter(strName, nType, nDirection, lSize, vtValue);
}

bool CADOCommand::AddParameter(string strName, int nType, int nDirection, long lSize, COleDateTime time)
{
	_variant_t vtValue;

	vtValue.vt = VT_DATE;
	vtValue.date = time;

	return AddParameter(strName, nType, nDirection, lSize, vtValue);
}


bool CADOCommand::AddParameter(string strName, int nType, int nDirection, long lSize, _variant_t vtValue, int nPrecision, int nScale)
{
	try
	{
		_ParameterPtr pParam = m_pCommand->CreateParameter(strName.c_str(),
			(DataTypeEnum)nType, (ParameterDirectionEnum)nDirection, lSize, vtValue);
		pParam->PutPrecision(nPrecision);
		pParam->PutNumericScale(nScale);
		m_pCommand->Parameters->Append(pParam);

		return TRUE;
	}
	catch(_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

void CADOCommand::SetText(string strCommandText)
{
	if (strCommandText.empty())
		return;

	m_strCommandText = strCommandText;
	m_pCommand->CommandText = strCommandText.c_str();//_com_util::ConvertStringToBSTR(m_strCommandText.c_str());//SysAllocString(m_strCommandText.c_str());
}

void CADOCommand::SetType(int nCommandType)
{
	m_nCommandType = nCommandType;
	m_pCommand->CommandType = (CommandTypeEnum)m_nCommandType;
}

bool CADOCommand::Execute(int nCommandType /*= typeCmdStoredProc*/)
{
	_variant_t vRecords;
	m_nRecordsAffected = 0;
	try
	{
		m_nCommandType = nCommandType;
		m_pCommand->Execute(&vRecords, NULL, nCommandType);
		m_nRecordsAffected = vRecords.iVal;
		return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

void CADOCommand::dump_com_error(_com_error &e)
{
	char    szErrorInfo[4096] = {0};

	_bstr_t bstrSource(e.Source());
	_bstr_t bstrDescription(e.Description());
	sprintf_s(szErrorInfo, sizeof(szErrorInfo) - 1, "CADOCommand Error\n\tCode = %08lx\n\tCode meaning = %s\n\tSource = %s\n\tDescription = %s\n",
		e.Error(), e.ErrorMessage(), (LPCSTR)bstrSource, (LPCSTR)bstrDescription);

	m_strLastError = szErrorInfo;
	m_dwLastError = e.Error();

	//throw CADOException(e.Error(), e.Description());
}


///////////////////////////////////////////////////////
//
// CADOParameter Class
//

CADOParameter::CADOParameter(int nType, long lSize, int nDirection, string strName)
{
	m_pParameter = NULL;
	m_pParameter.CreateInstance(__uuidof(Parameter));
	m_strName = "";
	m_pParameter->Direction = (ParameterDirectionEnum)nDirection;
	m_strName = strName;
	m_pParameter->Name = m_strName.c_str();//_com_util::ConvertStringToBSTR(m_strName.c_str());//SysAllocString(m_strName.c_str());
	m_pParameter->Type = (DataTypeEnum)nType;
	m_pParameter->Size = lSize;
	m_nType = nType;
}


bool CADOParameter::SetValue(string strValue)
{
	_variant_t vtVal;

	//ASSERT(m_pParameter != NULL);

	if(!strValue.empty())
		vtVal.vt = VT_BSTR;
	else
		vtVal.vt = VT_NULL;

	//Corrected by Giles Forster 10/03/2001
	vtVal.bstrVal = _com_util::ConvertStringToBSTR(strValue.c_str());//SysAllocString(strValue.c_str());
	//vtVal.SetString(strValue.GetBuffer(0));

	try
	{
		if(m_pParameter->Size == 0)
			m_pParameter->Size = (long)sizeof(char) * (long)strValue.length();

		m_pParameter->Value = vtVal;
		::SysFreeString(vtVal.bstrVal);
		return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		::SysFreeString(vtVal.bstrVal);
		return FALSE;
	}
}


bool CADOParameter::SetValue(COleDateTime time)
{
	_variant_t vtVal;

	//ASSERT(m_pParameter != NULL);

	vtVal.vt = VT_DATE;
	vtVal.date = time;

	try
	{
		if(m_pParameter->Size == 0)
			m_pParameter->Size = sizeof(DATE);

		m_pParameter->Value = vtVal;
		return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}


bool CADOParameter::SetValue(_variant_t vtValue)
{

	//ASSERT(m_pParameter != NULL);

	try
	{
		if(m_pParameter->Size == 0)
			m_pParameter->Size = sizeof(VARIANT);

		m_pParameter->Value = vtValue;
		return TRUE;
	}
	catch(_com_error &e)
	{
		dump_com_error(e);
		return FALSE;
	}
}

bool CADOParameter::GetValue(string& strValue, string strDateFormat)
{
	_variant_t vtVal;
	string strVal;

	try
	{
		vtVal = m_pParameter->Value;
		switch(vtVal.vt) 
		{
		case VT_R4:
			strVal = DblToStr(vtVal.fltVal);
			break;
		case VT_R8:
			strVal = DblToStr(vtVal.dblVal);
			break;
		case VT_BSTR:
			strVal =  _com_util::ConvertBSTRToString(vtVal.bstrVal);//vtVal.bstrVal;
			break;
		case VT_I2:
		case VT_UI1:
			strVal = IntToStr(vtVal.iVal);
			break;
		case VT_INT:
			strVal = IntToStr(vtVal.intVal);
			break;
		case VT_I4:
			strVal = LongToStr(vtVal.lVal);
			break;
		case VT_DECIMAL:
			{
				double val = vtVal.decVal.Lo32;
				val *= (vtVal.decVal.sign == 128)? -1 : 1;
				val /= pow((float)10, (int)vtVal.decVal.scale); 
				strVal = DblToStr(val);
			}
			break;
		case VT_DATE:
			{
				COleDateTime dt(vtVal);

				if(strDateFormat.empty())
					strDateFormat = _T("%Y-%m-%d %H:%M:%S");
				CString strFormatDate = dt.Format(strDateFormat.c_str());
				strVal = strFormatDate.GetBuffer();
				strFormatDate.ReleaseBuffer();
			}
			break;
		case VT_EMPTY:
		case VT_NULL:
			strVal = "";
			break;
		default:
			strVal = "";
			return FALSE;
		}
		strValue = strVal;
		return TRUE;
	}
	catch(_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}


bool CADOParameter::GetValue(COleDateTime& time)
{
	_variant_t vtVal;

	try
	{
		vtVal = m_pParameter->Value;
		switch(vtVal.vt) 
		{
		case VT_DATE:
			{
				COleDateTime dt(vtVal);
				time = dt;
			}
			break;
		case VT_EMPTY:
		case VT_NULL:
			time.SetStatus(COleDateTime::null);
			break;
		default:
			return FALSE;
		}
		return TRUE;
	}
	catch(_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}


bool CADOParameter::GetValue(_variant_t& vtValue)
{
	try
	{
		vtValue = m_pParameter->Value;
		return TRUE;
	}
	catch(_com_error& e)
	{
		dump_com_error(e);
		return FALSE;
	}
}


void CADOParameter::dump_com_error(_com_error &e)
{
	char szErroInfo[4096] = {0};

	_bstr_t bstrSource(e.Source());
	_bstr_t bstrDescription(e.Description());
	sprintf_s(szErroInfo, sizeof(szErroInfo) - 1, "CADOParameter Error\n\tCode = %08lx\n\tCode meaning = %s\n\tSource = %s\n\tDescription = %s\n",
		e.Error(), e.ErrorMessage(), (LPCSTR)bstrSource, (LPCSTR)bstrDescription);
	m_strLastError = szErroInfo;
	m_dwLastError = e.Error();
}

WT_END