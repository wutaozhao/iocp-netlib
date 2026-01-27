#pragma once
#include <string>
#include <sstream>
#include <comutil.h>
#include <time.h>
using namespace std;

#include "Config.h"

WT_BEGIN

#define ADO_MAX_TIME64_T 0x7915ECBFFi64

struct TimeHolder
{
	TimeHolder():time(0) {}
	TimeHolder(time_t t):time(t) {}
	TimeHolder(const TimeHolder& rhs)
	{
		time = rhs.time;
	}

	TimeHolder& operator =(const TimeHolder& rhs) 
	{
		time = rhs.time;
		return *this;
	}
	time_t time;
};

template <typename T>
inline bool VariantToT(_variant_t& value, T& tValue)
{
	return false;
}

template <typename T>
inline bool TToVariant(T& tValue, _variant_t& value)
{
	return false;
}

// 特化char
template <> inline bool VariantToT<char>(_variant_t& value, char& tValue)
{
	switch(value.vt)
	{
	case VT_BOOL:
		tValue = (char)value.boolVal;
		break;
	case VT_I1:
		tValue = (char)value.cVal;
		break;
	case VT_UI1:
		tValue = (char)value.bVal;
		break;
	default:
		return false;
	}

	return true;
}

template <> inline bool VariantToT<unsigned char>(_variant_t& value, unsigned char& tValue)
{
	switch(value.vt)
	{
	case VT_BOOL:
		tValue = (unsigned char)value.boolVal;
		break;
	case VT_I1:
		tValue = (unsigned char)value.cVal;
		break;
	case VT_UI1:
		tValue = (unsigned char)value.bVal;
			break;
	default:
		return false;
	}

	return true;
}

template <> inline bool TToVariant<char>(char& tValue, _variant_t& value)
{
	value.vt = VT_I1;
	value.cVal = tValue;
	return FALSE;
}

template <> inline bool TToVariant<unsigned char>(unsigned char& tValue, _variant_t& value)
{
	value.vt = VT_UI1;
	value.cVal = tValue;
	return FALSE;
}

// 特化short
template <> inline bool VariantToT<short>(_variant_t& value, short& tValue)
{
	switch(value.vt)
	{
	case VT_I2:
		tValue = (short)value.iVal;
		break;
	case VT_UI2:
		tValue = (short)value.uiVal;
		break;
	default:
		{
			char v = 0;
			bool b = VariantToT<char>(value, v);
			if( b )
				tValue = v;
			return b;
		}
	}

	return true;
}

template <> inline bool VariantToT<unsigned short>(_variant_t& value, unsigned short& tValue)
{
	switch( value.vt )
	{
	case VT_I2:
		tValue = (unsigned short)value.iVal; 
		break;
	case VT_UI2:
		tValue = (unsigned short)value.uiVal;
		break;
	default:
		{
			unsigned char v = 0;
			bool b = VariantToT<unsigned char>(value, v);
			if( b )
				tValue = v;
			return b;
		}
	}
	return true;
}

template <> inline bool TToVariant<short>(short& value, _variant_t& tValue)
{
	tValue.vt = VT_I2;
	tValue.iVal = value;
	return true;
}

template <> inline bool TToVariant<unsigned short>(unsigned short& value, _variant_t& tValue)
{
	tValue.vt = VT_UI2;
	tValue.uiVal = value;
	return true;
}

// 特化int
template <> inline bool VariantToT<int>(_variant_t& value, int& tValue)
{
	switch( value.vt )
	{
	case VT_I4:
		tValue = value.lVal;
		break;
	case VT_INT:
		tValue = value.intVal;
		break;
	case VT_UI4:
		tValue = (int)value.ulVal;
		break;
	case VT_UINT:
		tValue = (int)value.uintVal;
		break;
	default:
		{
			short v = 0;
			bool b = VariantToT<short>(value, v);
			if( b )
				tValue = v;
			return b;
		}
	}
	return true;
}

template <> inline bool VariantToT<unsigned int>(_variant_t& value, unsigned int& tValue)
{
	switch( value.vt )
	{
	case VT_I4:
		tValue = (unsigned int)value.lVal;
		break;
	case VT_INT:
		tValue = (unsigned int)value.intVal;
		break;
	case VT_UI4:
		tValue = (unsigned int)value.ulVal;
		break;
	case VT_UINT:
		tValue = (unsigned int)value.uintVal;
		break;
	default:
		{
			unsigned short v = 0;
			bool b = VariantToT<unsigned short>(value, v);
			if( b )
				tValue = v;
			return b;
		}
	}
	return true;
}

