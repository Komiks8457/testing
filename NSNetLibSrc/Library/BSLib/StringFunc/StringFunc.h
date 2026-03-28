#ifndef __STRING_FUNC_H__
#define __STRING_FUNC_H__

#pragma once

#include <strsafe.h> // 안전한 스트링 함수들을 쓰기 위해서
#include <algorithm>
#include "UTF8Conv.h"

#define STRING_CONVERT_BUFFER_SIZE 1024

#define SF_sprintf		StringCchPrintf
#define SF_vsprintf		StringCchVPrintf
#define SF_strcat		StringCchCat
#define SF_strcpy		StringCchCopy
#define SF_wsprintf		StringCchPrintf
#define SF_lstrcpy		StringCchCopy
#define SF__tcscpy      StringCchCopy
#define SF__tcscat      StringCchCat
#define SF_strlen       StringCchLength
#define SF_lstrlen      StringCchLength

/*
inline void SF_strlwr(LPTSTR pszDest, size_t cchDest)
{

	if(0 != _tcslwr_s(pszDest, cchDest))
		RaiseException(STATUS_ACCESS_VIOLATION, 0, 0, 0);
}

inline void SF_strupr(LPTSTR pszDest, size_t cchDest)
{
	if(0 != _tcsupr_s(pszDest, cchDest))
		RaiseException(STATUS_ACCESS_VIOLATION, 0, 0, 0);
}

inline void SF_vsprintf(LPTSTR pszDest, size_t cchDest, LPCTSTR pszFmt, va_list argList)
{
	if( 0 > _vstprintf_s(pszDest, cchDest, pszFmt, argList))
		RaiseException(STATUS_ACCESS_VIOLATION, 0, 0, 0);
}

inline void SF_sprintf(LPTSTR pszDest, size_t cchDest, LPCTSTR pszFmt, ...)
{
	va_list args;
	va_start(args, pszFmt);

	SF_vsprintf(pszDest, cchDest, pszFmt, args);

	va_end(args);
}

inline void SF_strcat(LPTSTR pszDest, size_t cchDest, LPCTSTR pszSrc)
{
	if(0 != _tcscat_s(pszDest, cchDest, pszSrc))
		RaiseException(STATUS_ACCESS_VIOLATION, 0, 0, 0);
}

inline void SF_strcpy(LPTSTR pszDest, size_t cchDest, LPCTSTR pszSrc)
{
	if(0 != _tcscpy_s(pszDest, cchDest, pszSrc))
		RaiseException(STATUS_ACCESS_VIOLATION, 0, 0, 0);
}

inline size_t SF_strlen(LPCTSTR pszSrc, size_t cchMax)
{
	size_t nLen = -1;
	HRESULT hr = StringCchLength(pszSrc, cchMax, &nLen);
	if(!SUCCEEDED(hr));
		RaiseException(STATUS_ACCESS_VIOLATION, 0, 0, 0);

	return nLen;
}
*/

inline int SF_strcmp(LPCTSTR pszString1, LPCTSTR pszString2)
{	
	return (CompareString(LOCALE_USER_DEFAULT, 0, pszString1, lstrlen(pszString1), pszString2, lstrlen(pszString2)) - CSTR_EQUAL);
}

inline int SF_stricmp(LPCTSTR pszString1, LPCTSTR pszString2)
{
	return (CompareString(LOCALE_USER_DEFAULT, NORM_IGNORECASE|NORM_IGNOREKANATYPE, pszString1, lstrlen(pszString1), pszString2, lstrlen(pszString2)) - CSTR_EQUAL);
}

inline int SF_strncmp(LPCTSTR pszString1, LPCTSTR pszString2, DWORD dwLength)
{
	return (CompareString(LOCALE_USER_DEFAULT, 0, pszString1, dwLength, pszString2, dwLength) - CSTR_EQUAL);
}

inline int SF_strnicmp(LPCTSTR pszString1, LPCTSTR pszString2, DWORD dwLength)
{
	return (CompareString(LOCALE_USER_DEFAULT, NORM_IGNORECASE|NORM_IGNOREKANATYPE, pszString1, dwLength, pszString2, dwLength) - CSTR_EQUAL);
}

