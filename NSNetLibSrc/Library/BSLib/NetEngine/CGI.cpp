#include "stdafx.h"
#include "CGI.h"
#include <shlwapi.h>
#include <WiniNet.h>

namespace cgi {

#define MAX_BUFFER_LENGTH	1024
	
StringBuffer& StringBuffer::operator << (const wchar_t* szValue)
{
	m_strBuffer += std::wstring(szValue);
	return *this;
}

StringBuffer& StringBuffer::operator << (const int iValue)
{
	wchar_t szTemp[MAX_PATH] = {0, };
	_itow_s(iValue, szTemp, 10);

	m_strBuffer += std::wstring(szTemp);
	return *this;
}

StringBuffer& StringBuffer::operator << (const long long llValue)
{
	wchar_t szTemp[MAX_PATH] = {0, };
	_i64tow_s(llValue, szTemp, MAX_PATH, 10);

	m_strBuffer += std::wstring(szTemp);
	return *this;
}

StringBuffer& StringBuffer::operator << (const unsigned short usValue)
{
	wchar_t szTemp[MAX_PATH] = {0, };
	_itow_s(usValue, szTemp, 10);

	m_strBuffer += std::wstring(szTemp);
	return *this;
}

StringBuffer& StringBuffer::operator >> (int& iValue)
{
	std::wstring strBuffer;	
	if (false == IsSplitStringEmpty(strBuffer))
	{
		iValue = _wtoi(strBuffer.c_str());
	}

	return *this;
}

StringBuffer& StringBuffer::operator >> (unsigned int& uiValue)
{
	std::wstring strBuffer;
	if (false == IsSplitStringEmpty(strBuffer))
	{
		uiValue = _wtoi(strBuffer.c_str());
	}

	return *this;
}

StringBuffer& StringBuffer::operator >> (long& lValue)
{
	std::wstring strBuffer;
	if (false == IsSplitStringEmpty(strBuffer))
	{
		lValue = _wtol(strBuffer.c_str());
	}

	return *this;
}

StringBuffer& StringBuffer::operator >> (unsigned long& ulValue)
{
	std::wstring strBuffer;
	if (false == IsSplitStringEmpty(strBuffer))
	{
		ulValue = _wtol(strBuffer.c_str());
	}

	return *this;
}

StringBuffer& StringBuffer::operator >> (float& fValue)
{
	std::wstring strBuffer;
	if (false == IsSplitStringEmpty(strBuffer))
	{
		fValue = static_cast<float>(_wtof(strBuffer.c_str()));
	}

	return *this;
}

StringBuffer& StringBuffer::operator >> (std::wstring& strValue)
{
	std::wstring strBuffer;
	if (false == IsSplitStringEmpty(strBuffer))
	{
		strValue = strBuffer;
	}

	return *this;
}

StringBuffer& StringBuffer::operator >> (wchar_t& cValue)
{
	std::wstring strBuffer;
	if (false == IsSplitStringEmpty(strBuffer))
	{
		if (1 >= strBuffer.size())
		{
			cValue = strBuffer.c_str()[0];
		}
	}

	return *this;
}

unsigned long __stdcall URLEscape(const wchar_t* szURL, wchar_t* szEscaped)
{
	unsigned long ulEscapedSize = MAX_BUFFER_LENGTH - 1;

	HRESULT hr = UrlEscape(szURL, szEscaped, &ulEscapedSize, URL_ESCAPE_SEGMENT_ONLY);

	if (FAILED(hr))
	{
		return 0;
	}

	return ulEscapedSize;
}

bool __stdcall GetResultString(const wchar_t* lpszURL, wchar_t* szResultBuf, DWORD dwBufSize)
{
	if (1 > ::lstrlen(lpszURL) || NULL == szResultBuf || 0 == dwBufSize)
	{		
		return false;
	}	

	HINTERNET hInternet = InternetOpen(L"CGI", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);

	if (NULL == hInternet)
	{				
		return false;
	}

	DWORD dwFlags = INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_RELOAD;

	if (0 != wcsstr(lpszURL, L"https"))
	{
		dwFlags |= INTERNET_FLAG_SECURE;
	}

	HINTERNET hURL = InternetOpenUrl(hInternet, lpszURL, NULL, 0, dwFlags, 0);

	if (NULL == hURL)
	{
		InternetCloseHandle(hInternet);		
		return false;
	}

	DWORD dwAvailableSize = 0;
	DWORD dwRead = 0;
	DWORD dwReadOffset = 0;

	do
	{
		if (InternetQueryDataAvailable(hURL, &dwAvailableSize, 0, 0) == FALSE)
			return false;

		if (dwAvailableSize == 0 || dwReadOffset == dwBufSize)
			break;

		if (dwAvailableSize > dwBufSize)
			dwAvailableSize = dwBufSize;

		if (dwReadOffset + dwAvailableSize > dwBufSize)
		{
			InternetCloseHandle(hURL);
			InternetCloseHandle(hInternet);

			return false;
		}

		InternetReadFile(hURL, szResultBuf + dwReadOffset, dwAvailableSize, &dwRead);

		dwReadOffset += dwRead;

	} while (true);

	InternetCloseHandle(hURL);
	InternetCloseHandle(hInternet);

	if (dwBufSize > dwReadOffset)
		szResultBuf[dwReadOffset] = '\0';	

	return true;
}

bool __stdcall Execute(const wchar_t* szURL, const wchar_t* szValues, StringBuffer& stringBuffer)
{
	wchar_t szTempBuf[MAX_BUFFER_LENGTH] = {0, };
	wchar_t szCGIRUL[MAX_BUFFER_LENGTH] = {0, };

	// Parameter 에 해당하는 부분을 'Escape' 적용하여 16 진수 값으로 인코딩
	if (S_OK != StringCbPrintf(szTempBuf, MAX_BUFFER_LENGTH - 1, L"%s", szValues))
	{
		return false;
	}

	/*
	wchar_t szEscaped[MAX_BUFFER_LENGTH] = {0, };
	if (0 == URLEscape(szTempBuf, szEscaped))
	{		
		return false;
	}
	*/

	// CGI 호출
	if (S_OK != StringCbPrintf(szCGIRUL, MAX_BUFFER_LENGTH - 1, L"%s?%s", szURL, szTempBuf))
	{
		return false;
	}	

	//unsigned long ulPrevTickCount = 0;
	//unsigned long ulCurrnetTickCount = 0;

	//ulPrevTickCount = GetTickCount();

	if (false == GetResultString(szCGIRUL, szTempBuf, (MAX_BUFFER_LENGTH - 1)))
	{
		return false;
	}

	//ulCurrnetTickCount = GetTickCount();

	stringBuffer << szTempBuf;

	return true;
}

} // namespace cgi