template <> inline bool TToVariant<int>(int& tValue, _variant_t& value)
{
	value.vt = VT_INT;
	value.intVal = tValue;
	return true;
}

template <> inline bool TToVariant<unsigned int>(unsigned int& tValue, _variant_t& value)
{
	value.vt = VT_UINT;
	value.uintVal = tValue;
	return true;
}

//特化long
template <> inline bool VariantToT<long>(_variant_t& value, long& tValue)
{
	return VariantToT<int>(value, (int&)tValue);
}

template <> inline bool VariantToT<unsigned long>(_variant_t& value, unsigned long& tValue)
{
	return VariantToT<unsigned int>(value, (unsigned int&)tValue);
}

template <> inline bool TToVariant<long>(long& tValue, _variant_t& value)
{
	value.vt = VT_I4;
	value.lVal = tValue;
	return true;
}

template <> inline bool TToVariant<unsigned long>(unsigned long& tValue, _variant_t& value)
{
	value.vt = VT_UI4;
	value.ulVal = tValue;
	return true;
}

// 特化int64
template <> inline bool VariantToT<signed long long>(_variant_t& value, signed long long& tValue)
{
	switch( value.vt )
	{
	case VT_I8:
		tValue = value.llVal;
		break;
	case VT_UI8:
		tValue = (signed long long)value.ullVal;
		break;
	case VT_DECIMAL:
		tValue = value.decVal.Lo64;
		tValue *= (value.decVal.sign == 128)? -1 : 1;
		break;
	default:
		{
			int v = 0;
			bool b = VariantToT<int>(value, v);
			if( b )
				tValue = v;
			return b;
		}
	}
	return true;
}

template <> inline bool VariantToT<unsigned long long>(_variant_t& value, unsigned long long& tValue)
{
	switch( value.vt )
	{
	case VT_I8:
		tValue = (unsigned long long)value.llVal;
		break;
	case VT_UI8:
		tValue = (unsigned long long)value.ullVal;
		break;
	case VT_DECIMAL:
		tValue = value.decVal.Lo64;
		tValue *= (value.decVal.sign == 128)? -1 : 1;
		break;
	default:
		{
			unsigned int v = 0;
			bool b = VariantToT<unsigned int>(value, v);
			if( b )
				tValue = v;
			return b;
		}
	}
	return true;
}

template <> inline bool TToVariant<signed long long>(signed long long& tValue, _variant_t& value)
{
	value.vt = VT_I8;
	value.llVal = tValue;
	return true;
}

template <> inline bool TToVariant<unsigned long long>(unsigned long long& tValue, _variant_t& value)
{
	value.vt = VT_UI8;
	value.ullVal = tValue;
	return true;
}

// 特化float
template <> inline bool VariantToT<float>(_variant_t& value, float& tValue)
{
	switch( value.vt )
	{
	case VT_R4:
		tValue = value.fltVal;
		break;
	default:
		return false;
	}
	return true;
}

template <> inline bool TToVariant<float>(float& tValue, _variant_t& value)
{
	value.vt = VT_R4;
	value.fltVal = tValue;
	return true;
}

// 特化double
template <> inline bool VariantToT<double>(_variant_t& value, double& tValue)
{
	switch( value.vt )
	{
	case VT_R8:
		tValue = value.dblVal;
		break;
	case VT_DECIMAL:
		{
			tValue = value.decVal.Lo32;
			tValue *= (value.decVal.sign == 128)? -1 : 1;
			tValue /= pow(10.0, (int)value.decVal.scale); 
		}
		break;
	default:
		{
			float v = 0.f;
			bool b = VariantToT<float>(value, v);
			if( b )
				tValue = v;
			return b;
		}
	}
	return true;
}

template <> inline bool TToVariant<double>(double& value, _variant_t& tValue)
{
	tValue.vt = VT_R8;
	tValue.dblVal = value;
	return true;
}

// 特化时间类型
template <> inline bool VariantToT<TimeHolder>(_variant_t& value, TimeHolder& tValue)
{
	switch( value.vt )
	{
	case VT_DATE:
		{
			SYSTEMTIME st = { 0 };
			if( !VariantTimeToSystemTime(value.date, &st) )
				return false;
			struct tm tmDest = { 0 };
			tmDest.tm_sec = st.wSecond;
			tmDest.tm_min = st.wMinute;
			tmDest.tm_hour = st.wHour;
			tmDest.tm_mday = st.wDay;
			tmDest.tm_mon = st.wMonth - 1;
			tmDest.tm_year = st.wYear - 1900;
			tmDest.tm_wday = st.wDayOfWeek;
			tmDest.tm_isdst = -1;   // Force DST checking

			if( tmDest.tm_year >= 1101 )
				return false;
			tValue.time = mktime(&tmDest);    // Normalize
			if( tValue.time < 0 )
				tValue.time = 0;
		}
		break;
	case VT_NULL:
	case VT_EMPTY:
		{
			tValue.time = 0;
		}
		break;
	default:
		return false;
	}
	return true;
}

