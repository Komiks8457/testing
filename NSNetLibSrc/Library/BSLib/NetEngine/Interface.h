#pragma once

#include "netengine_def.h"
#include "Msg.h"

#include "../thread/bsthread.h"
#include "../util/facilities.h"
#include "../util/que.h"
#include "../util/chunkallocator.h"

#include "../pattern/singleton.h"

#define FS_RESERVED_SYSTEM_SESSION_ID		0

////////////////////////////////////////////////////////////////////////////////
// some reserved system tasks
////////////////////////////////////////////////////////////////////////////////

DEFINE_TASKID_SYS(TASK_SYS_ACCEPTOR,	1)
DEFINE_TASKID_SYS(TASK_SYS_NETIO,		2)
DEFINE_TASKID_SYS(TASK_SYS_MAINTAINER,	3)
DEFINE_TASKID_SYS(TASK_SYS_PERFMON,		4)
DEFINE_TASKID_SYS(TASK_SYS_PEER_NETIO,	5)

/////////////////////////////////////////////////////////////
// FileContext
/////////////////////////////////////////////////////////////
struct FileContext
{
	FileContext() { dwAccessOffset = 0;	}

	DWORD	dwAccessOffset;

	DWORD	MoveAccessOffset(long offset)
	{
		dwAccessOffset+= offset;

		_ASSERT((long)dwAccessOffset >= 0);

		return dwAccessOffset;
	}

	void	SetAccessOffset(DWORD cur_pos)
	{
		dwAccessOffset = cur_pos;

		_ASSERT((long)dwAccessOffset >= 0);
	}
};


typedef HASH_MAP<DWORD, FileContext*>		FILECTX;
/////////////////////////////////////////////////////////////
// IFile
/////////////////////////////////////////////////////////////

class IFile
{
public:
	IFile()
	{
		m_dwFileHandle	= NULL;
		m_dwFileSize	= 0;
		m_dwHitCount	= 0;
		m_OpenMode		= OPEN_MODE_UNKNOWN;
		m_FileType		= FILE_TYPE_INVALID;
	}
	virtual ~IFile()
	{
		TerminateFile();
	}

	enum OPEN_MODE
	{
		OPEN_MODE_UNKNOWN = -1,
		OPEN_MODE_READ = 0,
		OPEN_MODE_WRITE
	};
	
protected:
	OPEN_MODE		m_OpenMode;
	FILE_TYPE		m_FileType;

	DWORD			m_dwFileHandle;
	DWORD			m_dwFileSize;

	std::wstring	m_strFileName;

	FILECTX			m_Contexts;

	CCriticalSectionBS	m_CS;

	DWORD			m_dwHitCount;

public:
	long			AddContext(DWORD dwID);

protected:
	DWORD			GetRemainBytes(FileContext* pCtx);
	
	FileContext*	GetContext(DWORD dwID);
	size_t			EraseContext(DWORD dwID);
	void			SetOpenMode(OPEN_MODE mode) { m_OpenMode = mode; }

	DWORD		GetFileAccessPos(DWORD dwFrom, FileContext* pCtx)
	{
		DWORD dwBasePos = 0;
		switch (dwFrom)
		{
		case FILE_BEGIN:
			dwBasePos = 0;
			break;
		case FILE_CURRENT:
			dwBasePos = pCtx->dwAccessOffset;
			break;
		case FILE_END:
			dwBasePos = m_dwFileSize;
			break;
		default:
			_ASSERT(FALSE);
			break;
		}

		return dwBasePos;
	}

protected:
	void	SetFileHandle(DWORD dwHandle) { _ASSERT(dwHandle != NULL); m_dwFileHandle = dwHandle; }
	
	virtual DWORD	_ReadBytes(FileContext* pCtx, BYTE* pDestBuf, DWORD Len) = 0;
	virtual DWORD	_WriteBytes(FileContext* pCtx, BYTE* pSrcBuf, DWORD Len) = 0;

	virtual size_t	Close(DWORD dwID);
	
public:
	virtual void	TerminateFile();		// @redpixel {14-09-29} NetEngine\FileSystem.cpp에 IFile::TerminateFile() 함수가 이미 구현되어있어서 선언부를 수정합니다.
	virtual BOOL	Flush()			= 0;
	virtual HANDLE	GetHandle()		= 0;
	virtual BOOL	IsValidFile()	= 0;
	
