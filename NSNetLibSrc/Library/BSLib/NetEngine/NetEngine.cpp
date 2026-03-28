#include "StdAfx.h"
#include "NetEngine.h"
#include "Proactor.h"
#include "ProactorEvent.h"
#include "Accept.h"
#include "maintainer.h"
#include <stdio.h>
#include <iphlpapi.h>		// mac address

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Iphlpapi.lib")

///////////////////////////////////////////////////////////////////////
// our friends
///////////////////////////////////////////////////////////////////////
/*
extern "C" __declspec(dllexport) IUnknownInterface* CreateInstance()
{
	IUnknownInterface* pI = NULL;

	if (CNetEngine::GetEngine() == NULL)
		pI = static_cast<IBSNet*>(new CNetEngine);
	else
		pI = CNetEngine::GetEngine();

	pI->AddRef();

	return pI;
}
*/
///////////////////////////////////////////////////////////////////////
// it
///////////////////////////////////////////////////////////////////////
NETENGINE_CONFIG CNetEngine::m_EngineConfig;

#ifdef _DEBUG
#define PASSIVE_SOCKET_POOL_RATIO	2000
#else
#define PASSIVE_SOCKET_POOL_RATIO	200
#endif

#ifdef SERVER_BUILD
void PutLog(DWORD logtype, JSONValue& value)
{
	NETENGINE_CONFIG& ec = CNetEngine::GetEngineConfig();
	if (ec.lpfReportLog_Json != NULL)
	{
		__try
		{
			(*ec.lpfReportLog_Json)(logtype, value);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
		}
	}
}
#endif // #ifdef SERVER_BUILD

extern void InitializeCrcLookupTable();
CNetEngine::CNetEngine() :	
		m_MsgPool(DEFAULT_CHUNK_SIZE, &CMsg::Reset, 0, _T("NetEngine::MsgPool") ), 
		m_ActiveSocketPool(DEFAULT_CHUNK_SIZE, NULL, 0, _T("NetEngine::ActiveSocketPool") ), 
		m_PassiveSocketPool(DEFAULT_CHUNK_SIZE * PASSIVE_SOCKET_POOL_RATIO, NULL, 0, _T("NetEngine::PassiveSocketPool") ), 
		m_CS(TRUE)
{
	m_cRef	  = 0;
	m_pListener = NULL;

	nProactorType = PROACTOR_INVALID;
	bShutdown = TRUE;

	InitializeCrcLookupTable();

}

CNetEngine::~CNetEngine()
{
	for each (SKeepAlive* keep_alive in keep_alive_sessions_)
		delete keep_alive;

	keep_alive_sessions_.clear();

	_ASSERT(m_SessionMgr.GetSessionCount() == 0);
	if (m_pListener)
	{
		delete m_pListener;
		m_pListener = NULL;
	}
}

HRESULT __stdcall CNetEngine::QueryInterface(const IID& iid, void** ppv)
{ 	
	GUID iidBSNet = IID_BSNET;
	if (IsEqualGUID(iid, iidBSNet))
		*ppv = static_cast<IBSNet*>(this);
	else
	{  	   
		*ppv = NULL;
		return E_NOINTERFACE ;
	}

	reinterpret_cast<IUnknownInterface*>(*ppv)->AddRef(); 
	return S_OK ;
}

ULONG __stdcall CNetEngine::Release() 
{
	if (bShutdown == FALSE)
	{
		bShutdown = TRUE;

		m_SessionMgr.InvalidateAllSessions();
		m_SessionMgr.Cleanup();
		
		if(m_pListener)
			m_pListener->Close();
		
		system_.Finalize();
	}

	return 0;
}

