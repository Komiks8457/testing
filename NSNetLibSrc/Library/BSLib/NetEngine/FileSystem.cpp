 #include "stdafx.h"
#include "filesystem.h"

/////////////////////////////////////////////////////////////
// IFile
/////////////////////////////////////////////////////////////
void IFile::TerminateFile()
{
	SCOPED_LOCK_SINGLE(&m_CS);

	FILECTX::iterator it = m_Contexts.begin();
	FILECTX::iterator it_end = m_Contexts.end();

	for (; it != it_end; ++it)
		delete (*it).second;
	m_Contexts.clear();

	m_strFileName.clear();

	m_dwFileHandle	= NULL;
	m_dwFileSize	= 0;
	m_dwHitCount	= 0;
	m_OpenMode		= OPEN_MODE_UNKNOWN;
	m_FileType		= FILE_TYPE_INVALID;
}

size_t IFile::Close(DWORD dwID)
{
	SCOPED_LOCK_SINGLE(&m_CS);
	
	return EraseContext(dwID);
}

DWORD IFile::GetRemainBytes(FileContext* pCtx)
{
	SCOPED_LOCK_SINGLE(&m_CS);

	if (m_dwFileSize <= pCtx->dwAccessOffset)
		return 0;

	return (m_dwFileSize - pCtx->dwAccessOffset);
}

long IFile::AddContext(DWORD dwClientID)
{
	SCOPED_LOCK_SINGLE(&m_CS);

	FILECTX::iterator it = m_Contexts.find(dwClientID);
	if (it != m_Contexts.end())
		return FSERR_ALREADY_OPEN;

	FileContext* pCtx = new FileContext;
	pCtx->SetAccessOffset(0);

	m_Contexts.insert(FILECTX::value_type(dwClientID, pCtx));

	++m_dwHitCount;	// caching 용 정보로 쓰일까 해서... 

	return FSERR_GOOD;
}

FileContext* IFile::GetContext(DWORD dwClientID)
{
	SCOPED_LOCK_SINGLE(&m_CS);

	FILECTX::iterator it = m_Contexts.find(dwClientID);
	if (it != m_Contexts.end())
		return (*it).second;

	return NULL;
}

long IFile::GetCurFileOffset(DWORD dwClientID)
{
	SCOPED_LOCK_SINGLE(&m_CS);

	FileContext* pCtx = GetContext(dwClientID);
	if (pCtx == NULL)
		return 0;

	return pCtx->dwAccessOffset;
}

size_t IFile::EraseContext(DWORD dwClientID)
{
	SCOPED_LOCK_SINGLE(&m_CS);

	FILECTX::iterator it = m_Contexts.find(dwClientID);
	if (it != m_Contexts.end())
	{
		delete (*it).second;
		m_Contexts.erase(it);
	}
	
	return m_Contexts.size();
}

long IFile::Open(std::wstring& strFileName, DWORD dwForWrite, DWORD dwClientID)
{
	SCOPED_LOCK_SINGLE(&m_CS);

	_ASSERT(strFileName.length() > 0);

	IFile::OPEN_MODE open_mode = (dwForWrite > 0) ? IFile::OPEN_MODE_WRITE : IFile::OPEN_MODE_READ;
	
	// 이미 열려있는 화일이다.
	if (IsValidFile() == TRUE)
	{
		// write mode로 열린 화일은 중복해서 접근할 수 없다. exclusive mode 이다!
		if (GetOpenMode() == IFile::OPEN_MODE_WRITE)
		{
			if (dwClientID != FS_RESERVED_SYSTEM_SESSION_ID)
				return FSERR_CANT_ACCESS_MULTIUSER;
		}

		// 기존에 열린 mode 와 반드시 같은 mode로 open해야 한다. (사실 한마디로 Read Access의 경우만 복수 접근이 가능하다)
		if (GetOpenMode() != open_mode)
			return FSERR_OPENMODE_CONFLICED;

		if (GetContext(dwClientID) != NULL)
			return FSERR_ALREADY_OPEN;

		return FSERR_GOOD;
	}
		
	SetOpenMode(open_mode);
	::StrLwr(m_strFileName, strFileName.c_str());
	
	return FSERR_GOOD;
}

