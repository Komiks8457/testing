#pragma once

#define MAX_MEM_BUFFER 256

class CSharedMemory
{
public:
	CSharedMemory();
	virtual ~CSharedMemory();

public:
	BOOL				OpenSharedMemory(std::tstring strShareName);

private:

	char				m_szBuffer[MAX_MEM_BUFFER];
	DWORD				m_dwWrCur;

	HANDLE				m_hMemoryMap;
	LPBYTE				m_pMemoryMap;


public:
	void				ResetMemory();
	void				WriteBytes(void* pBuf, WORD nLen);
	void				FlushMemory();



public:
	template <class T>
	void operator << (T arg)
	{
		WriteBytes(&arg, sizeof(T));
	}

	/*
	CFStream& operator << (const char* lpszString)
	{
		int nLen = lstrlenA(lpszString);
		WriteBytes(&nLen, sizeof(int));
	}
	*/
};