BOOL __stdcall CNetEngine::Create(NETENGINE_CONFIG Config)
{
	bShutdown = FALSE;

	system_.Initialize();
	
	// bslibrary debugging moddule option
//	SetDebugOption( Config.dwDebugOption[ 0], Config.dwDebugOption[ 1]);

	if (system_.GetSockSystem()->Create(m_EngineConfig.SockMajorVer, m_EngineConfig.SockMinorVer) == FALSE)
		return FALSE;
	
	m_EngineConfig = Config;
	
	if (Config.lpfMsgPoolDump != NULL)
		m_MsgPool.SetDumpCallbacks(Config.lpfMsgPoolDump);

	///////////////////////////////////////////////////////
	// Create System Services
	///////////////////////////////////////////////////////
	
	// I/O task, acceptor
	CRuntime* pServiceObjIO		= NULL;
	CRuntime* pServiceObjAccept = NULL;

	nProactorType = DetermineProactorType();

//	CThreadManager* pThreadMgr = SYSTEM.GetThreadManager();
	switch (nProactorType)
	{
	case PROACTOR_IOCP:
		pServiceObjIO	  = BS_RUNTIME_CLASS(CProactorIOCP);
		pServiceObjAccept = BS_RUNTIME_CLASS(CProactorIOCP);
		break;
	case PROACTOR_EVENT:
		pServiceObjIO	  = BS_RUNTIME_CLASS(CProactorEvent);
		pServiceObjAccept = BS_RUNTIME_CLASS(CProactorEvent);
		break;
	default:
		_ASSERT(NULL);
		break;
	}

	if (Config.dwConfigFlags & ENGINE_CONFIG_SERVER_ROLE)
	{
		_ASSERT(Config.dwConcurrentAcceptPosting > 0);

		if (RegisterTask(TASK_SYS_ACCEPTOR, NULL, pServiceObjAccept, NULL) == FALSE)
			return FALSE;
	}

	if (RegisterTask(TASK_SYS_NETIO, NULL, pServiceObjIO, NULL) == FALSE)
		return FALSE;

	if (Config.MC.target_flags != 0)
	{
		if (RegisterTask(TASK_SYS_MAINTAINER, NULL, BS_RUNTIME_CLASS(CMaintainer), (ULONG_PTR)&Config.MC) == FALSE)
			return FALSE;
	}

	///////////////////////////////////////////////////////
	// Create System Resources
	///////////////////////////////////////////////////////
	if (Config.dwConfigFlags & ENGINE_CONFIG_SERVER_ROLE)
	{
		// resize chunks for server-side tasks
		if (m_SessionMgr.Create((Config.dwConfigFlags & ENGINE_CONFIG_MASSIVE_CLIENT) ? SERVER_SIDE_SOCKET_POOL_CHUNKSIZE : CLIENT_SIDE_SOCKET_POOL_CHUNKSIZE) == FALSE)
			return FALSE;

		m_PassiveSocketPool.SetChunkSize(SERVER_SIDE_SOCKET_POOL_CHUNKSIZE);
		//m_PassiveSocketPool.SetChunkSize(1);
		m_ActiveSocketPool.SetChunkSize(CLIENT_SIDE_SOCKET_POOL_CHUNKSIZE);
		//m_ActiveSocketPool.SetChunkSize(1);
		m_MsgPool.SetChunkSize(SERVER_SIDE_MSGPOOL_CHUNKSIZE);
	}
	else
	{
		if (m_SessionMgr.Create(CLIENT_SIDE_SOCKET_POOL_CHUNKSIZE) == FALSE)
			return FALSE;

		m_PassiveSocketPool.SetChunkSize(SERVER_SIDE_SOCKET_POOL_CHUNKSIZE);
		m_ActiveSocketPool.SetChunkSize(CLIENT_SIDE_SOCKET_POOL_CHUNKSIZE);
		m_MsgPool.SetChunkSize(CLIENT_SIDE_MSGPOOL_CHUNKSIZE);
	}

	size_t PhyMemTotal = 0;
	size_t PhyMemAvail = 0;
	GetPhyMemorySize(PhyMemTotal, PhyMemAvail);
	
	if (Config.FileCacheSize > PhyMemAvail / 2)
		Config.FileCacheSize = PhyMemAvail / 2;
	
	/*
	//if (FILESYSTEM.Create(Config.dwFileCacheSize, (Config.dwConfigFlags & ENGINE_CONFIG_FILE_DISPATCHER) ? TRUE : FALSE) == FALSE)
	if (FILESYSTEM.Create(Config.FileCacheSize, TRUE) == FALSE)
		return FALSE;
		*/
	
	return TRUE;
}

DWORD __stdcall CNetEngine::Connect(LPCWSTR lpszAddrConnect, WORD nPort, LPCWSTR lpszAddrBind, DWORD dwTaskToBind, BOOL bKeepAlive, bool reconnect, DWORD connect_timeout)
{	
	DWORD ip_bind = CSockSystem::GetAddressFromName(lpszAddrBind);
	DWORD ip = CSockSystem::GetAddressFromName(lpszAddrConnect);
	if (ip == INADDR_NONE || ip_bind == INADDR_NONE)
		return 0;

	BSSOCKADDR_IN addr_connect;
	BSSOCKADDR_IN addr_bind;
	::ZeroMemory(&addr_connect, sizeof(addr_connect));
	::ZeroMemory(&addr_bind, sizeof(addr_bind));
	
	addr_connect.sin_family	= AF_INET;
	addr_connect.sin_port	= htons(nPort);
	addr_connect.sin_addr	= ip;

	BSSOCKADDR_IN* pAddrBind = NULL;
	if (addr_bind.sin_addr != 0)
		pAddrBind = &addr_bind;

	DWORD session_id = Connect(&addr_connect, pAddrBind, dwTaskToBind, bKeepAlive, connect_timeout);

	if (bKeepAlive == TRUE && reconnect)
	{
		SKeepAlive* newone = new SKeepAlive(&addr_connect, pAddrBind, dwTaskToBind, session_id);
		if (AddActiveSessionKeepAlive(*newone) == false)
		{
			delete newone;
			CloseSession(session_id);
		}
	}

	return session_id;
}

DWORD __stdcall CNetEngine::Connect(BSSOCKADDR_IN* addr_connect, BSSOCKADDR_IN* addr_bind, DWORD dwTaskToBind, BOOL bKeepAlive, DWORD connect_timeout)
{
	DWORD dwNewSessionID = NULL;
	CSockStream* pSock = AllocActiveNewSock();	
	if (pSock != NULL)
	{
		if (pSock->CreateActiveSession(this, addr_connect, addr_bind, bKeepAlive, connect_timeout) == FALSE)
		{
			FreeActiveSocket(pSock);
		}
		else
		{
			dwTaskToBind = (dwTaskToBind == 0) ? m_EngineConfig.SessionConfig[SESSION_TYPE_ACTIVE].dwDefaultTask : dwTaskToBind;
			dwNewSessionID = CreateNewSession(pSock, dwTaskToBind);
		}
	}
		
	return dwNewSessionID;
}