	virtual long	Open(std::wstring& strFileName, DWORD dwForWrite, DWORD dwID);

	virtual BYTE*		GetFilePtr() { return NULL; }
	virtual BYTE*		ChangeBuffer(BYTE* pNewBuf, DWORD dwBufSize) { _ASSERT(FALSE); return NULL; }
	virtual FILE_TYPE	GetFileType() { return FILE_TYPE_INVALID; }
	
	DWORD		Read(DWORD dwID, LPBYTE lpBuffer, long nBytesToRead);
	DWORD		Write(DWORD dwID, BYTE* lpBuffer, long nBytesToWrite);
	long		Seek(DWORD dwID, long Offset, DWORD dwFrom = FILE_BEGIN);

	DWORD		GetFileHandle() { if (this == NULL) return 0; return m_dwFileHandle; }
	DWORD		GetFileSize() { return m_dwFileSize; }
	LPCWSTR		GetFileName() { return m_strFileName.c_str(); }
	OPEN_MODE	GetOpenMode() { _ASSERT(m_OpenMode != OPEN_MODE_UNKNOWN); return m_OpenMode; }
	DWORD		GetHitCount() { return m_dwHitCount; }
	long		GetCurFileOffset(DWORD dwID);
	
	size_t		GetRefCnt()   { SCOPED_LOCK_SINGLE(&m_CS); return m_Contexts.size(); }

	friend class CFileSystem;
};

/////////////////////////////////////////////////////////////
// Maintainer Configuration
/////////////////////////////////////////////////////////////
struct MaintainerEntry
{
	DWORD interval;
	DWORD timeout;

	MaintainerEntry() : interval(TIMEOUT_INFINITE), timeout(TIMEOUT_INFINITE) {}	
};

struct MaintainerConfig
{
	MaintainerConfig() : resolution(MAINTAINER_POLL_RESOLUTION), target_flags(0)
	{
		SetDefaultValue();
	}

	DWORD resolution;
	DWORD target_flags;
	MaintainerEntry target_entries[MAINTAIN_TARGET_NUM];

	void SetDefaultValue()
	{
		if (target_flags & MT_MASK_STALE_SESSIONS)
		{
			target_entries[MAINTAIN_TARGET_STALE_SESSIONS].interval = MAINTAINER_STALE_SESSION_INTERVAL;
			target_entries[MAINTAIN_TARGET_STALE_SESSIONS].timeout  = MAINTAINER_STALE_SESSION_TIMEOUT;
		}

		// 현재는 안쓰이고 있다. 사실 앞으로도 크게 쓸일은 없어보이네...
		if (target_flags & MT_MASK_RARELY_USED_FILES)
		{
			// 안쓰이는 화일들 cache에서 내리기
			target_entries[MAINTAIN_TARGET_RARELY_USED_FILES].interval = (60 * 1000) * 5;	// 5분마다 한번씩 체크해서
			target_entries[MAINTAIN_TARGET_RARELY_USED_FILES].timeout  = (60 * 1000) * 10; // 10분 이상 activity 없는 세션 죽여라
		}

		if (target_flags & MT_MASK_KEEP_ALIVE_SESSION)
		{
			target_entries[MAINTAIN_TARGET_KEEP_ALIVE_SESSION].interval = MAINTAINER_STALE_SESSION_INTERVAL / 10;
			target_entries[MAINTAIN_TARGET_KEEP_ALIVE_SESSION].timeout  = MAINTAINER_STALE_SESSION_TIMEOUT;
		}
	}
};

enum SOCKBUF_OPTION
{
	SOCKBUF_OPTION_DEFAULT = 0,
	SOCKBUF_OPTION_TURNOFF,
	SOCKBUF_OPTION_MAXIMUM
};

// 클라이언트 응답 없을때, Buffered Mode 에서 메시지 무한 쌓이는 것 방지...
#define DEFAULT_SENDQUE_DEPTH	100
#define MAX_SENDQUE_DEPTH		0xffffffff

struct SESSION_CONFIG
{
	SESSION_CONFIG() { Reset(); }
	
