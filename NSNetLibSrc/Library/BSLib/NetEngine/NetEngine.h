#pragma once

#include "sockettcp.h"
#include "SessionMgr.h"
#include "system.h"

#include "PeerNet.h"
#include "../Util/ChunkAllocatorCMsg.h"

////////////////////////////////////////////////
// supported types of proactor
////////////////////////////////////////////////
enum
{
	PROACTOR_INVALID = -1,
	
	PROACTOR_IOCP = 0,
	PROACTOR_EVENT,
	
	PROACTOR_TYPENUM
};


////////////////////////////////////////////////
// connection data to keep alive
////////////////////////////////////////////////

#define CLIENT_SIDE_MSGPOOL_CHUNKSIZE		500
#define CLIENT_SIDE_SOCKET_POOL_CHUNKSIZE	50	// 혹시... 데이터들 정리되기 전에 세션ID 할당 받아서 엉킬까봐... 쪼금 여유있게...

#ifdef _DEBUG
	#define SERVER_SIDE_MSGPOOL_CHUNKSIZE		30
	#define SERVER_SIDE_SOCKET_POOL_CHUNKSIZE	50
#else
	#define SERVER_SIDE_MSGPOOL_CHUNKSIZE		3000
	#define SERVER_SIDE_SOCKET_POOL_CHUNKSIZE	5000
#endif

class CNetEngine : public IBSNet
{
public:
	CNetEngine(void);
	virtual ~CNetEngine(void);

private:
	long m_cRef;

	static NETENGINE_CONFIG m_EngineConfig;

public:
	//IUnknown implementation
	virtual HRESULT __stdcall QueryInterface(const IID& iid, void** ppv);
	virtual ULONG	__stdcall AddRef() { /*return InterlockedIncrement(&m_cRef); */ return 0;}
	virtual ULONG	__stdcall Release();

	virtual BOOL	__stdcall Create(NETENGINE_CONFIG Config);
	virtual DWORD	__stdcall Connect(LPCWSTR lpszAddrConnect,WORD nPort,LPCWSTR lpszAddrBind, DWORD dwTaskToBind, BOOL bKeepAlive, bool reconnect = false, DWORD connect_timeout = 3);
	virtual DWORD	__stdcall Connect(BSSOCKADDR_IN* addr_connect, BSSOCKADDR_IN* addr_bind, DWORD dwTaskToBind = 0, BOOL bKeepAlive = FALSE, DWORD connect_timeout = 3);
	virtual bool	__stdcall Ping(LPCTSTR domain, WORD port, DWORD& time);
	virtual BOOL	__stdcall StartServer(LPCWSTR lpszAddr, SHORT sPort);
	virtual BOOL	__stdcall StartServer(BSSOCKADDR_IN* pAddrToListen);
	virtual BOOL	__stdcall RegisterTask(DWORD dwTaskID, CTask* pTask, CRuntime* pRT_ServiceObj, ULONG_PTR Param);
	virtual BOOL	__stdcall OverrideTask(DWORD TaskID, CRuntime* pRT_ServiceObj, DWORD dwParam, CTask* pTask);
	virtual BOOL	__stdcall ActivateService(DWORD dwTaskID, long ThreadNumToSpawn, long nPriority, DWORD dwStackSize, DWORD dwFlags, ProcessorAffinity* pPA);
	virtual BOOL	__stdcall ResumeService(DWORD dwTaskID);
	virtual BOOL	__stdcall SuspendService(DWORD dwTaskID);
	virtual BOOL	__stdcall KillTask(DWORD dwTaskID);
	virtual void	__stdcall GetSysResourceUsage(DWORD dwMask, SYS_RES_USAGE& usage);
	virtual BOOL	__stdcall GetPeerIP(DWORD dwSession, std::wstring* pstrPeerIP, DWORD* pdwIP,WORD* pwPort);
	virtual CTask*	__stdcall GetTask(DWORD dwTaskID) { return system_.GetTaskManager()->GetTask(dwTaskID); }	

	virtual void	__stdcall SetSessionEncodingContext(DWORD dwSession, EncodeCtx& ec);
	virtual CMsg*	__stdcall NewMsg(BOOL bEncrypt = FALSE);
	virtual void	__stdcall DelMsg(CMsg* pMsg);
	virtual long	__stdcall SendMsg(DWORD dwSession, CMsg* pMsg);	
	virtual BOOL	__stdcall RecvFile(DWORD dwSession, std::wstring strFileFullPath, DWORD dwFileSize, long nOffset = 0, FILE_TYPE file_type = FILE_TYPE_INVALID);
	virtual BOOL	__stdcall SendFile(DWORD dwSession, std::wstring& strFileFullPath, long nOffset = 0, FILE_TYPE file_type = FILE_TYPE_INVALID);
	virtual BOOL	__stdcall CloseSession(DWORD dwSession, DWORD dwReason = DISCONNECT_REASON_INTENTIONAL_KILL);
	virtual size_t	__stdcall CloseFile(DWORD dwSession, DWORD dwFileHandle = NULL);
	virtual bool	__stdcall SetLinkedSession(DWORD session_id, DWORD linked_session_id);
	virtual void	__stdcall SkipMsgSecurity(DWORD session_id);
	virtual void	__stdcall SetReliableSession(DWORD session_id, bool onoff);
	virtual bool	__stdcall UpdateReliableSession(DWORD session_id);
	virtual bool	__stdcall ResetReliableSession(DWORD session_id);
	virtual bool	__stdcall IsReliableSession(DWORD session_id);
	virtual DWORD	__stdcall GetSessionType(DWORD dwSession);
	virtual bool	__stdcall RemoveKeepAliveSessionEntry(DWORD dwSession);
	virtual ULONGLONG __stdcall GetSessionSerialNumber(DWORD session_id);
	virtual void	__stdcall SetSessionSerialNumber(DWORD session_id, ULONGLONG serial_number);	
	virtual bool	__stdcall SetSessionMsgSizeLimit(DWORD session_id, DWORD msg_size_limit);
	virtual bool	__stdcall SetSessionSendQueDepth(DWORD session_id, DWORD send_que_depth);
	virtual SKeepAlive*	__stdcall AddActiveSessionKeepAlive(LPCWSTR lpszIP,WORD nPort,LPCWSTR lpszBindIP,DWORD dwTargetTask,DWORD dwAssocSessionID = 0);
	virtual void	__stdcall ResetFileSystem();