bool __stdcall CNetEngine::Ping(LPCTSTR domain, WORD port, DWORD& time)
{
	time = 1;

	DWORD ip = CSockSystem::GetAddressFromName(domain);
	if (ip == INADDR_NONE)
		return false;

	BSSOCKADDR_IN addr_connect;
	BSSOCKADDR_IN addr_bind;
	::ZeroMemory(&addr_connect, sizeof(addr_connect));
	::ZeroMemory(&addr_bind, sizeof(addr_bind));

	addr_connect.sin_family	= AF_INET;
	addr_connect.sin_port	= htons(port);
	addr_connect.sin_addr	= ip;

	CSockStream* pSock = AllocActiveNewSock();	
	if (pSock == NULL)
		return false;

	ULONGLONG start_tick = GetTickCount64();
	BOOL succeeded = pSock->CreateActiveSession(this, &addr_connect, nullptr, false, 1);
	ULONGLONG elapsed_tick = GetTickCount64() - start_tick;
	FreeActiveSocket(pSock);

	if (!succeeded)
		return false;

	if (elapsed_tick >= 1000)
		return false;

	time = static_cast<DWORD>(elapsed_tick);
	return true;
}

BOOL __stdcall CNetEngine::StartServer(LPCWSTR lpszAddr, SHORT sPort)
{
	if ((m_EngineConfig.dwConfigFlags & ENGINE_CONFIG_SERVER_ROLE) == 0)
	{
		_ASSERT(FALSE);
		return FALSE;
	}

	DWORD ip = 0;
	if (lpszAddr == NULL)
		ip = 0;
	else
		ip = CSockSystem::GetAddressFromName(lpszAddr);

	if (ip == INADDR_NONE)
		return FALSE;

	BSSOCKADDR_IN addr_listen;
	::ZeroMemory(&addr_listen, sizeof(addr_listen));
	addr_listen.sin_family	= AF_INET;
	addr_listen.sin_port	= ::htons(sPort);
	addr_listen.sin_addr	= ip;

	return StartServer(&addr_listen);
}

BOOL __stdcall CNetEngine::StartServer(BSSOCKADDR_IN* pAddrToListen)
{
	if ((m_EngineConfig.dwConfigFlags & ENGINE_CONFIG_SERVER_ROLE) == 0)
	{
		_ASSERT(FALSE);
		return FALSE;
	}

	SAFE_DELETE(m_pListener);

	m_pListener = new CSockListener;
	if (m_pListener->Create(this, pAddrToListen) == FALSE)
	{
		delete m_pListener;
		m_pListener = NULL;
		return FALSE;
	}

	CTask* pAcceptor = system_.GetTaskManager()->GetTask(TASK_SYS_ACCEPTOR);
	
	_ASSERT(pAcceptor != NULL);

	CServiceObject* pAcceptorService = pAcceptor->GetServiceObject();
	
	_ASSERT(pAcceptorService != NULL);

	if (m_pListener->Listen(pAcceptorService) == FALSE)
		return FALSE;

	return TRUE;
}

BOOL __stdcall CNetEngine::RegisterTask(DWORD dwTaskID, CTask* pTask, CRuntime* pRT_ServiceObj, ULONG_PTR Param)
{
	_ASSERT(pRT_ServiceObj != NULL);

	BOOL bCreatedHere = FALSE;

	if (pTask == NULL)
	{
		bCreatedHere = TRUE;
		pTask = new CTask;
		if (pTask->Create(this, system_.GetThreadManager(), pRT_ServiceObj, Param) == FALSE)
		{
			delete pTask;
			return FALSE;
		}
	}	

	if (system_.GetTaskManager()->RegisterTask(dwTaskID, pTask) == FALSE)
	{
		_ASSERT(FALSE);

		if (bCreatedHere == TRUE)
		{
			delete pTask;
		}
		
		return FALSE;
	}
	
	return TRUE;		
}

BOOL __stdcall CNetEngine::OverrideTask(DWORD TaskID, CRuntime* pRT_ServiceObj, DWORD dwParam, CTask* pTask)
{
	_ASSERT(FALSE);
	return FALSE;	
}

BOOL __stdcall CNetEngine::ActivateService(DWORD dwTaskID, long ThreadNumToSpawn, long nPriority, DWORD dwStackSize, DWORD dwFlags, ProcessorAffinity* pPA)
{	
	CTask* pTask = system_.GetTaskManager()->GetTask(dwTaskID);
	if (pTask == NULL)
		return FALSE;

	if (ThreadNumToSpawn <= 0)
	{		
		if (nProactorType == PROACTOR_IOCP)
		{
			SYSTEM_INFO sysinfo;
			::GetSystemInfo(&sysinfo);
			ThreadNumToSpawn = sysinfo.dwNumberOfProcessors * 2;
		}
		else		
			ThreadNumToSpawn = 1;			
	}

	CServiceObject* pServiceObject = pTask->GetServiceObject();
	if (pServiceObject == NULL)
		return FALSE;

	if (!pServiceObject->Activate(ThreadNumToSpawn))
		return FALSE;

	return pTask->StartJob(ThreadNumToSpawn, nPriority, dwStackSize, dwFlags, pPA);
}