DWORD IFile::Read(DWORD dwID, LPBYTE lpBuffer, long nBytesToRead)
{
	SCOPED_LOCK_SINGLE(&m_CS);

	if (nBytesToRead <= 0)
		return 0;

	FileContext* pFileCtx = GetContext(dwID);
	if (pFileCtx == NULL)
	{
		PutLog(LOG_FATAL, _T("IFile::Read() - User Context 얻기 실패!"));
		return 0;
	}

	DWORD dwRemain = GetRemainBytes(pFileCtx);
	if (dwRemain == 0)
	{
		PutLog(LOG_FATAL, _T("IFile::Read() - RemainBytes == 0 (요청한 바이트 수: %d)"), nBytesToRead);
		return 0;
	}

	DWORD dwActualRead = (dwRemain >= (DWORD)nBytesToRead) ? nBytesToRead : dwRemain;
	
	dwActualRead = _ReadBytes(pFileCtx, lpBuffer, dwActualRead);
	
	pFileCtx->MoveAccessOffset(dwActualRead);

	return dwActualRead;
}

DWORD IFile::Write(DWORD dwID, BYTE* lpBuffer, long nBytesToWrite)
{
	SCOPED_LOCK_SINGLE(&m_CS);

	if (nBytesToWrite <= 0)
		return 0;

	FileContext* pFileCtx = GetContext(dwID);
	if (pFileCtx == NULL)
	{
		PutLog(LOG_FATAL, _T("IFile::Write() - User Context 얻기 실패!"));
		return 0;
	}

	DWORD dwRemain = GetRemainBytes(pFileCtx);
	if (dwRemain == 0)
	{
		PutLog(LOG_FATAL, _T("IFile::Read() - RemainBytes == 0 (요청한 바이트 수: %d)"), nBytesToWrite);
		return 0;
	}

	if ((DWORD)nBytesToWrite > dwRemain)
		nBytesToWrite = dwRemain;

	DWORD dwActualWrite = _WriteBytes(pFileCtx, lpBuffer, nBytesToWrite);
	pFileCtx->MoveAccessOffset(dwActualWrite);

	return dwActualWrite;
}

long IFile::Seek(DWORD dwID, long Offset, DWORD dwFrom)
{
	SCOPED_LOCK_SINGLE(&m_CS);

	FileContext* pFileCtx = GetContext(dwID);
	if (pFileCtx == NULL)
	{
		PutLog(LOG_FATAL, _T("IFile::Seek() - User Context 얻기 실패!"));
		return FSERR_MUST_OPEN;
	}

	if (Offset == 0)
		return FSERR_GOOD;

	DWORD dwBasePos = GetFileAccessPos(dwFrom, pFileCtx);
	
	long available_bytes = (Offset >= 0) ? (m_dwFileSize - dwBasePos) : pFileCtx->dwAccessOffset;

	if (::abs(Offset) > available_bytes)
	{
		PutLog(LOG_FATAL, _T("IFile::Seek() - 잘못된 Offset 지정했다! [Offset: %d, Available: %d]"), Offset, available_bytes);
		return FSERR_INVALID_PARAM;
	}
	
	dwBasePos += Offset;

	pFileCtx->SetAccessOffset(dwBasePos);
	
	return FSERR_GOOD;
}

/////////////////////////////////////////////////////////////
// CFileMem
/////////////////////////////////////////////////////////////
CFileMem::CFileMem()
{
	m_pBuffer	= NULL;
	m_FileType	= FILE_TYPE_MEM;
}

CFileMem::~CFileMem()
{
	TerminateFile();
}

void CFileMem::TerminateFile()
{
	IFile::TerminateFile();

	if (m_pBuffer != NULL)
	{
		delete [] m_pBuffer;
		m_pBuffer = NULL;
	}
}

BOOL CFileMem::Flush()
{
	// nothing to do with...
	return TRUE;
}

