#pragma once
/*///////////////////////////////////////////////////////////////////////
								CEventHandler
									|
								CBaseSocket
			|-----------------------|------------------------|
		CSockListner			CSockStream				CSockDatagram
									
///////////////////////////////////////////////////////////////////////*/

#include "IOContext.h"
#include "SockSystem.h"
#include "KeyGenerator.h"
#include "../security/blowfish.h"
#include "../security/keyexchange.h"

class CNetEngine;

//////////////////////////////////////////////////////////////////
// CBaseSocket
//////////////////////////////////////////////////////////////////
class CBaseSocket : public CEventHandler
{
public:
	enum eSockType
	{
		eST_Stream,
		eST_DGram,
		eST_Listener,
		eST_Count,
		eST_Invalid = -1
	};

protected:
	CBaseSocket();
	virtual ~CBaseSocket();

protected:
	SOCKET				m_hSocket;
	SESSION_TYPE		m_SessionType;
	
	BOOL				m_bGracefulShutdown;
	CCriticalSectionBS	m_CS;	
	eSockType			m_Type;

	CNetEngine*			engine_;
			
protected:
	virtual void	Reset()
	{
	}

	BOOL			Create(CNetEngine* engine, SESSION_TYPE session_type);

public:
	virtual long	Close();
	BOOL	IsValidSock() 
	{ 
		if (this == NULL)
			return FALSE;

		return (m_hSocket != INVALID_SOCKET); 
	}

	SOCKET			GetHandle() { return m_hSocket; }

	eSockType		GetType(){ return m_Type;}

	sockaddr_in		GetLocalIp();
	unsigned short	GetLocalPort();

	CNetEngine* engine() const { return engine_; }
};

//////////////////////////////////////////////////////////////////
// CSockStream
//////////////////////////////////////////////////////////////////
class CMsg;
class IFile;
class CSession;

class CSockStream : public CBaseSocket
{
public:
	CSockStream();
	virtual ~CSockStream();

protected:
	BSSOCKADDR_IN	m_addrPeer;
	
	CIOContext		m_ctxWrite;
	CIOContext		m_ctxRead;
	CIOContext		m_ctxDisconnect;
			
	ULONGLONG		m_latest_access_tick;	// keep latest recv occur to check stale session

	DWORD			m_dwdisconnectreason;
	
	CMsg*			m_pPendingSend;
	CMsg*			m_pPendingRecv;
	CSession*		m_pSession;	
    
	CQueMT<CMsg*>	m_SendingQue;

	long			m_dwCurSentBytes;
	DWORD			m_dwCurRecvBytes;

	CTask*			m_pTask;

	////////////////////////////////////////// msg protector
	SESSION_STATE	m_SessionState;

	CBlowFish			m_BF;
	CRC32				m_Crc;	
	CSeqNumGenerator	m_SeqGenerator;
	
	BYTE			m_cMsgEncodingOption;
	
	// new security
	CKeySender		m_Sender;
	CKeyRepicient	m_Recipient;

	// seucurity
	BYTE			m_HandShake;
	
	DWORD msg_size_limit_;
	DWORD send_queue_depth_;
	bool msg_security_;
	DWORD disconnect_error_code_;
	
	int send_buffer_size_;
	int	recv_buffer_size_;

	////////////////////////////////////////// completion handlers
	long			(CSockStream::*FP_EVENTHANDLER[IOTYPE_NUM][IO_TARGET_NUM])(IIOContext* pContext, DWORD dwTransferred);

	long			(CSockStream::*FP_RECVED_MSG_HANDLER)(CMsg* pRecvedMsg);

	long			(CSockStream::*FP_INVOKER_SENDDATA)(CMsg* pMsg);

private:
	BOOL			SecurityModeCheck( void );	
	
protected:
	virtual BOOL	RegisterHandleTo(CServiceObject* pServiceObj, HANDLE hHandle);	
	
	void			Invalidate();

	BOOL			CreateWSAEvent();

	BOOL			GetMsgToSend();
	BOOL			CheckEncodingOption(BYTE flag) { return (m_cMsgEncodingOption & flag); }

	bool			PrepareSocket();
	bool			CreateBuffer();
	
	// msg coder / decoder
	void			_SetEncodeCtx(EncodeCtx& ec);

	BOOL			ProtectMsg(CMsg*& pMsg);
	DWORD			NeutralizeMsg(DWORD& DataSize);

	// msg forger
	long			_ForgeMsgToSend(CMsg*& pMsg);
	
	// send
	long			_SendMsgDirect(CMsg* pMsg);
	long			_SendMsgBuffered(CMsg* pMsg);
	
	long			_OnDataSentDirect(IIOContext* pContext, DWORD dwTransferred);
	long			_OnDataSentBuffered(IIOContext* pContext, DWORD dwTransferred);
	long			_OnFileSent(IIOContext* pContext, DWORD dwTransferred);