BOOL __stdcall CNetEngine::ResumeService(DWORD dwTaskID)
{
	CTask* pTask = system_.GetTaskManager()->GetTask(dwTaskID);
	if (pTask == NULL)
		return FALSE;
	
	if (pTask->Resume() == TRUE)
		return TRUE;

	return FALSE;
}

BOOL __stdcall CNetEngine::SuspendService(DWORD dwTaskID)
{	
	CTask* pTask = system_.GetTaskManager()->GetTask(dwTaskID);
	if (pTask == NULL)
		return FALSE;
	
	if (pTask->Suspend() == TRUE)
		return TRUE;

	return FALSE;
}

BOOL __stdcall CNetEngine::KillTask(DWORD dwTaskID)
{
	CTask* pTask = GetTask(dwTaskID);
	if (pTask == NULL)
		return FALSE;
	
	CTaskManager* pTaskManager = system_.GetTaskManager();
	if (pTaskManager->RemoveTask(dwTaskID))
		return TRUE;
	
	return FALSE;
}

void __stdcall CNetEngine::SetSessionEncodingContext(DWORD dwSession, EncodeCtx& ec)
{
	CSession* pSession = m_SessionMgr.GetSession(dwSession);
	if (pSession != NULL)
	{
		if( pSession->SetEncodeContext(ec) == FALSE )
		{
			CSockStream* pSock = pSession->GetSocket();

			if( pSock && pSock->Disconnect( DISCONNECT_REASON_OVERLAPPED ) == FALSE )
				CloseSession( pSock->GetSessionID(), pSock->GetDisconnectReason() );	
		}
	}
	else
	{
		// 붙자 마자 끊어 져 버리거나 App가 로딩을 늦게 하거나 해서.. 연결 후 이 호출이 무지 하게 
		// 늦어 질 수도 있음

		//_ASSERTE(FALSE);
	}
}

long __stdcall CNetEngine::SendMsg(DWORD dwSession, CMsg* pMsg)
{	
	pMsg->ForceStopRead();
	
	CSession* pSession = m_SessionMgr.GetSession(dwSession);
	if (pSession != NULL)
	{
		long rcount_prev = pMsg->GetRefCnt();
		long Ret = pSession->SendMsg(pMsg);
		if (Ret != NERR_GOOD)
		{
			if (pMsg->GetRefCnt() > rcount_prev)	// Socket에서 AddRef 가 이루어졌다!
				DelMsg(pMsg);

			if (IS_CRITICAL(Ret))
				CloseSession(pSession->GetID());
		}
		
		return Ret;
	}
	return NERR_INVALID_SESSION;
}

CMsg* __stdcall CNetEngine::NewMsg(BOOL bEncrypt)
{	
	CMsg* pMsg = m_MsgPool.NewItem();	
	
	_ASSERT(pMsg != NULL);

	if (bEncrypt == TRUE)
		pMsg->SetAsEncrypted();

	pMsg->AddRef();

	_ASSERT(pMsg->GetRefCnt() == 1);
		
	return pMsg;
}

void __stdcall CNetEngine::DelMsg(CMsg* pMsg)
{
	_ASSERT(pMsg != NULL);	
	
	// netengine이 끝난 상태에서 (MsgPool이 Clear된 상태에서 이 함수가 호출 될 수 있음) 이강희
	if( IsShutdown())
		return;
	
	// msg reference count가 음수로 가게 되면 이눔이 재활용 될때엔 골때리는 상황이 생길 수 있잖냐?
	// NewMsg()했을때 RefCount가 1 이 아니라 0 이나 -1이 될 수 있다는 건데... 그런 상황이 발생하지 않도록
	// 오류 처리는 할망정 음수로 가는 것은 방지 했다.
	long ref_cnt = pMsg->GetRefCnt();
	if (ref_cnt <= 0)
	{
	}
	else
	{
		if (pMsg->DecRef() == 0)
		{	
			m_MsgPool.FreeItem(pMsg);

			if (pMsg->AllDataRead() == FALSE)
			{
			}
		}
	}
}

BOOL __stdcall CNetEngine::RecvFile(DWORD dwSession, std::wstring strFileFullPath, DWORD dwFileSize, long nOffset, FILE_TYPE file_type)
{
	CSession* pSession = m_SessionMgr.GetSession(dwSession);
	if (pSession != NULL)
	{
		if (file_type == FILE_TYPE_INVALID)
		{
			file_type = m_EngineConfig.FileTypeForRecv;
			_ASSERT(file_type != FILE_TYPE_INVALID);
		}
		
		return pSession->RecvFile(file_type, strFileFullPath, dwFileSize, nOffset);
	}
	
	return FALSE;
}

BOOL __stdcall CNetEngine::SendFile(DWORD dwSession, std::wstring& strFileFullPath, long nOffset, FILE_TYPE file_type)
{
	CSession* pSession = m_SessionMgr.GetSession(dwSession);
	if (pSession != NULL)
	{
		if (file_type == FILE_TYPE_INVALID)
			file_type = m_EngineConfig.FileTypeForSend;

		return pSession->SendFile(file_type, strFileFullPath, nOffset);
	}
	
	PutLog(LOG_FATAL, _T("화일을 보내려 했지만 해당 Session을 찾을 수 없다!"));
	return FALSE;
}