template <> inline bool TToVariant<TimeHolder>(TimeHolder& value, _variant_t& tValue)
{
	tValue.vt = VT_DATE;
	tValue.date = 0;

	SYSTEMTIME st = { 0 };
	struct tm tmp = { 0 };

	if( (signed long long)value.time < 0 )
		value.time = 0;
	if( value.time > ADO_MAX_TIME64_T )
		value.time = ADO_MAX_TIME64_T;

	if( localtime_s(&tmp, &value.time) )
		return false;
	st.wYear = tmp.tm_year+1900;
	st.wMonth = tmp.tm_mon + 1;
	st.wDay = tmp.tm_mday;
	st.wHour = tmp.tm_hour;
	st.wMinute = tmp.tm_min;
	st.wSecond = tmp.tm_sec;
	double t = 0;
	if( !SystemTimeToVariantTime(&st, &t))
		return false;

	tValue.date = t;
	return true;
}

// 特化string
template <> inline bool VariantToT<string>(_variant_t& value, string& tValue)
{
	switch(value.vt) 
	{
	case VT_R4:
		{
			ostringstream ss;
			ss << value.fltVal;
			tValue = ss.str();
		}
		break;
	case VT_R8:
		{
			ostringstream ss;
			ss << value.dblVal;
			tValue = ss.str();
		}
		break;
	case VT_BSTR:
		{
			_bstr_t  bstrTemp(value.bstrVal);
			tValue.assign((char*)bstrTemp);
		}
		break;
	case VT_I1:
		{
			ostringstream ss;
			ss << (int)value.cVal;
			tValue = ss.str();
		}
		break;
	case VT_UI1:
		{
			ostringstream ss;
			ss << (unsigned int)value.bVal;
			tValue = ss.str();
		}
		break;
	case VT_I2:
		{
			ostringstream ss;
			ss<<value.iVal;
			tValue = ss.str();
		}
		break;
	case VT_UI2:
		{
			ostringstream ss;
			ss << value.uiVal;
			tValue = ss.str();
		}
		break;
	case VT_I4:
		{
			ostringstream ss;
			ss << value.lVal;
			tValue = ss.str();
		}
		break;
	case VT_UI4:
		{
			ostringstream ss;
			ss << value.ulVal;
			tValue = ss.str();
		}
		break;
	case VT_INT:
		{
			ostringstream ss;
			ss << value.intVal;
			tValue = ss.str();
		}
		break;
	case VT_UINT:
		{
			ostringstream ss;
			ss << value.uintVal;
			tValue = ss.str();
		}
		break;
	case VT_I8:
		{
			char szBuf[256] = { 0 };
			_snprintf_s(szBuf, 255, _TRUNCATE, "%lld", value.llVal);
			tValue = szBuf;
		}
		break;
	case VT_UI8:
		{
			char szBuf[256] = { 0 };
			_snprintf_s(szBuf, 255, _TRUNCATE, "%llu", value.ullVal);
			tValue = szBuf;
		}
		break;
	case VT_DECIMAL:
		{
			double val = value.decVal.Lo32;
			val *= (value.decVal.sign == 128)? -1 : 1;
			val /= pow((double)10, (int)value.decVal.scale); 
			ostringstream ss;
			ss << val;
			tValue = ss.str();
		}
		break;
	case VT_DATE:
		{
			TimeHolder th;
			if( !VariantToT<TimeHolder>(value, th) )
				return false;

			char szBuf[256] = { 0 };
			struct tm t;
			localtime_s(&t, &th.time);
			if( asctime_s(szBuf, 255, &t) )
				return false;
			tValue = szBuf;
		}
		break;
	case VT_EMPTY:
	case VT_NULL:
		tValue="";
		break;
	case VT_BOOL:
		tValue = (value.boolVal == VARIANT_TRUE? "TRUE" : "FALSE");
		break;
	default:
		tValue = "";
		return false;
	}
	return true;
}

template <> inline bool TToVariant<string>(string& value, _variant_t& tValue)
{
	_bstr_t bst(value.c_str());
	tValue = bst;
	return true;
}