	// recv
	long			_OnDataRecved(IIOContext* pContext, DWORD dwTransferred);

	long			_WriteFileData(CMsg* pFileMsg);

	long			_OnMsgReceivedBeforeHandshake(CMsg* pRecvMsg);
	long			_OnMsgReceivedAfterHandshake(CMsg* pRecvMsg);
	void			_SessionHandshaked( void );
	long			_MsgReceived(CMsg* pRecvedMsg);

	// posting
	long			PostSend(IIOContext* pIOContext);
	long			PostRecv();

public:
	void			Init();
	virtual long	Close() override;

	void			TimeStamp()
	{
#ifdef SERVER_BUILD
		m_latest_access_tick = GetTickCount64();
#endif
	}

	ULONGLONG		GetLatestAccessTick() const { return m_latest_access_tick; }
	BOOL			LetsRock(CServiceObject* pServiceObject) { return RegisterHandleTo(pServiceObject, (HANDLE)GetHandle()); }
	
	sockaddr_in		GetPeerIp()						{ return *(struct sockaddr_in*)&m_addrPeer; }
	void			SetSession(CSession* pSession)	{ m_pSession = pSession; }
	CSession*		GetSession()					{ return m_pSession; }
	DWORD			GetSessionID();
	void			SetCurTask(CTask* pTask)		{ _ASSERT(pTask != NULL); SCOPED_LOCK_SINGLE(&m_CS); m_pTask = pTask; }
	CIOContext*		GetContext(long nOP);
	DWORD			GetElapsedTick(ULONGLONG current_tick) const { return static_cast<DWORD>(current_tick - m_latest_access_tick); }
	SESSION_TYPE	GetSessionType() { return m_SessionType; }
	BOOL			IsLiveSession(int& nEstablishedSeconds);

	BOOL			CreateActiveSession(CNetEngine* engine, BSSOCKADDR_IN* addr_connect, BSSOCKADDR_IN* addr_bind = NULL,BOOL bForKeepAlive = FALSE, DWORD connect_timeout = 3);
	BOOL			CreatePassiveSession(CNetEngine* engine, SOCKET hListener);
	long			OnAccepted(SOCKET hListener);
	
	long			SendMsg(CMsg* pMsg);
	long			SendFile(IFile* pFile, DWORD dwOffset);
	long			RecvFile();

	BOOL			ProtectSession(EncodeCtx& ec);
	bool			MigrateSession(CSession* session);
	
	virtual long	HandleEvent(DWORD dwTransferred, void* pContext);

	BYTE			Disconnect(DWORD dwDisconnectReason);
	DWORD			GetDisconnectReason() { return m_dwdisconnectreason; }
	void			SetDisconnectReason( DWORD dwDisconnectReason );

	size_t GetSendQueSize() { return m_SendingQue.GetSize(); }

	void set_msg_size_limit(DWORD msg_size_limit) { msg_size_limit_ = msg_size_limit; }
	void set_send_queue_depth(DWORD send_queue_depth) { send_queue_depth_ = send_queue_depth; }
	void SkipMsgSecurity() { msg_security_ = false; }
	
	void set_disconnect_error_code(DWORD disconnect_error_code) { disconnect_error_code_ = disconnect_error_code; }
	DWORD disconnect_error_code() const { return disconnect_error_code_; }

	int send_buffer_size() const { return send_buffer_size_; }
	int recv_buffer_size() const { return recv_buffer_size_; }
			
	friend class CSession;
};

inline CIOContext* CSockStream::GetContext(long nOP)
{
	switch (nOP)
	{
	case IO_READ:
		return &m_ctxRead;
	case IO_WRITE:
		return &m_ctxWrite;
	default:
		_ASSERT(0);
	}
	return NULL;
}

//////////////////////////////////////////////////////////////////
// CSockListener
//////////////////////////////////////////////////////////////////
class CSockListener : public CBaseSocket
{
public:
	CSockListener();
	virtual ~CSockListener();

protected:
	long			m_nBackLog;

	std::list<CSockStream*> m_socketlist;

	CCriticalSectionBS	m_AcceptCS;

public:
	static	long	s_nCurPostedAccept;
		
public:
	virtual long	Close();
	virtual long	HandleEvent(DWORD dwTransferred, void* pContext);
	virtual BOOL	RegisterHandleTo(CServiceObject* pServiceObj, HANDLE hHandle);

	BOOL			Create(CNetEngine* engine, BSSOCKADDR_IN* addr_bind, int backlog = DEFAULT_BACK_LOG);
	BOOL			Listen(CServiceObject* pServiceObj);
	long			Accept(CSockStream* pSock);
	
	BOOL			PostReuseAccept(CSockStream* pSock);
	BOOL			DisconnectCheck( CSockStream* pSock, DWORD dwReason );
	
protected:
	void			PostNewAccept();
};










