BOOL __stdcall CNetEngine::CloseSession( DWORD dwSession, DWORD dwReason /*= DISCONNECT_REASON_INTENTIONAL_KILL*/ )
{
	CSession* pSession = m_SessionMgr.GetSession(dwSession);
	if (pSession == NULL)
		return TRUE;

	return m_SessionMgr.FreeSession(pSession->GetSocket(), dwReason);
}

size_t __stdcall CNetEngine::CloseFile(DWORD dwSession, DWORD dwFileHandle)
{
	if (dwSession != FS_RESERVED_SYSTEM_SESSION_ID)
	{
		CSession* pSession = m_SessionMgr.GetSession(dwSession);
		if (pSession != NULL)
			return pSession->CloseFile();
	}
	
	return FILESYSTEM.CloseFile(dwFileHandle, dwSession);
}

void __stdcall CNetEngine::GetSysResourceUsage(DWORD dwMask, SYS_RES_USAGE& usage)
{	
	size_t Total;
	size_t Used;
	size_t Free;
	
	if (dwMask & SYSRES_USAGE_MASK_MSG)
	{
		m_MsgPool.GetUsage(Total, Used, Free);
		usage.UsageData[SYS_RES_USAGE::SYS_RES_USAGE_MSG].Total = Total;
		usage.UsageData[SYS_RES_USAGE::SYS_RES_USAGE_MSG].Used = Used;
		usage.UsageData[SYS_RES_USAGE::SYS_RES_USAGE_MSG].Free = Free;
	}

	if (dwMask & SYSRES_USAGE_MASK_SOCKET)
	{
		m_ActiveSocketPool.GetUsage(Total, Used, Free);
		m_PassiveSocketPool.GetUsage(Total, Used, Free);

		usage.UsageData[SYS_RES_USAGE::SYS_RES_USAGE_SOCKET].Total = Total;
		usage.UsageData[SYS_RES_USAGE::SYS_RES_USAGE_SOCKET].Used = Used;
		usage.UsageData[SYS_RES_USAGE::SYS_RES_USAGE_SOCKET].Free = Free;
	}
	
	if (dwMask & SYSRES_USAGE_MASK_SESSION)
	{
		m_SessionMgr.GetUsage(Total, Used, Free);

		usage.UsageData[SYS_RES_USAGE::SYS_RES_USAGE_SESSION].Total = Total;
		usage.UsageData[SYS_RES_USAGE::SYS_RES_USAGE_SESSION].Used = Used;
		usage.UsageData[SYS_RES_USAGE::SYS_RES_USAGE_SESSION].Free = Free;
	}

	if (dwMask & SYSRES_USAGE_MASK_FILESYS)
	{
		size_t MaxCache, CacheUsage;
		size_t HitPercentage;
		FILESYSTEM.GetStatistic(Total, Used, MaxCache, CacheUsage, HitPercentage);
		
		usage.UsageData[SYS_RES_USAGE::SYS_RES_USAGE_FILESYS].Total		 = Total;
		usage.UsageData[SYS_RES_USAGE::SYS_RES_USAGE_FILESYS].Used		 = Used;
		usage.UsageData[SYS_RES_USAGE::SYS_RES_USAGE_FILESYS].MaxCache	 = MaxCache;
		usage.UsageData[SYS_RES_USAGE::SYS_RES_USAGE_FILESYS].CacheUsed	 = CacheUsage;
		usage.UsageData[SYS_RES_USAGE::SYS_RES_USAGE_FILESYS].CacheHitPercentage = HitPercentage;
	}
	
	if (dwMask & SYSRES_USAGE_MASK_ACCEPT)
	{
		if (m_pListener != NULL)
		{
			usage.UsageData[SYS_RES_USAGE::SYS_RES_USAGE_ACCEPT].Total = GetConcurrentAcceptNum();
			usage.UsageData[SYS_RES_USAGE::SYS_RES_USAGE_ACCEPT].Used	 = CSockListener::s_nCurPostedAccept;
		}
	}

	if (dwMask & SYSRES_USAGE_MASK_SESSION_CLASSLINK)
	{
		static CSession temp;

//		usage.UsageData[SYS_RES_USAGE::SYS_RES_USAGE_SESSION_CLASSLINK].Used = temp.GetSessionCountOfClassLink();
	}

	return;
}

void __stdcall CNetEngine::Dump(DWORD dwDumpTargetMask, int count)
{
	if (dwDumpTargetMask & SYSRES_USAGE_MASK_MSG)
	{
#ifdef _DUMP_ALLOCATED_MSG
		m_MsgPool.Dump(count);		
#endif
	}
}

BOOL __stdcall CNetEngine::GetPeerIP(DWORD dwSession, std::wstring* pstrPeerIP, DWORD* pdwIP,WORD* pwPort)
{
	CSession* pSession = m_SessionMgr.GetSession(dwSession);
	if (pSession)
	{
		CSockStream* pSock = pSession->GetSocket();
		if (pSock != NULL)
		{
			sockaddr_in addr = pSock->GetPeerIp();
			
			if( pdwIP != NULL)
				*pdwIP = (DWORD)(addr.sin_addr.S_un.S_addr);
			
			if (pstrPeerIP != NULL)
			{
				//*pstrPeerIP = ::inet_ntoa((pSock->GetPeerIp()).sin_addr);
				CSockSystem::GetIP( (DWORD)(addr.sin_addr.S_un.S_addr), pstrPeerIP );
			}

			if( pwPort != NULL)
				*pwPort = ntohs( addr.sin_port);

			return TRUE;
		}
	}

	return FALSE;
}