	virtual void	__stdcall Dump(DWORD dwDumpTarget, int count = -1);
	
	virtual void	__stdcall SetTerminateProcess(bool bEnable);
	virtual void	__stdcall CloseAllPassiveSession();
	virtual void*	__stdcall GetMsgGenerator();
	
	virtual BOOL	__stdcall GetMacAddress( DWORD dwSession, BYTE* pAddress );

	virtual void	__stdcall SetSecurityMode( BYTE mode );
	
	virtual void	__stdcall DisplayEngineVer( void );

	//PEER NET
	virtual BOOL	__stdcall StartPeerTask( DWORD dwTaskID );
	virtual long	__stdcall StartPeerNetService( WORD port );
	//virtual long	__stdcall SendPeerMsg( SOCKADDR_IN& addr, CMsg* pMsg );
	//virtual BOOL	__stdcall GetPeerNetSockAddr( SOCKADDR_IN& addr );
	virtual void	__stdcall ClosePeerNet() override;
	virtual BOOL	__stdcall SetNoDelay( DWORD dwSession, BOOL bNoDelay ) override;

public:
	BOOL			PostNewAccept();
	void			FreeTypeSocket(CSockStream* pSock);
	void			FreeActiveSocket(CSockStream* pSock);
	void			FreePassiveSocket(CSockStream* pSock);
	void			FreeDGramSocket( CSockDatagram* pSock );
	
	CServiceObject* GetServiceObject(DWORD dwTaskID);
	CSockStream*	AllocActiveNewSock() { return m_ActiveSocketPool.NewItem(); }
	CSockStream*	AllocPassiveNewSock() { return m_PassiveSocketPool.NewItem(); }
	CSockDatagram*	AllocDGramNewSock(){ return m_DGramSocketPool.NewItem(); }
	
	DWORD			GetAcceptTimeoutSec() { return m_EngineConfig.dwAcceptTimeout; }
	DWORD			GetConcurrentAcceptNum() { return m_EngineConfig.dwConcurrentAcceptPosting; }
	DWORD			CreateNewSession(CSockStream* pSockToBind, DWORD dwTaskToBind) { return m_SessionMgr.CreateNewSession(pSockToBind, dwTaskToBind); }
	DWORD			GetDefaultTaskID(SESSION_TYPE session_type) { return m_EngineConfig.SessionConfig[session_type].dwDefaultTask; }
	SESSION_CONFIG*	GetSessionConfig(SESSION_TYPE session_type) { return &m_EngineConfig.SessionConfig[session_type]; }
	IO_MODE			GetIOMode(SESSION_TYPE session_type) { return m_EngineConfig.SessionConfig[session_type].IOMode; }
	static NETENGINE_CONFIG& GetEngineConfig() { return m_EngineConfig; }
	
	// functions for maintainer
	void			ConnectOfflinedKeepAliveSessions();
	long			RemoveStaleSessions(DWORD interval, DWORD dwTimeout);

	// rhymes
	BOOL			DisconnectCheck( CSockStream* pSock, DWORD dwReason );
	BOOL			CloseSession(CSockStream* pSock, DWORD dwReason) { return m_SessionMgr.FreeSession(pSock, dwReason); }
	
protected:
	bool			AddActiveSessionKeepAlive(SKeepAlive& ka);

protected:
	CSystem								system_;

	CCriticalSectionBS					m_CS;

	CSockListener*						m_pListener;
	
	ChunkAllocatorMT<CSockStream>		m_ActiveSocketPool;
	ChunkAllocatorMT<CSockStream>		m_PassiveSocketPool;
	ChunkAllocatorMT<CSockDatagram>	m_DGramSocketPool;

	CSessionMgr							m_SessionMgr;
	CPeerNet							m_PeerNet;

	CChunkAllocatorMT_For_CMsg<CMsg>	m_MsgPool;

	SESSIONS_TO_CONNECT					keep_alive_sessions_;


protected:
	long				nProactorType;
	BOOL				bShutdown;

	BOOL			DetermineProactorType();
	void			GetPhyMemorySize(size_t& TotalPhyMem, size_t& AvailablePhyMem);
	
public:
	CSystem&			system() { return system_; }

	long			GetProactorType() { return nProactorType; }
	BOOL			IsShutdown() { return bShutdown; }
};