// by novice.. for Validate Format Strings..
// If the function succeeds, the return value is TRUE.
// If the function fails, the return value is FALSE (if the nOutputLen greater than kMaxBufferLen)
static BOOL ValidateFormatString( LPTSTR szOutput OUT, size_t nOutputLen IN, LPCTSTR szOriginal IN )
{
	const int kMaxBufferLen = 8192;
	LPTSTR pCurPos = (LPTSTR)szOriginal;
	size_t nCurPos = 0;
	TCHAR  szTemp[kMaxBufferLen] = {0,};

	while ( NULL != *pCurPos )
	{
		if ( _T('%') == *(pCurPos) )
		{
			TCHAR* pNextPos = CharNext( pCurPos );

			// be referred from "MSDN - printf Type Field Characters"
			if (	( _T('c') != *pNextPos ) && ( _T('C') != *pNextPos ) &&
					( _T('d') != *pNextPos ) && ( _T('i') != *pNextPos ) &&
					( _T('o') != *pNextPos ) && ( _T('u') != *pNextPos ) &&
					( _T('x') != *pNextPos ) && ( _T('X') != *pNextPos ) &&
					( _T('e') != *pNextPos ) && ( _T('E') != *pNextPos ) &&
					( _T('f') != *pNextPos ) && ( _T('g') != *pNextPos ) &&
					( _T('G') != *pNextPos ) && ( _T('a') != *pNextPos ) &&
					( _T('A') != *pNextPos ) && ( _T('n') != *pNextPos ) &&
					( _T('p') != *pNextPos ) && ( _T('s') != *pNextPos ) &&
					( _T('S') != *pNextPos ) && ( _T('%') != *pNextPos ) &&
					(( _T('0') >  *pNextPos ) || ( _T('9') <  *pNextPos ))	&&	
					(( _T('I') != *pNextPos ) || ( _T('6') != *(pNextPos+1) ) &&
					 ( _T('4') != *(pNextPos+2) ) || ( _T('d') != *(pNextPos+3) )) )
			{
				// Type Filed Character도 아니라면.. %를 두개를 썼어야지..
				szTemp[nCurPos++] = _T('%');
			}
			else
			{
				// Type Filed Character는 패스~!!
				szTemp[nCurPos++] = *(pCurPos);
				szTemp[nCurPos++] = *(pNextPos);

				pCurPos =  CharNext( pNextPos );

				continue;
			}
		}

		size_t Offset = CharNext( pCurPos ) - pCurPos;

		CopyMemory( &szTemp[nCurPos], pCurPos, Offset );

		pCurPos =  CharNext( pCurPos );
		nCurPos += Offset;
	}

	return ( nullptr != lstrcpyn( szOutput, szTemp, (int)nOutputLen ) );
}

inline void ConvertString( IN OUT TCHAR* szString, IN BYTE byBit )
{
	for ( int nIdx = 0; nIdx < lstrlen(szString); nIdx++ )
	{
		*(szString + nIdx) ^= byBit;
	}
}

inline std::wstring ReplaceString(const std::wstring &srcStr, const std::wstring &oldStr, const std::wstring &newStr)
{
	if (srcStr.size() <= 0 || oldStr.size() <= 0)
	{
		return srcStr;
	}
	std::wstring strReturn = srcStr;
	std::wstring::size_type offset = 0;
	std::wstring::size_type start = strReturn.find(oldStr);
	while (start != std::wstring::npos)
	{
		offset = start + newStr.size();
		strReturn.replace(start, oldStr.size(), newStr);
		start = strReturn.find(oldStr, offset);
	}

	return strReturn;
}

static std::string FormatStringArgs(const char *fmt, va_list args)
{
	if (!fmt)
		return "";

	int result = -1, length = 1024;
	char *buffer = 0;
	while (result == -1) 
	{
		if (buffer)
			delete [] buffer;

		buffer = new char [length + 1];
		memset(buffer, 0, length + 1);
		result = _vsnprintf_s(buffer, length, _TRUNCATE, fmt, args);
		length *= 2;
	}

	std::string s(buffer);
	delete [] buffer;
	return s;
}

static std::string FormatString(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	std::string s = FormatStringArgs(fmt, args);
	va_end(args);
	return s;
}

static BOOL ConversionANSI(const std::wstring& wstrOrg, OUT std::string& strConverted)
{
    char szTemp[8192] = {0,};

    int nLength = WideCharToMultiByte(CP_ACP, 0, wstrOrg.c_str(), static_cast<int>(wstrOrg.length()), szTemp, sizeof(szTemp), NULL, NULL);

    if ( nLength <= 0 )
    {
        return FALSE;
    }

    strConverted.assign( szTemp );

    return TRUE;
}

static BOOL ConversionUnicode(const std::string& strOrg, OUT std::wstring& wstrConverted)
{
    wchar_t wszTemp[8192] = {0,};

    int nLength = MultiByteToWideChar(CP_ACP, 0, strOrg.c_str(), static_cast<int>(strOrg.length()), wszTemp, _countof(wszTemp));

    if(nLength <= 0)
    {
        return FALSE;
    }

    wstrConverted.assign( wszTemp );

    return TRUE;
}

static BOOL ConversionTrimUnicode(std::string& strOrg, OUT std::wstring& wstrConverted)
{
	strOrg.erase(std::remove_if(strOrg.begin(), strOrg.end(), ::isspace ), strOrg.end());
	return ConversionUnicode(strOrg, wstrConverted);
}

static void StringSplit(const std::string& s, char delim, OUT std::vector<std::string>& elems)
{
	std::stringstream ss(s);
	std::string e;

	while (std::getline(ss, e, delim))
		elems.push_back(e);
}

static void StringSplit(const std::wstring& s, wchar_t delim, OUT std::vector<std::wstring>& elems)
{
    std::wstringstream ss(s);
    std::wstring e;

    while (std::getline(ss, e, delim))
        elems.push_back(e);
}

#endif