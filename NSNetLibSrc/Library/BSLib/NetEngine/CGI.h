#pragma once

namespace cgi {

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "shlwapi.lib")

class StringBuffer
{
public:
	StringBuffer& operator << (const wchar_t* szValue);
	StringBuffer& operator << (const int iValue);
	StringBuffer& operator << (const long long llValue);
	StringBuffer& operator << (const unsigned short usValue);

	StringBuffer& operator >> (std::wstring& strValue);
	StringBuffer& operator >> (int& iValue);
	StringBuffer& operator >> (unsigned int& uiValue);
	StringBuffer& operator >> (long& lValue);
	StringBuffer& operator >> (unsigned long& ulValue);
	StringBuffer& operator >> (wchar_t& cValue);
	StringBuffer& operator >> (float& fValue);

	const wchar_t* GetString() const { return m_strBuffer.c_str(); }

	void Clear() { m_strBuffer.clear(); }

private:
	bool IsSplitStringEmpty(std::wstring& strBuffer)
	{
		if (false == m_strBuffer.empty())
		{
			if (false == SplitString(m_strBuffer, '|', strBuffer, m_strBuffer))
			{
				strBuffer = m_strBuffer;
				m_strBuffer.clear();
			}
		}

		return strBuffer.empty();
	}

	bool SplitString(std::wstring& strSrc, wchar_t szSep, std::wstring& strSub1, std::wstring& strSub2)
	{
		size_t iSepPos = strSrc.find(szSep);

		if (std::string::npos == iSepPos)
		{
			return false;
		}

		strSub1 = strSrc.substr(0, iSepPos);
		strSub2 = strSrc.substr(iSepPos + 1, strSrc.length());

		return true;
	}

	std::wstring m_strBuffer;
};

bool __stdcall Execute(const wchar_t* szURL, const wchar_t* szValues, StringBuffer& stringBuffer);
bool __stdcall GetResultString(const wchar_t* lpszURL, wchar_t* szResultBuf, DWORD dwBufSize);
unsigned long __stdcall URLEscape(const wchar_t* szURL, wchar_t* szEscaped);

} // namespace cgi