	void Reset()
	{
		nType = BS_SOCK_STREAM;
		bNonBlocking			= TRUE;
		bGracefulShutdown		= FALSE;
		
		nBufOption				= SOCKBUF_OPTION_DEFAULT;
		bApplyBufOptionForSend	= TRUE;
		bApplyBufOptionForRecv	= TRUE;

		dwDefaultTask = 0;
		IOMode = IO_MODE_BUFFERED;

		dwMaxSendQueDepth = DEFAULT_SENDQUE_DEPTH;
	}

	// socket relatives
	int				nType; // TCP? or UDP?
	BOOL			bNonBlocking;
	BOOL			bGracefulShutdown;
	
	long			nBufOption;
	BOOL			bApplyBufOptionForSend;
	BOOL			bApplyBufOptionForRecv;

	DWORD			dwMaxSendQueDepth;

	// netengine relatives
	DWORD			dwDefaultTask;
	
	IO_MODE			IOMode;
};

struct SKeepAlive
{
	SKeepAlive(BSSOCKADDR_IN* addr_connect, BSSOCKADDR_IN* addr_bind, DWORD dwTask, DWORD dwSession) : reliable_session(false), addrConnect(*addr_connect)
	{
		if (addr_bind != NULL)
		{
			addrBind = *addr_bind;
		}
		else
		{
			addrBind.sin_addr = 0;
			addrBind.sin_port = 0;
		}

		dwTaskToBind	 = dwTask;
		register_session_id = dwSession;
		assoc_session_id = register_session_id;
	}

	bool			reliable_session;
	BSSOCKADDR_IN	addrConnect;
	BSSOCKADDR_IN	addrBind;
	DWORD			dwTaskToBind;	
	DWORD			register_session_id;
	DWORD			assoc_session_id;
		
	bool operator==(const SKeepAlive& keep_alive) const
	{
		if (addrConnect.sin_addr == keep_alive.addrConnect.sin_addr && addrConnect.sin_port == keep_alive.addrConnect.sin_port && addrBind.sin_addr == keep_alive.addrBind.sin_addr && addrBind.sin_port == keep_alive.addrBind.sin_port)
			return true;

		return false;
	}
};

////////////////////////////////
// LOG types
////////////////////////////////
// 음.. 보기 않좋으니 Log는 다른데로 뺄까 생각중
/*
enum eLogLevel
{
	LL_NOTIFY,		// 그냥 알려주고 싶을때
	LL_WARNNING,	// 경고 (서버를 내릴정도는 아니지만 주의하라 머 그런거)
	LL_FATAL,		// 더이상 서버를 진행 할 수 없을때
	LL_COUNT,
};

#define LOGTYPE( level, major_index, minor_index) (DWORD)(((level & 0xFF) << 24) | ((major_index & 0xFF) << 16) | (minor_index & 0xFFFF))
#define LOG_NOTIFY		LOGTYPE( LL_NOTIFY, 0, 0)
#define LOG_WARNING	LOGTYPE( LL_WARNNING, 0, 0)
#define LOG_FATAL		LOGTYPE( LL_FATAL, 0, 0)
#define LOG_FATAL_FILE	LOGTYPE( LL_FATAL, 0, 1)
*/

////////////////////////////////
// NetEngine Configuration
////////////////////////////////
#define DEFAULT_ACCEPT_TIMEOUT_SEC		(60 * 1)	// 1분간 accept 되지 않으면 끊어버린다.
#define MASSIVE_CONCURRENT_ACCEPT		100

#define ENGINE_CONFIG_SERVER_ROLE				0x00000001
#define ENGINE_CONFIG_CLIENT_ROLE				0x00000002
#define ENGINE_CONFIG_MASSIVE_CLIENT			0x00000010
#define ENGINE_CONFIG_FILE_DISPATCHER			0x00000020

inline void DefaultMsgDump(DWORD data1, DWORD data2)
{
	Put(_T("[MsgID: 0x%x] [Count: %d]"), data1, data2);
}

// BSLib가 아직 유니코드가 아니기땜시.. LPCTSTR을 하면 다른곳에 포함을 못시킨다 by novice
extern void PutLog(DWORD logtype, LPCTSTR foramt, ...);
#ifdef SERVER_BUILD
extern void PutLog(DWORD logtype, JSONValue& value);
#endif // #ifdef SERVER_BUILD