long CFileMem::Open(std::wstring& strFileName, DWORD dwForWrite, DWORD dwClientID)
{
	SCOPED_LOCK_SINGLE(&m_CS);

	long rval = IFile::Open(strFileName, dwForWrite, dwClientID);
	if (rval != FSERR_GOOD)
		return rval;

	if (dwClientID == FS_RESERVED_SYSTEM_SESSION_ID)
	{
		_ASSERT(m_dwFileSize == dwForWrite);
		_ASSERT(m_pBuffer != NULL);
	}
	else
	{
		_ASSERT(GetOpenMode() != IFile::OPEN_MODE_UNKNOWN);
		
		m_dwFileSize = dwForWrite;

		_ASSERT(m_pBuffer == NULL);

		m_pBuffer = new BYTE[m_dwFileSize];
	}

	return AddContext(dwClientID);
}

DWORD CFileMem::_ReadBytes(FileContext* pCtx, BYTE* lpBuffer, DWORD dwBytesToRead)
{
	if (m_pBuffer == NULL)
	{
		return 0;
	}
	
	if (pCtx->dwAccessOffset + dwBytesToRead > GetFileSize())
		dwBytesToRead = GetFileSize() - pCtx->dwAccessOffset;

	::memcpy(lpBuffer, m_pBuffer + pCtx->dwAccessOffset, dwBytesToRead);
	return dwBytesToRead;
}

DWORD CFileMem::_WriteBytes(FileContext* pCtx, BYTE* lpBuffer, DWORD dwBytesToWrite)
{
	if (m_pBuffer == NULL)
	{
		return 0;
	}

	if (pCtx->dwAccessOffset + dwBytesToWrite > GetFileSize())
		dwBytesToWrite = GetFileSize() - pCtx->dwAccessOffset;

	::memcpy(m_pBuffer + pCtx->dwAccessOffset, lpBuffer, dwBytesToWrite);
	return dwBytesToWrite;
}

/////////////////////////////////////////////////////
// CFileLegacy
/////////////////////////////////////////////////////
CFileLegacy::CFileLegacy()
{
	m_hFile		= INVALID_HANDLE_VALUE;
	m_FileType	= FILE_TYPE_LEGACY;
}

CFileLegacy::~CFileLegacy()
{
	TerminateFile();
}

void CFileLegacy::TerminateFile()
{
	SCOPED_LOCK_SINGLE(&m_CS);

	Flush();

	if (m_hFile != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(m_hFile);
		m_hFile = INVALID_HANDLE_VALUE;
	}

	IFile::TerminateFile();
}

BOOL CFileLegacy::Flush()
{
	SCOPED_LOCK_SINGLE(&m_CS);

	if (IsValidFile() == TRUE && GetOpenMode() == IFile::OPEN_MODE_WRITE)
		return FlushFileBuffers(m_hFile);

	return TRUE;
}

long CFileLegacy::Open(std::wstring& strFileName, DWORD dwForWrite, DWORD dwClientID)
{
	SCOPED_LOCK_SINGLE(&m_CS);

	int rval = IFile::Open(strFileName, dwForWrite, dwClientID);
	if (rval != FSERR_GOOD)		// open 실패! 
		return rval;

	if (IsValidFile() == FALSE)	// open된 화일이면 IFile::Open()에서 context만 추가하고 걍 리턴!
	{	
		DWORD dwAccess, dwCreationFlag, dwFlagsAndAttr;
		if (GetOpenMode() == IFile::OPEN_MODE_WRITE)
		{
			dwAccess		= GENERIC_WRITE;
			dwCreationFlag	= CREATE_ALWAYS;
			dwFlagsAndAttr	= FILE_ATTRIBUTE_NORMAL;
		}
		else
		{
			dwAccess		= GENERIC_READ;
			dwCreationFlag	= OPEN_EXISTING;
			//dwFlagsAndAttr	= FILE_ATTRIBUTE_NORMAL;
			dwFlagsAndAttr	= FILE_FLAG_SEQUENTIAL_SCAN;
		}
		
		m_hFile = ::CreateFile(strFileName.c_str(), dwAccess, FILE_SHARE_READ, NULL, dwCreationFlag, dwFlagsAndAttr, NULL);
		if (m_hFile == INVALID_HANDLE_VALUE)
			return FSERR_CANT_OPEN_FILE;
		
		if (GetOpenMode() == IFile::OPEN_MODE_WRITE)
		{
			m_dwFileSize = dwForWrite;	// Write용일 경우는 화일 사이즈를 담고있다
			
			::SetFilePointer(m_hFile, m_dwFileSize, NULL, FILE_BEGIN);
			::SetEndOfFile(m_hFile);
			::SetFilePointer(m_hFile, 0, NULL, FILE_BEGIN);
		}
		else
		{
			m_dwFileSize = ::GetFileSize(m_hFile, NULL);
			if (m_dwFileSize == 0)
				return FSERR_CANT_GET_FILESIZE;
		}
	}
	
	return AddContext(dwClientID);
}

