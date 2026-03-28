////////////////////////////////////////////////
// Programmed by 강문철
// Email: overdrv72@hotmail.com
// Description: 음... Memory Mapped File을 이용한
//				간단한 화일 시스템을 구성했다.
//				내용자체는 별로 복잡할 것이 없다.
//				머 MMF관련해서 생소한 API들이
//				몇개 있을 수 있지만 MSDN이 해결해
//				줄테니... 걱정없다. 
//				Application레벨에선
//				CFileSystem의 Instance를 생성하고
//				마치 MFC의 CFile을 사용하듯 
//				CFileLegacy만 사용하면 된다.
////////////////////////////////////////////////
#pragma once

#include "interface.h"

/////////////////////////////////////////////////////////////
// CFileMem
/////////////////////////////////////////////////////////////
class CFileMem : public IFile
{
public:
	CFileMem();
	virtual ~CFileMem();

protected:
	BYTE*	m_pBuffer;

public:
	virtual void	TerminateFile();
	virtual BOOL	Flush();
	virtual long	Open(std::wstring& strFileName, DWORD dwForWrite, DWORD dwID);

	virtual DWORD	_ReadBytes(FileContext* pCtx, BYTE* pDestBuf, DWORD Len);
	virtual DWORD	_WriteBytes(FileContext* pCtx, BYTE* pSrcBuf, DWORD Len);

	virtual BOOL	IsValidFile() { return (m_pBuffer != NULL); }
	virtual HANDLE	GetHandle()   { _ASSERT(FALSE); return NULL; }	// 뭣에 쓰려고? -_-+
	virtual BYTE*	GetFilePtr()  { return m_pBuffer; }
	virtual BYTE*	ChangeBuffer(BYTE* pNewBuf, DWORD dwBufSize) 
	{ 
		BYTE* pOldBuf	= m_pBuffer;
		
		m_pBuffer		= pNewBuf;
		m_dwFileSize	= dwBufSize;

		return pOldBuf;
	}

	virtual FILE_TYPE	GetFileType() { return FILE_TYPE_MEM; }
};

/////////////////////////////////////////////////////////////
// CFileLegacy
/////////////////////////////////////////////////////////////
class CFileLegacy : public IFile
{
public:
	CFileLegacy();
	virtual ~CFileLegacy();

protected:
	HANDLE	m_hFile;
	
protected:
	BOOL	_BeforeAccess(FileContext* pCtx);
	BOOL	_AfterAccess();

public:
	virtual void	TerminateFile();
	virtual BOOL	Flush();
	virtual long	Open(std::wstring& strFileName, DWORD dwForWrite, DWORD dwID);

	virtual DWORD	_ReadBytes(FileContext* pCtx, BYTE* pDestBuf, DWORD Len);
	virtual DWORD	_WriteBytes(FileContext* pCtx, BYTE* pSrcBuf, DWORD Len);

	virtual HANDLE	GetHandle()   { return m_hFile; }
	virtual BYTE*	GetFilePtr()  { return NULL; }

	virtual BOOL		IsValidFile() { return (m_hFile != INVALID_HANDLE_VALUE); }
	virtual FILE_TYPE	GetFileType() { return FILE_TYPE_LEGACY; }
};

/////////////////////////////////////////////////////////////
// CFileMM
/////////////////////////////////////////////////////////////

class CFileMM : public IFile
{
public:
	CFileMM();
	virtual ~CFileMM();

protected:
	HANDLE  m_hFile;
	HANDLE	m_hMMFile;
	BYTE*	m_lpBase;

public:
	virtual void	TerminateFile();
	virtual BOOL	Flush();
	virtual long	Open(std::wstring& strFileName, DWORD dwForWrite, DWORD dwID);

	virtual DWORD	_ReadBytes(FileContext* pCtx, BYTE* pDestBuf, DWORD Len);
	virtual DWORD	_WriteBytes(FileContext* pCtx, BYTE* pSrcBuf, DWORD Len);

	virtual HANDLE	GetHandle()   { return m_hFile; }
	virtual BYTE*	GetFilePtr()  { return m_lpBase; }

	virtual BOOL	IsValidFile() { return (m_hFile != INVALID_HANDLE_VALUE && m_hMMFile != NULL); }

	virtual FILE_TYPE	GetFileType() { return FILE_TYPE_MEMORYMAPPED; }
};

/////////////////////////////////////////////////////////////
// CFileSystem
/////////////////////////////////////////////////////////////
#define DEFAULT_MMCACHE_SIZE		size_t(10000000)		// 대략 10M
#define CACHE_REFRESH_FILLFACTOR	0.8f				// 약 80정도 cache가 사용되기 시작하면... 캐쉬 청소 들어가자...

typedef std::list<DWORD>			FILEHANDLES;

typedef HASH_MAP<std::wstring, DWORD>	NAME_LOOKUP;
typedef HASH_MAP<DWORD, IFile*>		FILES_BY_ID;

class CFileSystem : public Singleton<CFileSystem>
{	
public:
	CFileSystem();
	virtual ~CFileSystem();

private:
	CCriticalSectionBS	m_CS;

	// in use
	FILES_BY_ID		m_Files;
	NAME_LOOKUP		m_FileLookup;

	size_t			m_MaxCacheSize;
	size_t			m_CacheUsage;

	size_t			m_nFileCountInUse;			

	// 캐쉬 히트율 계산용...
	size_t			m_OpenTry;
	size_t			m_CacheHit;

	CIDPool			m_IDPool;

	BOOL			m_bShareFile;
	
protected:
	DWORD	_AddNewFileEntry(IFile* pEntry);
	DWORD	_DeleteIdleFile();
	IFile*	_CreateFile(std::wstring& strFileName, DWORD& dwFileHandle, FILE_TYPE file_type);
	void	_RemoveFile(IFile* pFile);
	
public:
	void	Cleanup();
	BOOL	Create(size_t CacheSize, BOOL bShareFile);
	long	OpenFile(FILE_TYPE file_type, std::wstring& strFileName, DWORD dwForWrite, DWORD dwID, DWORD& dwFileHandle);
	size_t	CloseFile(DWORD dwFileHandle, DWORD dwID);
	IFile*	GetFile(DWORD dwFileHandle);
	void	GetStatistic(size_t& OpenFileCount, size_t& FileCountInUse, size_t& MaxCacheSize, size_t& CacheUsage, size_t& CacheHitPercentage)
	{
		SCOPED_LOCK_SINGLE(&m_CS);

		OpenFileCount	= m_Files.size();
		FileCountInUse	= m_nFileCountInUse;
		
		MaxCacheSize  = m_MaxCacheSize;
		CacheUsage	= m_CacheUsage;

		if (m_OpenTry == 0)
			CacheHitPercentage = 0;
		else
			CacheHitPercentage	= m_CacheHit * 100 / m_OpenTry;
	}
};

#define FILESYSTEM CFileSystem::GetSingleton()