typedef bool	(*lpfReportLogFunc)(DWORD logType, LPCTSTR format,...);
#ifdef SERVER_BUILD
typedef bool	(*lpfReportLogFunc_Json)(DWORD logType, JSONValue& value);
#endif // #ifdef SERVER_BUILD
typedef void	(*MSG_DUMP_CALLBACK)(DWORD Data1, DWORD Data2);

struct NETENGINE_CONFIG
{
	NETENGINE_CONFIG::NETENGINE_CONFIG()
	{
		SockMajorVer = 
		SockMinorVer = 2;		// 일반적인 경우라면 건드릴 일이 없겠지?
		
		dwConfigFlags= 0;
		
		dwAcceptTimeout = DEFAULT_ACCEPT_TIMEOUT_SEC;
		
		FileCacheSize = 0;

		io_thread_count = 0;

		SYSTEM_INFO sysinfo;
		::GetSystemInfo(&sysinfo);
		io_thread_count = sysinfo.dwNumberOfProcessors * 2;
		dwConcurrentAcceptPosting = io_thread_count;

		FileTypeForSend = FILE_TYPE_INVALID;
		FileTypeForRecv = FILE_TYPE_INVALID;

		lpfMsgPoolDump		= DefaultMsgDump;
		lpfReportLog		= NULL;
#ifdef SERVER_BUILD
		lpfReportLog_Json	= NULL;
#endif // #ifdef SERVER_BUILD

		SecurityMode = BSNET_DEFAULT;
	}
	
	BYTE		SecurityMode;
	
	BYTE		SockMajorVer, SockMinorVer;
	
	DWORD		dwConfigFlags;

	DWORD		dwConcurrentAcceptPosting;
	DWORD		dwAcceptTimeout;	// in seconds
	
	size_t		FileCacheSize;	// in bytes
	FILE_TYPE	FileTypeForSend;
	FILE_TYPE	FileTypeForRecv;

	long		io_thread_count;

	lpfReportLogFunc			lpfReportLog;
#ifdef SERVER_BUILD
	lpfReportLogFunc_Json		lpfReportLog_Json;
#endif // #ifdef SERVER_BUILD
	MSG_DUMP_CALLBACK			lpfMsgPoolDump;

	MaintainerConfig	MC;

	SESSION_CONFIG		SessionConfig[SESSION_TYPE_NUM];

	DWORD		dwDebugOption[ 2];

	void	Init(DWORD dwFlags)
	{
		dwConfigFlags = dwFlags;

		////////////////////////////////////////////////
		// Accept 관련
		////////////////////////////////////////////////
		if (dwFlags & ENGINE_CONFIG_SERVER_ROLE)
		{
			if (dwFlags & ENGINE_CONFIG_MASSIVE_CLIENT)
				dwConcurrentAcceptPosting = MASSIVE_CONCURRENT_ACCEPT;

			dwAcceptTimeout = DEFAULT_ACCEPT_TIMEOUT_SEC;

			// i/o mode가 buffered인 경우는 무시된다.
			SessionConfig[SESSION_TYPE_PASSIVE].dwMaxSendQueDepth = DEFAULT_SENDQUE_DEPTH;
			SessionConfig[SESSION_TYPE_ACTIVE].dwMaxSendQueDepth  = MAX_SENDQUE_DEPTH;
		}

		////////////////////////////////////////////////
		// File 처리 관련
		////////////////////////////////////////////////
		if (dwFlags & ENGINE_CONFIG_FILE_DISPATCHER)
		{
			MEMORYSTATUS MemStatus;
			::GlobalMemoryStatus(&MemStatus);
			FileCacheSize = MemStatus.dwTotalPhys / 4;	// 대략 이정도면 되겠지?
			
			FileTypeForSend = FILE_TYPE_MEMORYMAPPED;
			FileTypeForRecv = FILE_TYPE_LEGACY;
		}
		else
		{
			FileCacheSize = 0;	// caching하지 않는다.
			
			if ((dwFlags & ENGINE_CONFIG_SERVER_ROLE) == 0)	// 순수 클라이언트다.
			{
				FileTypeForSend = FILE_TYPE_LEGACY;
				FileTypeForRecv = FILE_TYPE_MEM;	// 최종적으로 pack file에 write되야 하거덩...
			}
			else
			{
				FileTypeForSend = FILE_TYPE_LEGACY;
				FileTypeForRecv = FILE_TYPE_LEGACY;
			}
		}

		////////////////////////////////////////////////
		// Maintainer 관련
		////////////////////////////////////////////////
		if ((dwFlags & ENGINE_CONFIG_SERVER_ROLE) == 0)	// 순수 클라이언트다.
		{
			dwDebugOption[ 0] =	DEBUG_OPTION_DEFAULT_CLIENT_DEBUGGER_PRESENT;
			dwDebugOption[ 1] =	DEBUG_OPTION_DEFAULT_CLIENT_STAND_ALONE;
		}
		else
		{
			dwDebugOption[ 0] =	DEBUG_OPTION_DEFAULT_SERVER_DEBUGGER_PRESENT;
			dwDebugOption[ 1] =	DEBUG_OPTION_DEFAULT_SERVER_STAND_ALONE;
		}

		////////////////////////////////////////////////
		// Session Config
		////////////////////////////////////////////////
		SessionConfig[SESSION_TYPE_ACTIVE].IOMode  = IO_MODE_BUFFERED;
		SessionConfig[SESSION_TYPE_PASSIVE].IOMode = IO_MODE_DIRECT;
	}
};