BOOL CFileLegacy::_BeforeAccess(FileContext* pCtx)
{
	if (pCtx->dwAccessOffset != 0)
	{
		if (::SetFilePointer(m_hFile, pCtx->dwAccessOffset, NULL, FILE_BEGIN) == 0xffffffff)
			return FALSE;
	}

	return TRUE;
}

BOOL CFileLegacy::_AfterAccess()
{
	return (::SetFilePointer(m_hFile, 0, NULL, FILE_BEGIN) != 0xffffffff);
}

DWORD CFileLegacy::_ReadBytes(FileContext* pCtx, BYTE* lpBuffer, DWORD dwBytesToRead)
{
	_ASSERT(IsValidFile() == TRUE);

	if (dwBytesToRead == 0)
		return 0;

	_ASSERT(lpBuffer != NULL);
	
	if (_BeforeAccess(pCtx) == FALSE)
		return 0;

	DWORD dwRead = 0;
	::ReadFile(m_hFile, lpBuffer, dwBytesToRead, &dwRead, NULL);
	
	_AfterAccess();	// file pointer reset 위해 ReadFile() 다음에는 무조건 호출된다.

	return dwRead;
}

DWORD CFileLegacy::_WriteBytes(FileContext* pCtx, BYTE* lpBuffer, DWORD dwBytesToWrite)
{
	_ASSERT(m_hFile != INVALID_HANDLE_VALUE);

	if (dwBytesToWrite == 0)
		return 0;

	_ASSERT(lpBuffer != NULL);
	
	// 여기선 굳이 _BeforeAccess(), _AfterAccess() 호출 안한다. 
	// 왜냐면 어차피 한놈만 쓰니깐... 굳이 File Pointer Reset이 필요없거덩? 성능만 떨어뜨리지...
	DWORD nWritten;
	if (::WriteFile(m_hFile, lpBuffer, dwBytesToWrite, &nWritten, NULL) == FALSE)
		return 0;

	return nWritten;
}

/////////////////////////////////////////////////////
// CFileMM
/////////////////////////////////////////////////////
CFileMM::CFileMM()
{
	m_hFile		= INVALID_HANDLE_VALUE;
	m_hMMFile	= NULL;	// 이눔은 NULL 이 Invalid 이다... INVALID_HANDLE_VALUE 아니니깐 헷깔리지 말자!
	m_lpBase	= NULL;
}

CFileMM::~CFileMM()
{
	TerminateFile();
}

void CFileMM::TerminateFile()
{
	SCOPED_LOCK_SINGLE(&m_CS);

	Flush();

	if (m_lpBase)
	{
		::UnmapViewOfFile(m_lpBase);
		m_lpBase = NULL;
	}
	if (m_hMMFile)
	{
		::CloseHandle(m_hMMFile);
		m_hMMFile = NULL;
	}
	if (m_hFile != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(m_hFile);
		m_hFile = INVALID_HANDLE_VALUE;
	}

	IFile::TerminateFile();
}

BOOL CFileMM::Flush()
{
	SCOPED_LOCK_SINGLE(&m_CS);

	if (m_lpBase && GetOpenMode() == IFile::OPEN_MODE_WRITE)
		return FlushViewOfFile(m_lpBase, 0);

	return TRUE;
}