bool __stdcall CNetEngine::SetLinkedSession(DWORD session_id, DWORD linked_session_id)
{
	CSession* session = m_SessionMgr.GetSession(session_id);
	if (session == nullptr)
		return false;

	CSession* linked_session = m_SessionMgr.GetSession(linked_session_id);
	if (linked_session == nullptr)
		return false;

	if (!session->AttachLinkedSession(linked_session))
		return false;

	return true;
}

bool __stdcall CNetEngine::UpdateReliableSession(DWORD session_id)
{
	CSession* session = m_SessionMgr.GetSession(session_id);
	if (session)
		return session->UpdateReliableSession();

	return false;
}

bool __stdcall CNetEngine::ResetReliableSession(DWORD session_id)
{
	CSession* session = m_SessionMgr.GetSession(session_id);
	if (session)
		return session->ResetReliableSession();

	return false;
}

void __stdcall CNetEngine::SkipMsgSecurity(DWORD session_id)
{
	CSession* session = m_SessionMgr.GetSession(session_id);
	if (session)
		session->SkipMsgSecurity();
}

void __stdcall CNetEngine::SetReliableSession(DWORD session_id, bool onoff)
{
	CSession* session = m_SessionMgr.GetSession(session_id);
	if (session)
	{		
		session->SetReliableSession(onoff);

		for each (SKeepAlive* keep_alive in keep_alive_sessions_)
		{
			if (keep_alive->register_session_id == session_id)
				keep_alive->reliable_session = onoff;
		}
	}
}

bool __stdcall CNetEngine::IsReliableSession(DWORD session_id)
{
	CSession* session = m_SessionMgr.GetSession(session_id);
	if (session == nullptr)
		return false;

	return session->IsReliableSession();
}

DWORD __stdcall CNetEngine::GetSessionType(DWORD dwSession)
{
	CSession* pSession = m_SessionMgr.GetSession(dwSession);
	if (pSession)
		return pSession->GetSessionType();
	
	return (DWORD)SESSION_TYPE_INVALID;
}

bool __stdcall CNetEngine::RemoveKeepAliveSessionEntry(DWORD session_id)
{
	SCOPED_LOCK_SINGLE(&m_CS);

	SESSIONS_TO_CONNECT::iterator it = keep_alive_sessions_.begin();
	for (; it != keep_alive_sessions_.end(); ++it)
	{
		SKeepAlive* keep_alive = *it;

		if (keep_alive->register_session_id == session_id)	// 음... 이넘 더이상 관리하지 말라구?
		{
			keep_alive_sessions_.erase(it);
			return true;
		}
	}

	return false;
}

void __stdcall CNetEngine::SetSessionSerialNumber(DWORD session_id, ULONGLONG serial_number)
{
	CSession* session = m_SessionMgr.GetSession(session_id);
	if (session != nullptr)
		session->set_serial_number(serial_number);
}

ULONGLONG __stdcall CNetEngine::GetSessionSerialNumber(DWORD session_id)
{
	CSession* session = m_SessionMgr.GetSession(session_id);
	if (session != nullptr)
		return session->serial_number();

	return 0;
}

bool __stdcall CNetEngine::SetSessionMsgSizeLimit(DWORD session_id, DWORD msg_size_limit)
{
	return m_SessionMgr.SetMsgSizeLimit(session_id, msg_size_limit);
}

bool __stdcall CNetEngine::SetSessionSendQueDepth(DWORD session_id, DWORD send_que_depth)
{
	return m_SessionMgr.SetSendQueDepth(session_id, send_que_depth);
}

SKeepAlive* __stdcall CNetEngine::AddActiveSessionKeepAlive(LPCWSTR lpszIP,WORD nPort,LPCWSTR lpszBindIP,DWORD dwTargetTask,DWORD dwAssocSessionID)
{
	DWORD ip_bind = CSockSystem::GetAddressFromName(lpszBindIP);
	DWORD ip = CSockSystem::GetAddressFromName(lpszIP);
	if (ip == INADDR_NONE || ip_bind == INADDR_NONE)
		return nullptr;

	BSSOCKADDR_IN addr_connect;
	BSSOCKADDR_IN addr_bind;
	::ZeroMemory(&addr_connect, sizeof(addr_connect));
	::ZeroMemory(&addr_bind, sizeof(addr_bind));
	
	addr_connect.sin_family	= AF_INET;
	addr_connect.sin_port	= htons(nPort);
	addr_connect.sin_addr	= ip;

	BSSOCKADDR_IN* pAddrBind = NULL;
	if (addr_bind.sin_addr != 0)
		pAddrBind = &addr_bind;

	SKeepAlive* newone = new SKeepAlive(&addr_connect, pAddrBind, dwTargetTask, dwAssocSessionID);
	if (AddActiveSessionKeepAlive(*newone) == false)
	{
		delete newone;
		return nullptr;
	}

	return newone;
}