// 特化wstring
template <> inline bool VariantToT<wstring>(_variant_t& value, wstring& tValue)
{
	switch(value.vt) 
	{
	case VT_R4:
		{
			wostringstream ss;
			ss << value.fltVal;
			tValue = ss.str();
		}
		break;
	case VT_R8:
		{
			wostringstream ss;
			ss << value.dblVal;
			tValue = ss.str();
		}
		break;
	case VT_BSTR:
		{
			_bstr_t  bstrTemp(value.bstrVal);
			tValue.assign((wchar_t*)bstrTemp);
		}
		break;
	case VT_I1:
		{
			wostringstream ss;
			ss << (int)value.cVal;
			tValue = ss.str();
		}
		break;
	case VT_UI1:
		{
			wostringstream ss;
			ss << (unsigned int)value.bVal;
			tValue = ss.str();
		}
		break;
	case VT_I2:
		{
			wostringstream ss;
			ss << value.iVal;
			tValue = ss.str();
		}
		break;
	case VT_UI2:
		{
			wostringstream ss;
			ss << value.uiVal;
			tValue = ss.str();
		}
		break;
	case VT_I4:
		{
			wostringstream ss;
			ss << value.lVal;
			tValue = ss.str();
		}
		break;
	case VT_UI4:
		{
			wostringstream ss;
			ss << value.ulVal;
			tValue = ss.str();
		}
		break;
	case VT_INT:
		{
			wostringstream ss;
			ss << value.intVal;
			tValue = ss.str();
		}
		break;
	case VT_UINT:
		{
			wostringstream ss;
			ss << value.uintVal;
			tValue = ss.str();
		}
		break;
	case VT_I8:
		{
			wchar_t szBuf[256] = { 0 };
			wsprintfW(szBuf, L"%lld", value.llVal);
			tValue = szBuf;
		}
		break;
	case VT_UI8:
		{
			wchar_t szBuf[256] = { 0 };
			wsprintfW(szBuf, L"%llu", value.ullVal);
			tValue = szBuf;
		}
		break;
	case VT_DECIMAL:
		{
			double val = value.decVal.Lo32;
			val *= (value.decVal.sign == 128)? -1 : 1;
			val /= pow((double)10, (int)value.decVal.scale); 
			wostringstream ss;
			ss << val;
			tValue = ss.str();
		}
		break;
	case VT_DATE:
		{
			TimeHolder th;
			if( !VariantToT<TimeHolder>(value, th) )
				return false;

			wchar_t wszBuf[256] = { 0 };
			struct tm t;
			localtime_s(&t, &th.time);
			if( _wasctime_s(wszBuf, 255, &t) )
				return false;
			tValue = wszBuf;
		}
		break;
	case VT_EMPTY:
	case VT_NULL:
		tValue= L"";
		break;
	case VT_BOOL:
		tValue = (value.boolVal == VARIANT_TRUE? L"TRUE" : L"FALSE");
		break;
	default:
		tValue=L"";
		return false;
	}
	return true;
}

template <> inline bool TToVariant<wstring>(wstring& value, _variant_t& tValue)
{
	_bstr_t bst(value.c_str());
	tValue = bst;
	return true;
}

// _variant_t 和 binary 互转
inline _variant_t MakeBinary(char* pData, int nDataLen)
{
	_variant_t vtVal;
	
	vtVal.vt = VT_NULL;
	if (pData != NULL && nDataLen > 0)
	{
		do 
		{
			long lngOffset = 0;
			unsigned char chData;
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
			vtVal.vt = VT_ARRAY|VT_UI1;
			vtVal.parray = psa;

		} while (false);
	}

	return vtVal;
}

inline int GetBinary(_variant_t& vtVal, char* pBuffer, int nBufferLen)
{
	if( pBuffer == NULL || nBufferLen <= 0 )
		return -1;

	int bytes = 0;
	if( vtVal.vt == (VT_ARRAY | VT_UI1) )
	{
		char *pBuf = NULL;
		SafeArrayAccessData(vtVal.parray,(void **)&pBuf); 
		if( vtVal.parray->cbElements != 1 )
		{
			SafeArrayUnaccessData (vtVal.parray);
			return -1; 
		}
		bytes = (int)vtVal.parray->rgsabound[0].cElements;

		memcpy(pBuffer, pBuf, min(bytes, nBufferLen));

		SafeArrayUnaccessData (vtVal.parray);
		return bytes;
	}
	return 0;
}

WT_END