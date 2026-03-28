#pragma once

class CFStream
{
public:
	CFStream();
	virtual ~CFStream();

public:
	template <class T>
	CFStream& operator << (T arg)
	{
		WriteBytes(&arg, sizeof(T));
		return *this;
	}

	template <class T>
	CFStream& operator >> (T& arg)
	{
		ReadBytes(&arg, sizeof(T));
		return *this;
	}

	CFStream& operator << (const char* lpszString)
	{
		int nLen = lstrlenA(lpszString);
		WriteBytes(&nLen, sizeof(int));

		if (nLen < 1)
			return *this;

		WriteBytes((void*)lpszString, nLen + 1);	// NULL문자 넣기위해 +1 했다.
		return *this;
	}

	CFStream& operator << (std::string& strString)
	{
		(*this) << strString.c_str();
		return *this;
	}

	CFStream& operator >> (std::string& strString)
	{
		int nLen = 0;
		*this >> nLen;
		if (nLen > 0)
		{
			char* pBuf = new char[nLen + 1];
			ReadBytes(pBuf, nLen + 1);
			strString = pBuf;
			delete [] pBuf;
		}
		
		return *this;
	}

	// FOR UNICODE STRING!
	CFStream& operator << (const WCHAR* lpszString)
	{
		int nLen = lstrlenW(lpszString);
		WriteBytes(&nLen, sizeof(int));

		if (nLen < 1)
			return *this;

		WriteBytes((void*)lpszString, sizeof(WCHAR) * (nLen + 1));	// NULL문자 넣기위해 +1 했다.
		return *this;
	}

	CFStream& operator << (std::wstring& strString)
	{
		(*this) << strString.c_str();
		return *this;
	}

	CFStream& operator >> (std::wstring& strString)
	{
		int nLen = 0;
		*this >> nLen;
		if (nLen > 0)
		{
			WCHAR* pBuf = new WCHAR[nLen + 1];
			ReadBytes(pBuf, sizeof(WCHAR) * (nLen + 1));
			strString = pBuf;
			delete [] pBuf;
		}

		return *this;
	}


	BOOL	Open(LPCTSTR lpszFile, BOOL bForWrite);
	long	GetCurrentPos();
	BOOL	Goto(int nOffset);
	void	Close();
	BOOL	Rewind();
	LPCTSTR GetFileName() { return &m_strFileName[0]; }
	DWORD	GetFileSize() { return m_dwFileSize; }
		
	void	ReadBytes(void* pBuf, int nLen);
	void	WriteBytes(void* pBuf, int nLen);
		    		
protected:
	FILE*	m_pStream;
	TCHAR	m_strFileName[256];

	DWORD	m_dwStartPos;
	DWORD	m_dwEndPos;
	DWORD	m_dwFileSize;
		
	DWORD	m_dwRdCur;			// Header사이즈를 포함한 Offset
	DWORD	m_dwWrCur;			// Header사이즈를 포함한 Offset
};