///////////////////////////////////////////////////////////////////////////
// not exported
///////////////////////////////////////////////////////////////////////////
CServiceObject* CNetEngine::GetServiceObject(DWORD dwTaskID)
{
	CTask* pTask = system_.GetTaskManager()->GetTask(dwTaskID);	
	if (pTask == NULL)
		return NULL;

	return pTask->GetServiceObject();	
}

BOOL CNetEngine::PostNewAccept()
{	
	CSockStream* pNewSock = AllocPassiveNewSock();	
	if (pNewSock)
	{
		if (pNewSock->CreatePassiveSession(this, m_pListener->GetHandle()) == FALSE)
		{
			FreePassiveSocket( pNewSock );
			return FALSE;
		}

		return TRUE;		
	}	
	return FALSE;
}

void CNetEngine::FreeTypeSocket(CSockStream* pSock)
{
	if (pSock != NULL)
	{	
		SESSION_TYPE type = pSock->GetSessionType();

		pSock->Close();

		if( type == SESSION_TYPE_PASSIVE )
		{
			if( m_pListener->PostReuseAccept(pSock) == FALSE )
			{
				m_PassiveSocketPool.FreeItem(pSock);
			}
		}
		else if( type == SESSION_TYPE_ACTIVE )
		{
			m_ActiveSocketPool.FreeItem(pSock);
		}
	}
}

void CNetEngine::FreeActiveSocket(CSockStream* pSock)
{	
	if (pSock != NULL)
	{	
		pSock->Close();

		m_ActiveSocketPool.FreeItem(pSock);
	}
}

void CNetEngine::FreePassiveSocket(CSockStream* pSock)
{
	if (pSock != NULL)
	{	
		pSock->Close();

		m_PassiveSocketPool.FreeItem(pSock);
	}
}

void CNetEngine::FreeDGramSocket( CSockDatagram* pSock )
{
	if( pSock != NULL )
	{
		pSock->Close();
		m_DGramSocketPool.FreeItem( pSock );
	}
}

BOOL CNetEngine::DetermineProactorType()
{
	OSVERSIONINFOEX osvi;
	::ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));

	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

	BOOL bOsVersionInfoEx = GetVersionEx( (OSVERSIONINFO*)&osvi );
	if( !bOsVersionInfoEx )
	{
		osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
		if (!GetVersionEx((OSVERSIONINFO*) &osvi)) 
			return PROACTOR_INVALID;
	}

	if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT)
		return PROACTOR_IOCP;
	else
		return PROACTOR_EVENT;
}

void CNetEngine::GetPhyMemorySize(size_t& TotalPhyMem, size_t& AvailablePhyMem)
{
	MEMORYSTATUS MemStatus;
	::GlobalMemoryStatus(&MemStatus);

	TotalPhyMem = MemStatus.dwTotalPhys;
	AvailablePhyMem = MemStatus.dwAvailPhys;
}

bool CNetEngine::AddActiveSessionKeepAlive(SKeepAlive& new_keep_alive)
{
	SCOPED_LOCK_SINGLE(&m_CS);

	for each (const SKeepAlive* keep_alive in keep_alive_sessions_)
	{
		if (*keep_alive == new_keep_alive)
			return false;
	}

	keep_alive_sessions_.push_back(&new_keep_alive);
	return true;
}

void CNetEngine::ConnectOfflinedKeepAliveSessions()
{	
	SCOPED_LOCK_SINGLE(&m_CS);

	DWORD dwSessionID = NULL;
	std::wstring session_ip;

	SESSIONS_TO_CONNECT::iterator it = keep_alive_sessions_.begin();
	SESSIONS_TO_CONNECT::iterator it_end = keep_alive_sessions_.end();
	for (; it != it_end; ++it)
	{
		SKeepAlive* ka = *it;
		if (ka->assoc_session_id)
			continue;

		// 만약 KeepAlive = TRUE 로 넘기면 실패할 경우 리스트에 중복 추가될 거 아니냐...
		dwSessionID = Connect(&ka->addrConnect, (ka->addrBind.sin_addr == 0) ? NULL : &ka->addrBind, ka->dwTaskToBind, TRUE);

		if (dwSessionID == 0)
		{
			PutLog(LOG_FATAL_FILE, _T("cannot establish keep alive session : %d.%d.%d.%d %d"),
				(ka->addrConnect.sin_addr)& 0xFF,
				(ka->addrConnect.sin_addr >> 8)& 0xFF,
				(ka->addrConnect.sin_addr >> 16)& 0xFF,
				(ka->addrConnect.sin_addr >> 24)& 0xFF,
				htons(ka->addrConnect.sin_port));

			continue;
		}

		if (ka->register_session_id == 0)
			ka->register_session_id = dwSessionID;

		ka->assoc_session_id = dwSessionID;		
	}

	m_SessionMgr.CloseKeepAliveSessions(keep_alive_sessions_);
}

long CNetEngine::RemoveStaleSessions(DWORD interval, DWORD dwTimeout)
{
	_ASSERT(m_EngineConfig.MC.target_flags & MT_MASK_STALE_SESSIONS);
	return m_SessionMgr.CloseStaleSessions(interval, dwTimeout);
}

void CNetEngine::ResetFileSystem()
{
	FILESYSTEM.Cleanup();
}