/////////////////////////////////////////////////////////////////////////
// NetEngine interface
/////////////////////////////////////////////////////////////////////////

// GUID FOR NETENGINE
// {6BCAF077-95B7-46e8-AD0A-BB12D6F8A94B}

// {C39258FD-9E80-4ecf-A0D2-228706E3A317}
// {8FB726FA-E938-471d-9F26-AA717CFB3183}
static const GUID IID_BSNET = {
	0xa4af69f3, 0xc8b3, 0x4242, {0x9a, 0x96, 0x7, 0x8f, 0xf2, 0x7e, 0xb1, 0xf8}};

struct __declspec(novtable) IUnknownInterface
{
	virtual HRESULT	__stdcall QueryInterface(REFIID riid, LPVOID FAR * ppvObj) = 0;
    virtual ULONG	__stdcall AddRef()  = 0;
    virtual ULONG	__stdcall Release() = 0;
};

struct __declspec(novtable) IBSNet : public IUnknownInterface
{
    // IUnknown methods
    virtual HRESULT	__stdcall QueryInterface(REFIID riid, LPVOID FAR * ppvObj) = 0;
    virtual ULONG	__stdcall AddRef()  = 0;
    virtual ULONG	__stdcall Release() = 0;

	virtual BOOL	__stdcall Create(NETENGINE_CONFIG Config) = 0;
	virtual DWORD	__stdcall Connect(LPCWSTR lpszAddrConnect,WORD nPort,LPCWSTR lpszAddrBind, DWORD dwTaskToBind, BOOL bKeepAlive, bool reconnect = false, DWORD connect_timeout = 3) = 0;
	virtual DWORD	__stdcall Connect(BSSOCKADDR_IN* addr_connect, BSSOCKADDR_IN* addr_bind, DWORD dwTaskToBind = 0, BOOL bKeepAlive = FALSE, DWORD connect_timeout = 3) = 0;
	virtual bool	__stdcall Ping(LPCTSTR domain, WORD port, DWORD& time) = 0;
	virtual BOOL	__stdcall StartServer(LPCWSTR lpszAddr, SHORT sPort) = 0;
	virtual BOOL	__stdcall StartServer(BSSOCKADDR_IN* pAddrToListen) = 0;
	virtual BOOL	__stdcall RegisterTask(DWORD dwTaskID, CTask* pTask, CRuntime* pRT_ServiceObj, ULONG_PTR Param) = 0;
	virtual BOOL	__stdcall OverrideTask(DWORD TaskID, CRuntime* pRT_ServiceObj, DWORD dwParam, CTask* pTask) = 0;
	virtual BOOL	__stdcall ActivateService(DWORD dwTaskID, long ThreadNumToSpawn, long nPriority, DWORD dwStackSize, DWORD dwFlags, ProcessorAffinity* pPA) = 0;
	virtual BOOL	__stdcall ResumeService(DWORD dwTaskID) = 0;
	virtual BOOL	__stdcall SuspendService(DWORD dwTaskID) = 0;
	virtual BOOL	__stdcall KillTask(DWORD dwTaskID) = 0;
	virtual void	__stdcall GetSysResourceUsage(DWORD dwMask, SYS_RES_USAGE& usage) = 0;
	virtual BOOL	__stdcall GetPeerIP(DWORD dwSession, std::wstring* pstrPeerIP, DWORD* pdwIP,WORD* pwPort) = 0;
	virtual CTask*	__stdcall GetTask(DWORD dwTaskID) = 0;