long CFileMM::Open(std::wstring& strFileName, DWORD dwForWrite, DWORD dwClientID)
{
	SCOPED_LOCK_SINGLE(&m_CS);

	int rval = IFile::Open(strFileName, dwForWrite, dwClientID);
	if (rval != FSERR_GOOD)	// open 실패 했거나 이미 open된 화일이면 context만 추가하고 걍 리턴!
		return rval;

	if (IsValidFile() == FALSE)
	{	
		////////////////////////////////////// Set File Options for each open modes
		DWORD dwAccess, dwAccess2, dwFlagsAndAttr, dwProtect;
		if (GetOpenMode() == IFile::OPEN_MODE_WRITE)
		{
			dwAccess  = GENERIC_ALL;
			dwProtect = PAGE_READWRITE;
			dwAccess2 = FILE_MAP_WRITE;
			dwFlagsAndAttr = FILE_ATTRIBUTE_NORMAL;	
		}
		else
		{
			dwAccess  = GENERIC_READ;
			dwProtect = PAGE_READONLY;
			dwAccess2 = FILE_MAP_READ;
			dwFlagsAndAttr = FILE_FLAG_SEQUENTIAL_SCAN;
		}
		
		////////////////////////////////////// Open File
		m_hFile = ::CreateFile(strFileName.c_str(), dwAccess, FILE_SHARE_READ, NULL, OPEN_ALWAYS, dwFlagsAndAttr, NULL);
		if (m_hFile == INVALID_HANDLE_VALUE)
			return FSERR_CANT_OPEN_FILE;
		
		if (GetOpenMode() == IFile::OPEN_MODE_WRITE)
		{
			m_dwFileSize = dwForWrite;	// Write용일 경우는 화일 사이즈를 담고있다	
		}
		else
		{
			m_dwFileSize = ::GetFileSize(m_hFile, NULL);
			if (m_dwFileSize == NULL)
				return FSERR_CANT_GET_FILESIZE;
		}
		
		////////////////////////////////////// Map file to memory
		m_hMMFile = ::CreateFileMapping(m_hFile, NULL, dwProtect, 0, m_dwFileSize, NULL);
		
		if (m_hMMFile == NULL)
			return FSERR_CANT_CREATE_MMFILE;
		
		BYTE* lpFile = (BYTE*)::MapViewOfFile(m_hMMFile, dwAccess2, 0, 0, 0);
		if (lpFile == NULL)
			return FSERR_CANT_GET_MM_FILEPOINTER;
		
		m_lpBase = lpFile;
	}

	return AddContext(dwClientID);
}

DWORD CFileMM::_ReadBytes(FileContext* pCtx, BYTE* lpBuffer, DWORD dwBytesToRead)
{
	if (m_lpBase == NULL)
	{
		return 0;
	}

	if (pCtx->dwAccessOffset + dwBytesToRead > GetFileSize())
		dwBytesToRead = GetFileSize() - pCtx->dwAccessOffset;
	
	::memcpy(lpBuffer, m_lpBase + pCtx->dwAccessOffset, dwBytesToRead);
	return dwBytesToRead;
}

DWORD CFileMM::_WriteBytes(FileContext* pCtx, BYTE* lpBuffer, DWORD dwBytesToWrite)
{
	if (m_lpBase == NULL)
	{
		return 0;
	}

	if (pCtx->dwAccessOffset + dwBytesToWrite > GetFileSize())
		dwBytesToWrite = GetFileSize() - pCtx->dwAccessOffset;

	::memcpy(m_lpBase + pCtx->dwAccessOffset, lpBuffer, dwBytesToWrite);
	return dwBytesToWrite;
}


/////////////////////////////////////////////////////
// CFileSystem
/////////////////////////////////////////////////////
CFileSystem* Singleton<CFileSystem>::s_pObject = NULL;

CFileSystem theFS;

CFileSystem::CFileSystem()
{
	Cleanup();

	m_MaxCacheSize = DEFAULT_MMCACHE_SIZE;
}

CFileSystem::~CFileSystem()
{
	Cleanup();
}

void CFileSystem::Cleanup()
{
	SCOPED_LOCK_SINGLE(&m_CS);

	// 사용중인 화일 청소
	FILES_BY_ID::iterator it = m_Files.begin();
	FILES_BY_ID::iterator it_end = m_Files.end();
	for (; it != it_end; ++it)
		delete (*it).second;

	m_Files.clear();

	m_FileLookup.clear();

	m_CacheUsage = 0;
	m_nFileCountInUse = 0;

	// 캐쉬 히트율 계산용...
	m_OpenTry		= 0;	// 사용중인 화일 재사용 포함
	m_CacheHit		= 0;

	m_bShareFile	= FALSE;	// 읽기용으로 open한 화일들 share할 것인가?
}