// BSLib가 아직 유니코드가 아니기땜시.. LPCTSTR을 하면 다른곳에 포함을 못시킨다 by novice
void PutLog(DWORD logtype, LPCTSTR format, ...)
{
	NETENGINE_CONFIG& ec = CNetEngine::GetEngineConfig();
	if (ec.lpfReportLog != NULL)
	{
		__try
		{
			TCHAR buf[1024];

			va_list ap;
			va_start(ap, format);
			if (SF_vsprintf(buf, _countof(buf), format, ap) != S_OK)
			{
				_ASSERT(FALSE);
			}
			va_end(ap);

			(*ec.lpfReportLog)(logtype, _T("%s"), buf);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
		}
	}
}

extern bool g_bTerminateProcess;
void __stdcall CNetEngine::SetTerminateProcess(bool bEnable)
{
	g_bTerminateProcess = bEnable;
}

void __stdcall CNetEngine::CloseAllPassiveSession()
{
	SCOPED_LOCK_SINGLE(&m_CS);

	m_SessionMgr.CloseAllPassiveSession();
}

void* __stdcall CNetEngine::GetMsgGenerator()
{
	return &m_MsgPool;
}

BOOL __stdcall CNetEngine::GetMacAddress( DWORD dwSession, BYTE* pAddress )
{
	CSession* pSession = m_SessionMgr.GetSession(dwSession);
	
	if( pSession && pAddress )
	{
		CSockStream* pSock = pSession->GetSocket();
		if( pSock != NULL )
		{
//			sockaddr_in& addr = pSock->GetLocalIp();
		
			std::string pstrLocalIP;
			
			pstrLocalIP = ::inet_ntoa((pSock->GetLocalIp()).sin_addr);
			
			IP_ADAPTER_INFO *pAdapterInfo = NULL;

			IP_ADAPTER_INFO *pTmpInfo = NULL;

			ULONG ulSizeAdapterInfo = 0;

			if( GetAdaptersInfo(pAdapterInfo,&ulSizeAdapterInfo) == ERROR_BUFFER_OVERFLOW )
			{
				pAdapterInfo = new IP_ADAPTER_INFO[ ulSizeAdapterInfo ];
				if( pAdapterInfo == NULL )
				{
					return FALSE;	
				}

				GetAdaptersInfo( pAdapterInfo, &ulSizeAdapterInfo );
			}
			
			pTmpInfo = pAdapterInfo;

			while( pTmpInfo != NULL )
			{ 
				if( pTmpInfo->AddressLength == 6 && MIB_IF_TYPE_ETHERNET == pTmpInfo->Type)
				{
					// check ip list to find out same ip.
					IP_ADDR_STRING* pAddressString = &pTmpInfo->IpAddressList;
					do 
					{
						if(strcmp (pstrLocalIP.c_str(), pAddressString->IpAddress.String) == 0)
						{
							memcpy(pAddress, pTmpInfo->Address, 6);

							delete [] pAdapterInfo;
							return TRUE;
						}

						pAddressString = pAddressString->Next;

					} while(pAddressString != NULL);
				}

				pTmpInfo = pTmpInfo->Next;
			}

			delete [] pAdapterInfo;

			return FALSE;
		}
	}

    return FALSE;
}

BOOL CNetEngine::DisconnectCheck( CSockStream* pSock, DWORD dwReason )
{
	if( m_pListener ) 
		return m_pListener->DisconnectCheck( pSock, dwReason );
	else
		return CloseSession(pSock, dwReason);	
}

void CNetEngine::SetSecurityMode( BYTE mode )
{
	m_EngineConfig.SecurityMode = mode;
}

void CNetEngine::DisplayEngineVer( void )
{
	PutLog( LOG_NOTIFY_FILE, _T("netengine(ver %0.2f)"), NetEngine_Ver );
}

/*******************************************************************************************************
//P2P Process Initialize!
*******************************************************************************************************/
BOOL __stdcall CNetEngine::StartPeerTask( DWORD dwTaskID )
{
	return m_PeerNet.StartPeerTask( this, dwTaskID );
}

long __stdcall CNetEngine::StartPeerNetService( WORD port )
{
	if( m_PeerNet.StartPeerNetService(port) == TRUE )
		return NERR_GOOD;

	return NERR_UNKNOWN;
}

/*
long __stdcall CNetEngine::SendPeerMsg( SOCKADDR_IN& addr, CMsg* pMsg )
{
	return m_PeerNet.SendPeerMsg( addr, pMsg );
}


BOOL __stdcall CNetEngine::GetPeerNetSockAddr( SOCKADDR_IN& addr )
{
	return m_PeerNet.GetPeerNetSockAddr( addr );
}
*/

void __stdcall CNetEngine::ClosePeerNet()
{
	m_PeerNet.Finalize();
}

BOOL __stdcall CNetEngine::SetNoDelay( DWORD dwSession, BOOL bNoDelay )
{
	CSession* pSession = m_SessionMgr.GetSession( dwSession );
	if( pSession == NULL )
		return FALSE;

	CSockStream* pSocket = pSession->GetSocket();
	if( pSocket == NULL )
		return FALSE;

	return CSockSystem::SetNoDelay( pSocket->GetHandle(), bNoDelay );
}