	virtual void	__stdcall SetSessionEncodingContext(DWORD dwSession, EncodeCtx& ec) = 0;
	virtual CMsg*	__stdcall NewMsg(BOOL bEncrypt = FALSE) = 0;
	virtual void	__stdcall DelMsg(CMsg* pMsg) = 0;
	virtual long	__stdcall SendMsg(DWORD dwSession, CMsg* pMsg) = 0;
	virtual BOOL	__stdcall RecvFile(DWORD dwSession, std::wstring strFileFullPath, DWORD dwFileSize, long nOffset = 0, FILE_TYPE file_type = FILE_TYPE_INVALID) = 0;
	virtual BOOL	__stdcall SendFile(DWORD dwSession, std::wstring& strFileFullPath, long nOffset = 0, FILE_TYPE file_type = FILE_TYPE_INVALID) = 0;
	virtual BOOL	__stdcall CloseSession(DWORD dwSession, DWORD dwReason = DISCONNECT_REASON_INTENTIONAL_KILL) = 0;
	virtual size_t	__stdcall CloseFile(DWORD dwSession, DWORD dwFileHandle = NULL) = 0;
	virtual bool	__stdcall SetLinkedSession(DWORD session_id, DWORD linked_session_id) = 0;
	virtual void	__stdcall SkipMsgSecurity(DWORD session_id) = 0;
	virtual bool	__stdcall UpdateReliableSession(DWORD session_id) = 0;	
	virtual bool	__stdcall ResetReliableSession(DWORD session_id) = 0;
	virtual void	__stdcall SetReliableSession(DWORD session_id, bool onoff = true) = 0;	
	virtual bool	__stdcall IsReliableSession(DWORD session_id) = 0;
	virtual DWORD	__stdcall GetSessionType(DWORD dwSession) = 0;
	virtual bool	__stdcall RemoveKeepAliveSessionEntry(DWORD dwSession) = 0;
	virtual ULONGLONG __stdcall GetSessionSerialNumber(DWORD session_id) = 0;
	virtual void	__stdcall SetSessionSerialNumber(DWORD session_id, ULONGLONG serial_number) = 0;	
	virtual bool	__stdcall SetSessionMsgSizeLimit(DWORD session_id, DWORD msg_size_limit) = 0;
	virtual bool	__stdcall SetSessionSendQueDepth(DWORD session_id, DWORD send_que_depth) = 0;
	virtual SKeepAlive*	__stdcall AddActiveSessionKeepAlive(LPCWSTR lpszIP,WORD nPort,LPCWSTR lpszBindIP,DWORD dwTargetTask,DWORD dwAssocSessionID = 0) = 0;
	virtual void	__stdcall ResetFileSystem() = 0;

	virtual void	__stdcall Dump(DWORD dwDumpTarget, int count = -1) = 0;
	
	virtual void	__stdcall SetTerminateProcess(bool bEnable) = 0;
	
	virtual void	__stdcall CloseAllPassiveSession() = 0;	// keep alive상태의 session은 제외
	virtual void*	__stdcall GetMsgGenerator() = 0;
	
	virtual BOOL	__stdcall GetMacAddress( DWORD dwSession, BYTE* pAddress ) = 0;
	virtual void	__stdcall SetSecurityMode( BYTE mode ) = 0;	
	virtual void	__stdcall DisplayEngineVer( void ) = 0;

	//PEER NET
	virtual BOOL	__stdcall StartPeerTask( DWORD dwTaskID ) = 0;
	virtual long	__stdcall StartPeerNetService( WORD port ) = 0;
	//virtual long	__stdcall SendPeerMsg( SOCKADDR_IN& addr, CMsg* pMsg ) = 0;
	//virtual BOOL	__stdcall GetPeerNetSockAddr( SOCKADDR_IN& addr ) = 0;
	virtual void	__stdcall ClosePeerNet() = 0;

	virtual BOOL	__stdcall SetNoDelay( DWORD dwSession, BOOL bNoDelay ) = 0;
};

#define LPIBSNet	IBSNet*