BOOL CFileSystem::Create(size_t CacheSize, BOOL bShareFile)
{
	m_MaxCacheSize = CacheSize;
	m_bShareFile = bShareFile;

	if (m_IDPool.Create() == FALSE)
		return FALSE;

	return TRUE;
}

long CFileSystem::OpenFile(FILE_TYPE file_type, std::wstring& strFileName, DWORD dwForWrite, DWORD dwClientID, DWORD& dwFileHandle)
{
	if (file_type == FILE_TYPE_INVALID)
		return 0;

	SCOPED_LOCK_SINGLE(&m_CS);

//	BOOL bNewEntry = FALSE;
	
	// 모든 화일명은 소문자로!
	std::wstring strTemp;
	::StrLwr(strTemp, strFileName.c_str());
	strFileName.assign(strTemp);

	IFile* pFile = _CreateFile(strFileName, dwFileHandle, file_type);
	if (pFile == NULL)
	{
		PutLog(LOG_FATAL, _T("CFileSystem::_CreateFile() 실패 [FileName: %s]"), strFileName.c_str());
		return FSERR_CANT_CREATE_FILE;
	}

	if (pFile->GetFileType() != file_type)
	{
		_ASSERT(FALSE);	// 불가능하잖아!
		
		PutLog(LOG_WARNING, _T("CFileSystem::OpenFile() 캐쉬된 화일과 다른 화일타입을 지정해서 Open하려 했다[FileType: %d, %d][FileName: %s]"),
								pFile->GetFileType(), file_type, pFile->GetFileName());
		return FSERR_FILETYPE_MISMATCH;
	}
		
	int nRval = pFile->Open(strFileName, dwForWrite, dwClientID);
	if (nRval == FSERR_GOOD)
	{
		if (pFile->GetOpenMode() == IFile::OPEN_MODE_READ)
			++m_OpenTry;

		dwFileHandle = pFile->GetFileHandle();
		if (dwFileHandle == NULL)
			dwFileHandle = _AddNewFileEntry(pFile);

		if (dwFileHandle == NULL)
		{
			nRval = FSERR_FILE_ENTRY_ADD_FAILED;
			goto _FAILED_OPEN_FILE;
		}
	}
	else
	{
		PutLog(LOG_FATAL, _T("CFileSystem::OpenFile() - 화일 오픈실패 [FileName: %s][ErrorCode: %d]"), strFileName.c_str(), nRval);
		goto _FAILED_OPEN_FILE;
	}

	++m_nFileCountInUse;
	return FSERR_GOOD;
	
_FAILED_OPEN_FILE:
	if (pFile != NULL)
	{
		// 기존에 사용중이던 화일이면 함부로 지우면 안되겠지...
		if (pFile->GetFileHandle() == NULL)
			delete pFile;
	}

	return nRval;
}

size_t CFileSystem::CloseFile(DWORD dwFileHandle, DWORD dwUserID)
{
	SCOPED_LOCK_SINGLE(&m_CS);

	FILES_BY_ID::iterator it = m_Files.find(dwFileHandle);
	if (it == m_Files.end())
		return 0;

	IFile* pFile = (*it).second;
	
	_ASSERT(pFile != NULL);
	
	size_t nRefCnt = pFile->Close(dwUserID);

	--m_nFileCountInUse;
	
	IFile::OPEN_MODE OpenMode = pFile->GetOpenMode();
	if (nRefCnt == 0)
	{
		if (m_MaxCacheSize == 0 || OpenMode == IFile::OPEN_MODE_WRITE)
		{
			_RemoveFile(pFile);
			
			m_Files.erase(it);
		}
	}

//	PutLog(LOG_NOTIFY_FILE, _T("화일 닫힘 (화일 ID: %d, OpenMode: %d, 현재 열린 화일 갯수: %d"), dwFileHandle, OpenMode, m_Files.size());
	
	return nRefCnt;
}

IFile* CFileSystem::GetFile(DWORD dwFileHandle)
{
	SCOPED_LOCK_SINGLE(&m_CS);

	FILES_BY_ID::iterator it = m_Files.find(dwFileHandle);
	if (it == m_Files.end())
	{
		PutLog(LOG_WARNING, _T("CFileSystem::GetFile() - 지정한 화일을 얻을 수 없다 [FileHandle: %d]"), dwFileHandle);
		return NULL;
	}

	return (*it).second;
}

DWORD CFileSystem::_AddNewFileEntry(IFile* pFile)
{
	DWORD dwFileHandle = m_IDPool.AllocID();
	pFile->SetFileHandle(dwFileHandle);
	
	m_Files.insert(FILES_BY_ID::value_type(dwFileHandle, pFile));
	
	if (pFile->GetFileType() != FILE_TYPE_MEMORYMAPPED	||	// memory mapped file이 아니거나 
		pFile->GetOpenMode() != IFile::OPEN_MODE_READ	||	// Read 용으로 Open된 화일이 아니거나
		m_MaxCacheSize == 0)										// 명시적으로 caching하지 말라고 NetEngine생성시 저정을 했으면!
		return dwFileHandle;

	if (m_MaxCacheSize > 0)
	{
		size_t nDeletedFileSize = 0;
		size_t nNewCacheUsage = m_CacheUsage + pFile->GetFileSize();
		while( nNewCacheUsage >= m_MaxCacheSize )
		{
			nDeletedFileSize = _DeleteIdleFile();

			// 현재 열려있는 아무 화일이 없거나, 놀고 있는 화일도 없는 경우
			if (nDeletedFileSize == 0)
				break;

			nNewCacheUsage -= nDeletedFileSize;
		}

		if (nNewCacheUsage < 0)
			nNewCacheUsage = 0;

		if (nNewCacheUsage > 0x7fffffff)
			nNewCacheUsage = 0x7fffffff;

		m_CacheUsage = nNewCacheUsage;

		m_FileLookup.insert(NAME_LOOKUP::value_type(pFile->GetFileName(), dwFileHandle));
	}

	return dwFileHandle;
}

DWORD CFileSystem::_DeleteIdleFile()
{
	IFile* pFile = NULL;

	FILES_BY_ID::iterator it = m_Files.begin();
	for (; it != m_Files.end(); ++it)
	{
		pFile = (*it).second;
		if (pFile->GetRefCnt() == 0)
		{
			DWORD dwFileSize = pFile->GetFileSize();
			
			_RemoveFile(pFile);
			
			m_Files.erase(it);

			return dwFileSize;
		}
	}

	return 0;
}

void CFileSystem::_RemoveFile(IFile* pFile)
{
	if (pFile == NULL)
		return;

	NAME_LOOKUP::iterator it_lookup = m_FileLookup.find(pFile->GetFileName());
	if (it_lookup != m_FileLookup.end())
		m_FileLookup.erase(it_lookup);
	
	m_IDPool.FreeID(pFile->GetFileHandle());

	delete pFile;	
}

// 반드시 lock 걸린 상태에서 호출하자!
IFile* CFileSystem::_CreateFile(std::wstring& strFileName, DWORD& dwFileHandle, FILE_TYPE file_type)
{
	dwFileHandle = NULL;

	// 먼저 화일이름으로 lookup 테이블에서 찾아보고!
	// (단, strFileName 는 반드시 소문자로 바뀐상태에서 호출해야 한다아!)
	NAME_LOOKUP::iterator it_lookup = m_FileLookup.find(strFileName);
	if (it_lookup != m_FileLookup.end())
	{
		dwFileHandle = (*it_lookup).second;

		_ASSERT(dwFileHandle != NULL);

		// 있다면 file handle 통해서 실제 화일 객체 포인터를 얻는다.
		FILES_BY_ID::iterator it_file = m_Files.find(dwFileHandle);
		
		_ASSERT(it_file != m_Files.end());

		return (*it_file).second;
	}
	else
	{
		IFile* pFile = NULL;
		switch (file_type)
		{
		case FILE_TYPE_MEM:
			pFile = new CFileMem;
			break;
		case FILE_TYPE_LEGACY:
			pFile = new CFileLegacy;
			break;
		case FILE_TYPE_MEMORYMAPPED:
			pFile = new CFileMM;
			break;
		default:
			_ASSERT(FALSE);
			break;
		}

		return pFile;
	}
}

