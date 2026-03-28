#include "StdAfx.h"
#include "sockettcp.h"
#include "NetEngine.h"

//#define SINGLE_POST_ACCEPT

///////////////////////////////////////////////////////////////
// Socket Base
///////////////////////////////////////////////////////////////
CBaseSocket::CBaseSocket() : m_CS(TRUE), engine_(nullptr)
{
	m_hSocket = INVALID_SOCKET;
	m_Type	  = eST_Invalid;
	m_bGracefulShutdown = FALSE;
	m_SessionType = SESSION_TYPE_INVALID;	
}

CBaseSocket::~CBaseSocket()
{
	CBaseSocket::Close();
}

long CBaseSocket::Close() 
{ 
	if (m_hSocket != INVALID_SOCKET) 
	{ 
		::closesocket(m_hSocket); 
		m_hSocket = INVALID_SOCKET; 
	} 
	
	return 0; 
}

BOOL CBaseSocket::Create(CNetEngine* engine, SESSION_TYPE session_type)
{
	_ASSERT(session_type == SESSION_TYPE_ACTIVE || session_type == SESSION_TYPE_PASSIVE);

	m_SessionType = session_type;

	if( m_hSocket == INVALID_SOCKET )
	{
		m_hSocket = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED); 
		if (m_hSocket == INVALID_SOCKET)
		{
			WSAERROR(TEXT("WSASocket()"), TEXT("CBaseSocket::Create"));
			goto __ERROR;
		}
	}
	
	/*
	switch (pSessionConfig->nBufOption)
	{
	case SOCKBUF_OPTION_DEFAULT:		
		break;
	case SOCKBUF_OPTION_TURNOFF:
		CSockSystem::TurnOffBuffer(m_hSocket, pSessionConfig->bApplyBufOptionForSend, pSessionConfig->bApplyBufOptionForRecv);
		break;
	case SOCKBUF_OPTION_MAXIMUM:
		if (pSessionConfig->bApplyBufOptionForSend)
			CSockSystem::SetMaxBuffer(m_hSocket, MAX_BUFFER_SIZE, SO_SNDBUF);
		if (pSessionConfig->bApplyBufOptionForRecv)
			CSockSystem::SetMaxBuffer(m_hSocket, MAX_BUFFER_SIZE, SO_RCVBUF);
		break;

	default:
		goto __ERROR;
		break;
	}
			
	if (m_bGracefulShutdown == FALSE)
	{
		if (CSockSystem::SetLinger(m_hSocket, TRUE, 0) == FALSE)
			goto __ERROR;
	}
	*/

	SESSION_CONFIG* pSessionConfig = engine->GetSessionConfig(session_type);
	m_bGracefulShutdown = pSessionConfig->bGracefulShutdown;
	engine_ = engine;

	return TRUE;

__ERROR:
	CBaseSocket::Close();
	return FALSE;
}

sockaddr_in CBaseSocket::GetLocalIp()
{
	sockaddr_in addr;
	int nLen = sizeof(addr);
	ZeroMemory(&addr, nLen);	

	::getsockname( GetHandle(), (struct sockaddr*)&addr, &nLen );
	return addr;
}

unsigned short CBaseSocket::GetLocalPort()
{
	sockaddr_in sa;
	int len = sizeof( sa );

	if( ::getsockname(m_hSocket,(sockaddr*)&sa,&len) != 0 )
		return 0x0000;

	return ::ntohs( sa.sin_port );
}
/////////////////////////////////////////////////////////////
//	CSockStream 
/////////////////////////////////////////////////////////////
CSockStream::CSockStream() : CBaseSocket(), m_ctxWrite(IO_WRITE), m_ctxRead(IO_READ), m_ctxDisconnect( IO_DISCONNECT )
{
	m_pPendingSend	= NULL;
	m_pPendingRecv	= NULL;
	m_pSession		= NULL;

	Init();

	m_dwdisconnectreason = NULL;

	m_ctxWrite.SetOwner(this);	// 얘는 Buffered Mode 에서만 사용되지만...
	m_ctxRead.SetOwner(this);
	m_ctxDisconnect.SetOwner(this);
	m_ctxRead.CreateBuffer();	// 얘는 항상 사용하거덩...
	m_ctxWrite.CreateBuffer();
	
	// reset handlers
	::ZeroMemory(FP_EVENTHANDLER, sizeof(FP_EVENTHANDLER));

	FP_RECVED_MSG_HANDLER = NULL;
	FP_INVOKER_SENDDATA = NULL;
	m_Type = eST_Stream;
}

CSockStream::~CSockStream()
{
	Close();
}

void CSockStream::Invalidate()
{
	m_pTask = NULL;
}

void CSockStream::Init()
{	
	SCOPED_LOCK_SINGLE(&m_CS);
	
	msg_size_limit_ = MSG_MASK_SIZE_ONLY;
	send_queue_depth_ = MAX_SENDQUE_DEPTH;
	msg_security_ = true;
	disconnect_error_code_ = 0;

	m_dwCurSentBytes = 0;
	m_dwCurRecvBytes = 0;
	send_buffer_size_ = 0;
	recv_buffer_size_ = 0;

	// IOContext::Reset() 에서 operation mode랑 Owner는 reset하지 않는다. 왜? 한번 정해지면 바뀌지 않으니깐...
	m_ctxWrite.Reset( FALSE );
	m_ctxRead.Reset( FALSE );	// ReadBuffer는 지웠다 만들었다 할 필요 없잖아...	
	m_ctxDisconnect.Reset( FALSE );

	m_pTask		= NULL;
	m_pSession	= NULL;
	m_latest_access_tick	= 0;
	
	m_dwdisconnectreason	= NULL;

	m_SessionState			= BEFORE_HANDSHAKE;

	m_cMsgEncodingOption	= BSNET_MSG_OPT_NOT_INITIALIZED;
	m_SeqGenerator.Reset();
	m_Crc.Reset();

	::ZeroMemory(&m_addrPeer, sizeof(m_addrPeer));

	if( m_pPendingSend )
	{	
		m_pPendingSend->ForceStopRead();
		engine_->DelMsg(m_pPendingSend);
		m_pPendingSend = NULL;
	}
	
	if( m_pPendingRecv )
	{
		m_pPendingRecv->ForceStopRead();
		engine_->DelMsg(m_pPendingRecv);
		m_pPendingRecv = NULL;
	}	
}

BYTE CSockStream::Disconnect(DWORD dwDisconnectReason)
{
	SCOPED_LOCK_SINGLE(&m_CS);

	BYTE btErrorCode = DISCONNECT_RESULT_UNKNOWN;

	if( m_pSession == NULL )
	{
		btErrorCode = DISCONNECT_RESULT_SESSION;
	}

	if (m_dwdisconnectreason)
		btErrorCode = DISCONNECT_RESULT_PENDING;
	else
		m_dwdisconnectreason = dwDisconnectReason;

	if( btErrorCode && dwDisconnectReason == DISCONNECT_REASON_STALE_SESSION )
	{
		CBaseSocket::Close();
	}
	else
	{
		BOOL errorcheck = CSockSystem::_TransmitFile( m_hSocket, NULL, NULL, NULL, (LPWSAOVERLAPPED)&m_ctxDisconnect.m_Overlapped, NULL, TF_DISCONNECT | TF_REUSE_SOCKET );
		if( errorcheck == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING )
		{
			CBaseSocket::Close();
			btErrorCode = DISCONNECT_RESULT_ERROR;
		}
		else
		{
			btErrorCode = DISCONNECT_RESULT_SUCCESS;	
		}
	}
				
	return btErrorCode;
}

long CSockStream::Close()
{
	if (this == NULL)
	{
		return NERR_INVALID_SESSION;
	}
	
	SCOPED_LOCK_SINGLE(&m_CS);

	if( engine_ != nullptr && !engine_->IsShutdown())
	{
		if (m_pPendingSend)
		{
			if (m_pSession && m_pSession->PostMsg(m_pPendingSend) == NERR_GOOD)
				m_pPendingSend = nullptr;
		}

		CMsg* pQueuedMsg;
		while (1)
		{
			pQueuedMsg = NULL;
			m_SendingQue.Deque(&pQueuedMsg, 0, TRUE);
			if (pQueuedMsg == NULL) // 큐에 쌓여있던 메시지 다 뽑혔구나...
				break;
				
			if (m_pSession == nullptr || m_pSession->PostMsg(pQueuedMsg) != NERR_GOOD)
				engine_->DelMsg(pQueuedMsg);
		}
	}
	
	////////////////////////////////////////////////////////////////////
	// Close Socket
	///////////////////////////////////////////////////////////////////	
	long BytesGot = 0;

	if( m_SessionType == SESSION_TYPE_ACTIVE )
	{
		if (m_hSocket != INVALID_SOCKET)
		{
			/*
			if (m_bGracefulShutdown)
			{
				int nRval = 0;
				
				CSockSystem::SetNonBlockingIO(m_hSocket, FALSE);
				::shutdown(m_hSocket, SD_SEND);
				
				nRval = 0;
				DWORD dwReturned;
				DWORD dwPendingData = 0;
				DWORD dwRecvNumBytes = 0;
				DWORD dwFlags = 0;

				while (nRval != SOCKET_ERROR)
				{
					if (WSAIoctl(m_hSocket, FIONREAD, NULL, 0, (void*)dwPendingData, sizeof(DWORD), &dwReturned, NULL, NULL) == SOCKET_ERROR)
						break;

					if (dwPendingData == 0)
						break;

					nRval = WSARecv(m_hSocket, (WSABUF*)&m_ctxWrite.m_wsaBuf[0], 1, &dwRecvNumBytes, &dwFlags, NULL, NULL);
					if (nRval != SOCKET_ERROR)
						BytesGot += dwRecvNumBytes;
				}
			}
			else
			*/
			{
				/////////////////////////////////////////////////////////////
				// Abortive close
				/////////////////////////////////////////////////////////////
				struct linger li = {1, 0};	
				::setsockopt(m_hSocket, SOL_SOCKET, SO_LINGER, (char *)&li, sizeof(li));		
			}

			::closesocket(m_hSocket);
			m_hSocket = INVALID_SOCKET;
		}

		// PASSIVE는 항상 세션을 유지하기에 초기화시켜주지않는다.
		m_SessionType = SESSION_TYPE_INVALID;
	}

	if (engine_ != nullptr)
	{
		if( engine_->IsShutdown() == FALSE)
		{
			Init();
		}
	}
	
	return BytesGot;
}

/***********************************************

[ I/O handler binding scheme ]

1. data i/o mode
	1) direct
		- send: SendMsgDirect(), OnSentDataDirect()
		- recv: RecvDataDirect(), OnReceivedDataDirect() (현재 direct mode전용 Recv계열 함수는 없어서 내부적으로는 buffered용을 호출한다.)

	2) buffered
		- send: SendMsgBuffered(), OnSentDataBuffered()
		- recv: RecvData(), OnReceivedData()

2. file i/o mode
	1) direct
		- send: 
			Win9x - SendFileBuffered(), OnSentFileBuffered()
			WinNT - SendFileDirect() --> use TransmitFile() internally, OnSentFileDirect()
		- recv: RecvFileDirect(), OnReceivedFileDirect()  (현재 direct mode전용 Recv계열 함수는 없어서 내부적으로는 buffered용을 호출한다.)

	2) buffered
		- send: SendFileBuffered(), OnSentFileBuffered()
		- recv: RecvFile(), OnReceivedFileBuffered();

	*** in case of buffered file transfer we don't care the exact file type what it is(memory_mapped, legacy or memory file)
		we just get IFile pointer from associated CSession object.

	단, os 가 win9x 계열인 경우는(client겠지) 일단 무조건 buffered 방식으로 돌게 하자.
	9x의 경우 event 방식 proactor를 사용해야 하므로 direct_mode를 돌리려면 모든 메시지에 대해서 
	overlapped.event 를 create 해야 할테고... 또한 현재 상태로는 direct방식을 쓴다는 자체가 
	undocumented된 것을 단지 내 판단만으로 진행하는 거라서... 아무래도 9x는 좀 후달린다아...
	그리구... 사실 9x 에선 transmitfile()도 사용 못하고 하니... 

************************************************/

bool CSockStream::PrepareSocket()
{
	_ASSERT(m_SessionType == SESSION_TYPE_ACTIVE || m_SessionType == SESSION_TYPE_PASSIVE);

	IO_MODE io_mode = engine_->GetIOMode(m_SessionType);
	
	if (engine_->GetProactorType() == PROACTOR_EVENT)	// Win9x 계열에선 무조건 buffered로 동작시키자.
		io_mode = IO_MODE_BUFFERED;

	_ASSERT(io_mode != IO_MODE_INVALID);
	
	/////////////////////////////////
	// 1. assign event handlers
	/////////////////////////////////

	// data i/o
	switch (io_mode)
	{
	case IO_MODE_DIRECT:
		FP_INVOKER_SENDDATA = &CSockStream::_SendMsgDirect;

		FP_EVENTHANDLER[IO_WRITE][IO_TARGET_DATA] = &CSockStream::_OnDataSentDirect;
		FP_EVENTHANDLER[IO_READ][IO_TARGET_DATA]  = &CSockStream::_OnDataRecved;
		break;

	case IO_MODE_BUFFERED:
		FP_INVOKER_SENDDATA	= &CSockStream::_SendMsgBuffered;

		FP_EVENTHANDLER[IO_WRITE][IO_TARGET_DATA] = &CSockStream::_OnDataSentBuffered;
		FP_EVENTHANDLER[IO_READ][IO_TARGET_DATA]  = &CSockStream::_OnDataRecved;
		break;

	default:
		_ASSERT(FALSE);
		break;
	}

	FP_EVENTHANDLER[IO_WRITE][IO_TARGET_FILE] = &CSockStream::_OnFileSent;
	FP_EVENTHANDLER[IO_READ][IO_TARGET_FILE]  = &CSockStream::_OnDataRecved;
	
	FP_RECVED_MSG_HANDLER = &CSockStream::_OnMsgReceivedBeforeHandshake;

	/////////////////////////////////
	// 2. set socket handle to iocontext
	/////////////////////////////////
	// 이유없이 클라이언트 끊는 버그 수정하면서 없어짐

	/////////////////////////////////
	// 3. create que for buffered sending mode
	/////////////////////////////////
	
	if (io_mode == IO_MODE_BUFFERED)
	{
		if (m_SendingQue.Open() == false)
			return false;
	}

	/////////////////////////////////
	// 4. create sending buffer
	/////////////////////////////////
	/*if (io_mode == IO_MODE_BUFFERED)
	{
		m_ctxWrite.CreateBuffer();
	}

  */
	return true;
}

bool CSockStream::CreateBuffer()
{
	int optlen = sizeof(send_buffer_size_);
	if (getsockopt(m_hSocket, SOL_SOCKET, SO_SNDBUF, (char*)&send_buffer_size_, &optlen) == SOCKET_ERROR)
		return false;

	if (getsockopt(m_hSocket, SOL_SOCKET, SO_RCVBUF, (char*)&recv_buffer_size_, &optlen) == SOCKET_ERROR)
		return false;

	m_ctxWrite.CreateBuffer(send_buffer_size_);
	m_ctxRead.CreateBuffer(recv_buffer_size_);
	return true;
}

BOOL CSockStream::CreateWSAEvent()
{
	if (engine_->GetProactorType() == PROACTOR_EVENT)
	{
		if (m_ctxRead.GetEventHandle() == NULL)
		{
			m_ctxRead.m_Overlapped.hEvent = ::WSACreateEvent();
			if (m_ctxRead.m_Overlapped.hEvent == WSA_INVALID_EVENT)
				return FALSE;
		}

		if (m_ctxWrite.GetEventHandle() == NULL)
		{
			m_ctxWrite.m_Overlapped.hEvent = ::WSACreateEvent();
			if (m_ctxWrite.m_Overlapped.hEvent == WSA_INVALID_EVENT)
				return FALSE;
		}
	}

	return TRUE;
}

DWORD CSockStream::GetSessionID()
{ 
	SCOPED_LOCK_SINGLE(&m_CS);

	if (m_pSession == NULL) 
		return NULL; 
	
	return m_pSession->GetID(); 
}

////////////////////////////////////////////////////////////////////////
//	For Client Socket
////////////////////////////////////////////////////////////////////////
BOOL CSockStream::CreateActiveSession(CNetEngine* engine, BSSOCKADDR_IN* addr_connect, BSSOCKADDR_IN* addr_bind,BOOL bForKeepAlive, DWORD connect_timeout)
{
	if (CBaseSocket::Create(engine, SESSION_TYPE_ACTIVE) == FALSE)
		return FALSE;
	
	if (addr_bind != NULL)
	{
		_ASSERT(addr_bind->sin_addr != 0);
		::bind(m_hSocket, (sockaddr*)addr_bind, sizeof(struct sockaddr));
	}
	
	if (connect_timeout > 0)
	{
		if (!CSockSystem::SetNonBlockingIO(m_hSocket))
			return FALSE;

		if (::connect(m_hSocket, (LPSOCKADDR)addr_connect, sizeof(SOCKADDR)) == 0)
			return FALSE;

		int error_code = WSAGetLastError();
		if (error_code != WSAEINPROGRESS && error_code != WSAEWOULDBLOCK)
			return FALSE;
			
		if (!CSockSystem::SetNonBlockingIO(m_hSocket, FALSE))
			return FALSE;

		fd_set write;
		FD_ZERO(&write);
		FD_SET(m_hSocket, &write);
		timeval timeout = {connect_timeout,0};
		if (::select(0, nullptr, &write, nullptr, &timeout) <= 0)
			return FALSE;

		if(FD_ISSET(m_hSocket, &write) == 0)
			return FALSE;
	}
	else
	{
		if (::connect(m_hSocket, (LPSOCKADDR)addr_connect, sizeof(SOCKADDR)) == SOCKET_ERROR)
			return FALSE;
	}

	if (bForKeepAlive)
	{
		if (CSockSystem::SetKeepAlive(m_hSocket, MAINTAINER_STALE_SESSION_TIMEOUT * 2, MAINTAINER_STALE_SESSION_INTERVAL) == false)
			return NERR_INVALID_SOCKET;
	}

	if (CreateWSAEvent() == FALSE)
		return FALSE;
	
	if (PrepareSocket() == false)
		return FALSE;

	if (!CreateBuffer())
		return FALSE;

	m_addrPeer = *addr_connect;

	TimeStamp();

	return TRUE;
}

BOOL CSockStream::CreatePassiveSession(CNetEngine* engine, SOCKET hListener)
{
	// 재사용시에는 소켓생성안함.
	if (CBaseSocket::Create(engine, SESSION_TYPE_PASSIVE) == FALSE)
	{
		return FALSE;
	}

	if (engine_->GetProactorType() == PROACTOR_IOCP)
	{
		_ASSERT(m_ctxRead.GetBuffer() != NULL);
		//m_ctxRead.m_access_tick = GetTickCount64();
				
		DWORD dwRecvNumBytes = 0;
		if (CSockSystem::_AcceptEx( hListener, 							
									GetHandle(), 							
									m_ctxRead.GetBuffer(), 							
									0, 							
									sizeof(SOCKADDR_IN) + 16, 							
									sizeof(SOCKADDR_IN) + 16, 							
									&dwRecvNumBytes, 							
									(LPOVERLAPPED)&m_ctxRead) == FALSE)
		{
			if ( ::WSAGetLastError() != ERROR_IO_PENDING)
			{
				CBaseSocket::Close();
				return FALSE;
			}
		}
	}
	else
	{
		if (CreateWSAEvent() == FALSE)
		{
			return FALSE;
		}
	}

	// setup handlers
	if (PrepareSocket() == false)
		return FALSE;
	
	return TRUE;	
}

long CSockStream::OnAccepted(SOCKET hListener)
{
	if (hListener == INVALID_SOCKET)
	{
		return NERR_UNKNOWN;
	}

	if (engine_->GetProactorType() == PROACTOR_EVENT)
	{
		m_hSocket = ::accept(hListener, NULL, NULL);
		if (m_hSocket == INVALID_SOCKET)
		{
			return NERR_UNKNOWN;
		}

		int	len = sizeof(m_addrPeer);
		::getpeername(m_hSocket, (SOCKADDR*)&m_addrPeer, &len);
	}
	else if (engine_->GetProactorType() == PROACTOR_IOCP)
	{
		int EstablishedSeconds = 0;
		if (IsLiveSession(EstablishedSeconds) == FALSE)
		{
			return NERR_FAILED_TO_ACCEPT;	// 음... accept하려고 보니깐 끊겼네... -_-;
		}
		
		// 타임아웃이 있을 경우 그 주어진 시간보다 오래동안 accept되지 않은 넘은 짤라버린다.
		// (EstablishedSeconds 이넘이 accept되기전까지 몇초동안 establish 되어있었는지 나타낸다)
		DWORD dwTimeOutForAccept = engine_->GetAcceptTimeoutSec();
		if (dwTimeOutForAccept != INFINITE)
		{
			if ((DWORD)EstablishedSeconds > dwTimeOutForAccept)
			{
				m_bGracefulShutdown = FALSE;
				
				return NERR_TIMEOUT;
			}
		}

		int LocalSockaddrLength		= 0;
		int RemoteSockaddrLength	= 0;
		SOCKADDR_IN *Local, *Remote;
		CSockSystem::_GetAcceptExSockAddrs(m_ctxRead.GetBuffer(), 
											0, 
											sizeof(SOCKADDR_IN) + 16, 
											sizeof(SOCKADDR_IN) + 16, 
											(sockaddr**)&Local, 
											&LocalSockaddrLength, 
											(sockaddr**)&Remote, 
											&RemoteSockaddrLength);
		
		::MoveMemory(&m_addrPeer, Remote, sizeof(sockaddr_in));
	}

	if (setsockopt(m_hSocket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (const char*)&hListener, sizeof(SOCKET)) == SOCKET_ERROR)
		return NERR_INVALID_SOCKET;

	if (!CreateBuffer())
		return NERR_INVALID_SOCKET;
	/*
	const MaintainerEntry& config = CNetEngine::GetEngineConfig().MC.target_entries[MAINTAIN_TARGET_KEEP_ALIVE_SESSION];
	if (CSockSystem::SetKeepAlive(m_hSocket, config.timeout * 2, config.interval) == false)
		return NERR_INVALID_SOCKET;
		*/
	
	//msg_size_limit_ = DEFAULT_MSGBUF_SIZE;
	send_queue_depth_ = engine_->GetSessionConfig(SESSION_TYPE_PASSIVE)->dwMaxSendQueDepth;
	TimeStamp();

	return NERR_GOOD;
}

BOOL CSockStream::RegisterHandleTo(CServiceObject* pServiceObj, HANDLE hHandle)
{
	if (pServiceObj == NULL)
		return FALSE;

	if (((CServiceObjectSock*)pServiceObj)->RegisterHandle(hHandle, (ULONG_PTR)this) == FALSE)
		return FALSE;

	if (PostRecv() != NERR_GOOD)
		return FALSE;

	return TRUE;
}

BOOL CSockStream::IsLiveSession(int& nEstablishedSeconds)
{
	int bytes = sizeof(nEstablishedSeconds);
	int err = ::getsockopt(m_hSocket, SOL_SOCKET, SO_CONNECT_TIME, (char*)&nEstablishedSeconds, &bytes);
	
	// nEstablishedSeconds: 0xffffffff --> not connected socket
	return (err == NO_ERROR);
}

////////////////////////////////////////////////////////
// protect session
////////////////////////////////////////////////////////
BOOL CSockStream::ProtectSession(EncodeCtx& ec)
{
	SCOPED_LOCK_SINGLE(&m_CS);
	
	if (m_SessionType == SESSION_TYPE_ACTIVE)
	{
		_ASSERT(FALSE);	// 세션 encoding handshaking은 언제나 서버측(passive session) 주도로 이루어진다.
		return TRUE;
	}

	_SetEncodeCtx(ec);

	if( engine_->GetEngineConfig().SecurityMode & BSNET_DIFFIEHELLMAN_DEEPDARK )
	{
		ec.options |= BSNET_MSG_OPT_SECURITY_SENDER;
	}

	// forge & send handshaking msg
	CMsg* pMsg = engine_->NewMsg();	// 모든 세션은 최초에 6Byte Header로 시작하거덩...
	pMsg->SetMsgID(ENGINEMSG_HANDSHAKING_REQ);

	*pMsg << ec.options;
	
	if( ec.options & BSNET_MSG_OPT_ENC )
	{
		pMsg->WriteBytes(&ec.bf_key[0], ENC_KEY_LEN);
	}

	if( ec.options & BSNET_MSG_OPT_SEQ_n_CHECKSUM )
	{
		*pMsg << ec.seq_seed << ec.crc_seed;
	}

	m_HandShake = BSNET_MSG_OPT_ENC | BSNET_MSG_OPT_SEQ_n_CHECKSUM;
	
	if( engine_->GetEngineConfig().SecurityMode & BSNET_DIFFIEHELLMAN_DEEPDARK )
	{		
		_DHSenderInfo SenderInfo;
		
		m_Sender.MakeSenderInfo( SenderInfo );
		
		*pMsg << SenderInfo;

		m_HandShake |= BSNET_MSG_OPT_SECURITY_SENDER;
	}
	
	long Result = (this->*FP_INVOKER_SENDDATA)(pMsg);	// 아직 handshake 전이라서... SendMsg() 호출하면 빠꾸 당하거덩...
	pMsg->ForceStopRead();
	engine_->DelMsg(pMsg);

	if( Result != NERR_GOOD )
	{
		return FALSE;
	}

	return TRUE;
}

void CSockStream::_SetEncodeCtx(EncodeCtx& ec)
{
	_ASSERT(ec.options != BSNET_MSG_OPT_NOT_INITIALIZED);

	m_cMsgEncodingOption = ec.options;

	if( ec.options & BSNET_MSG_DONT_ENCODE )
	{
		return;
	}
	else
	{
		if( ec.options & BSNET_MSG_OPT_ENC )
		{
			m_BF.Initialize(&ec.bf_key[0], ENC_KEY_LEN);
		}
		
		if( ec.options & BSNET_MSG_OPT_SEQ_n_CHECKSUM )
		{
			_ASSERT(ec.seq_seed != 0 && ec.crc_seed != 0);

			// seq
			CRndGenerator r(ec.seq_seed);
		
			DWORD dwAdder		= r.Generate();		
			DWORD dwMultiplier	= r.Generate();
			m_SeqGenerator.SetParams((BYTE)dwAdder, (BYTE)dwMultiplier);
			
			// checksum
			m_Crc.Initialize(ec.crc_seed);
		}
	}
}

////////////////////////////////////////////////////////
// coder / decoder
////////////////////////////////////////////////////////
BOOL CSockStream::ProtectMsg(CMsg*& pMsg)
{
	if (!msg_security_)
		return FALSE;

	BOOL bEncoded = FALSE;
	DWORD MsgSize = pMsg->GetMsgSize();
	BYTE* pSrcBuf = pMsg->GetBufferAt(0);
	
	// Set MSG Sequence Number
	if (CheckEncodingOption(BSNET_MSG_OPT_SEQ_n_CHECKSUM))
	{
		if (m_SessionType == SESSION_TYPE_ACTIVE)
		{
			pMsg->SetSequence(m_SeqGenerator.GenSeqNum());
			pMsg->SetCheckSum(0);
			pMsg->SetCheckSum(m_Crc.Generate(pSrcBuf, MsgSize));
		}
		else
		{
			// Server ---> Client 인 메시지는 굳이 seq, checksum을 넣어줄 필요 없겠다..
			// 생각해 봐라... 설마 서버가 메시지에 장난쳐서 날리겠냐?
			pMsg->SetSequence(0);
			pMsg->SetCheckSum(0);
		}
	}
	
	// Encrypt 할 메세지인가 (보호되는 session일지라도 명시적으로 메시지에 Enc하라고 되어있지 않으면 skip 한다)
	// Encryption은 Header Size와는 무관하게 가능하다.
	if (CheckEncodingOption(BSNET_MSG_OPT_ENC) && pMsg->IsEncMsg())
	{	
		DWORD RawDataSize = MSG_ENC_BEGIN; // 암호화 하지 않는 byte수 (size field 2byte + id field 2byte)
		DWORD SizeToEncrypt = pMsg->GetMsgSize() - MSG_ENC_BEGIN;	// 6byte 헤더의 경우 seq, checksum 도 함께 암호화 된다.
		DWORD SizeAfterEncrypt = m_BF.GetOutputLength(SizeToEncrypt);
		if ((RawDataSize + SizeAfterEncrypt) <= DEFAULT_MSGBUF_SIZE)		// 혹시 암호화 된 후의 메시지 size가 buffer size를 넘어가지는 않는거지?
		{
			// 알다시피 buffered mode의 broadcasting시에는 AddRef만 하면서 한놈을 여러 session에 대고 반복해서 날리는데
			// 각 세션별로 Send될 때 마다 seq_num 혹는 checksum 이 들어있는 header부분이 매번 바뀐다구... 
			// 그래서 encrypt 옵션이 켜있는 경우엔 복사본을 만들어 encrypt해서 날려야 한다.
			CMsg* pSpawned = engine_->NewMsg(TRUE);
			//pSpawned->SetMsgID(pMsg->GetMsgID());

			DWORD SizeEncrypted = m_BF.Encode(pMsg->GetBufferAt(MSG_ENC_BEGIN), pSpawned->GetBufferAt(MSG_ENC_BEGIN), SizeToEncrypt);
			if( SizeAfterEncrypt != SizeEncrypted )
			{
				_ASSERT( false );
			}
					
			pSpawned->SetDataSizeAtHeader(SizeToEncrypt - (MSG_HEADER_SIZE - MSG_ENC_BEGIN));
			pSpawned->SetWrPos(SizeAfterEncrypt + MSG_ENC_BEGIN);
			pSpawned->SetRdPos(SizeAfterEncrypt + MSG_ENC_BEGIN);
			pMsg = pSpawned;

			bEncoded = TRUE;
		}
		else
		{
			//PutLog(LOG_WARNING, _T("어떻게 된거냐? 도대체 얼마나 큰 메시지이길래 encryption fail이 나는거냐? 이런건 암호화 하지 말란말야! [msgid: 0x%x]"), pMsg->GetMsgID());
			pMsg->SetAsPlaneMsg();	// encrypt flag 제거하자.
		}
	}
	else if (CheckEncodingOption(BSNET_MSG_OPT_ENC) == 0 && pMsg->IsEncMsg())
	{
		//PutLog(LOG_WARNING, _T("Encryption을 지원하지 않는 세션인데 왜 자꾸만 암호화 하라구 그러는 거냐 대체! [msgid: 0x%x]"), pMsg->GetMsgID());
		_ASSERT(FALSE);
		pMsg->SetAsPlaneMsg();
	}

	return bEncoded;
}

DWORD CSockStream::NeutralizeMsg(DWORD& DataSize)
{
	DataSize = m_pPendingRecv->GetDataSize();
	
	if (!msg_security_)
		return NERR_GOOD;
	
	// IsEncMsg() 요거 체크안하면 우연히 encoding과정에서 ENGINEMSG_FILE_DATA 와 같은 MsgID가 생긴 것이랑 구분할 수가 없잖냐!
	if (m_pPendingRecv->IsEncMsg() == FALSE && m_pPendingRecv->GetMsgID() == ENGINEMSG_FILE_DATA)
		return NERR_GOOD;

	if (m_pPendingRecv->IsEncMsg())
	{
		// decrypt if secured session and msg
		if (CheckEncodingOption(BSNET_MSG_OPT_ENC))
		{
			DWORD dwSizeToDecode = m_BF.GetOutputLength(m_pPendingRecv->GetMsgSize() - MSG_ENC_BEGIN);
			//DWORD dwSizeToDecode = m_pPendingRecv->GetMsgSize() - MSG_ENC_BEGIN;

			if (dwSizeToDecode + MSG_ENC_BEGIN > DEFAULT_MSGBUF_SIZE)
				return NERR_INVALID_MSGSIZE;
			
			BYTE* pData = m_pPendingRecv->GetBufferAt(MSG_ENC_BEGIN);
			m_BF.Decode(pData, pData, dwSizeToDecode);
		}
		else
			return NERR_INVALID_MSG_HEADER;		// 뭐야? 이거 메시지 장난친 거 같은데?
	}

	// verify sequence number
	if (CheckEncodingOption(BSNET_MSG_OPT_SEQ_n_CHECKSUM))
	{
		// Client ---> Server 인 메시지만 혹시 조작했을지 모르니 검사하자!
		if (m_SessionType == SESSION_TYPE_PASSIVE)
		{
			if(m_pPendingRecv->GetSequence() != m_SeqGenerator.GenSeqNum())
			{
				return NERR_INVALID_SEQUENCE;
			}
						
			BYTE* pData	= m_pPendingRecv->GetBufferAt(0);
			BYTE checksum_recv = m_pPendingRecv->GetCheckSum();
			
			m_pPendingRecv->SetCheckSum(0);
			BYTE checksum = m_Crc.Generate(pData, m_pPendingRecv->GetMsgSize());

			if (checksum_recv != checksum)
				return NERR_INVALID_CHECKSUM;
		}
		else
		{
			if (m_pPendingRecv->GetCheckSum() != 0)
			{
				return NERR_INVALID_CHECKSUM;
			}
		}
	}
	
	return NERR_GOOD;
}

////////////////////////////////////////////////////////
// Invokers
////////////////////////////////////////////////////////

// <<<<<<<<<<<<<<< send series >>>>>>>>>>>>>>>>>>>
long CSockStream::SendMsg(CMsg* pMsg)
{
	if (this == NULL)
		return NERR_INVALID_SESSION;
	
	if (m_cMsgEncodingOption == BSNET_MSG_OPT_NOT_INITIALIZED || m_SessionState == BEFORE_HANDSHAKE)
		return NERR_NET_ACCESSED_BEFORE_HANDSHAKE;

	if (m_pSession == nullptr)
		return NERR_INVALID_SESSION;

	_ASSERT(FP_INVOKER_SENDDATA != NULL);
	return (this->*FP_INVOKER_SENDDATA)(pMsg);
}

long CSockStream::_ForgeMsgToSend(CMsg*& pMsg)
{
	if( m_dwdisconnectreason == DISCONNECT_REASON_OVERLAPPED )
		return NERR_UNKNOWN;
	
	if (m_hSocket == INVALID_SOCKET) 
		return NERR_UNKNOWN;
	
	BOOL bEncoded = FALSE;
	if (m_SessionState == AFTER_HANDSHAKE)
		bEncoded = ProtectMsg(pMsg);
	
	pMsg->SetOperationTarget(IO_TARGET_DATA);

	// Encode된 경우는 spawn된 넘이란 것인데 그 경우엔 AddRef()할 필요 없다.
	// 그리고 AddRef 된 후라면 이놈은 반드시 PostSend()가 호출되는 것을 보장해야 한다.
	// 그렇지 않으면 Msg Leak이 발생하게 된다.
	if (bEncoded == FALSE)
		pMsg->AddRef();

	return NERR_GOOD;
}

long CSockStream::_SendMsgDirect(CMsg* pMsg)
{
	SCOPED_LOCK_SINGLE(&m_CS);

	long rval = _ForgeMsgToSend(pMsg);
	if (rval != NERR_GOOD)
		return rval;
	
	return PostSend(pMsg);
}

long CSockStream::_SendMsgBuffered(CMsg* pMsg)
{
	SCOPED_LOCK_SINGLE(&m_CS);

	long rval = _ForgeMsgToSend(pMsg);
	if (rval != NERR_GOOD)
		return rval;
	
	if (send_queue_depth_ != MAX_SENDQUE_DEPTH && m_SendingQue.GetSize() > send_queue_depth_)
		return NERR_PEER_DOESNT_RESPONSE;

	m_SendingQue.Enque(pMsg);
	
	// 현재 네떡데이터를 날리던 도중이 아니면... 스타트 시키자.
	if (m_ctxWrite.IsEmpty() == TRUE )
	{
		rval = (this->*FP_EVENTHANDLER[IO_WRITE][IO_TARGET_DATA])(&m_ctxWrite, 0);
	}

	return rval;
}

long CSockStream::PostSend(IIOContext* pIOContext)
{
	switch (pIOContext->GetContextType())
	{
	case CONTEXT_TYPE_MSG:
		_ASSERT(((CMsg*)pIOContext)->GetMsgSize() >= MSG_HEADER_SIZE);
		((CMsg*)pIOContext)->PrepareOperation(); // <-- virtual 함수가 아니거덩... 얘들 virtual 안만들도록 했거덩?
		break;
	case CONTEXT_TYPE_CIRCULAR:
		if (m_ctxWrite.IsEmpty() == TRUE)
			return NERR_NODATA_TO_SEND;
		m_ctxWrite.PrepareOperation();
		break;
	default: 
		_ASSERT(FALSE);
		break;
	}
	
	long ret_val = NERR_GOOD;

	DWORD dwSent = 0;
	int rVal = ::WSASend(m_hSocket, (WSABUF*)&pIOContext->m_wsaBuf[0], pIOContext->m_nBufNum, &dwSent, 0, (LPWSAOVERLAPPED)&pIOContext->m_Overlapped, NULL);
	if (rVal == SOCKET_ERROR)
	{
		long nWSALastErr = ::WSAGetLastError();

		if (nWSALastErr != WSA_IO_PENDING)
		{
			CSockSystem::ShowWSAErrorString(nWSALastErr, _T("PostSend에서 에러났다!\n"), _T("PostSend()"));
		}

		if (nWSALastErr == WSA_IO_PENDING)
		{
			ret_val = NERR_GOOD;
		}
		else if (nWSALastErr == WSAEWOULDBLOCK || nWSALastErr == WSA_NOT_ENOUGH_MEMORY || nWSALastErr == WSAENOBUFS)
		{
			// from MSDN
			// [WSAEWOULDBLOCK] is
			// 1) Overlapped sockets: There are too many outstanding overlapped I/O requests. 
			// 2) Nonoverlapped sockets: The socket is marked as nonblocking and the send operation cannot be completed immediately.

			// 에러코드가 NERR_TOO_MANY_OUTSTANDING 라면 네떡 끊어 버리는 것이 옳다...
			// 왜냐면... peer에서 제대로 데이터 뽑지 못한다는 건데... 
			// client가 process는 살아있는 상태에서 맛이 간 경우거나... 아니면 이상한 수작해서 suspend 시킨 거겠지...
			// 그러므로 이 경우엔 client 끊어야 한다. 
			// 끊어버리지 않고 queueing한다던가 해서 어떻게든 유지시키려 한다면 결국 memory 부족으로 서버가 뻗게 된다...

			//PutLog(LOG_NOTIFY_FILE, _T("WSASend() - NERR_TOO_MANY_OUTSTANDING: %d"), nWSALastErr);
			ret_val = NERR_TOO_MANY_OUTSTANDING;
		}
		else 
		{
			ret_val = NERR_UNKNOWN;
		}
	}

	return ret_val;
}

long CSockStream::SendFile(IFile* pFile, DWORD dwOffset)
{	
	if (this == NULL)
		return NERR_INVALID_SESSION;
	
	SCOPED_LOCK_SINGLE(&m_CS);

	if (pFile == NULL || pFile->IsValidFile() == FALSE)
	{
		//PutLog(LOG_FATAL, _T("CSockStream::SendFile() - Valid File 이 아니다!"));
		_ASSERT(FALSE);
		return NERR_FAILED_TO_TRANSFILE;
	}
	
	if (dwOffset > 0)	// 이어받기 할거냐?
	{
		_ASSERT(pFile->GetFileSize() > dwOffset);
		pFile->Seek(GetSessionID(), 0);
	}

	return _OnFileSent(NULL, 0);	// send file !!!
}

////////////////////////////////////////////////////////
// handler
////////////////////////////////////////////////////////
long CSockStream::HandleEvent(DWORD dwTransferred, void* pCompleted)
{
	SCOPED_LOCK_SINGLE(&m_CS);
	
	if( m_dwdisconnectreason != NULL )
	{
		return NERR_GOOD;
	}

	if (dwTransferred == 0)
	{
		return NERR_SESSION_LOST;
	}

	if (GetHandle() == INVALID_SOCKET)
	{
		return NERR_INVALID_SOCKET;	
	}

	IIOContext* pContext = (IIOContext*)pCompleted;	
	_ASSERT(pContext != NULL);
	return (this->*FP_EVENTHANDLER[pContext->GetOperationMode()][pContext->GetOperationTarget()])(pContext, dwTransferred);
}

////////////////////////////////////////////////////////
// Completion handlers for each mode and target
////////////////////////////////////////////////////////

// 리턴값은 Deque성공을 뜻하는 것이 아니라 작업할 메시지가 있는지 없는지... 
// 그러니까 현재 Send를 해야할 m_pPendingSend 가 있는지를 여부를 나타낸다.
BOOL CSockStream::GetMsgToSend()
{	
	if (m_pPendingSend == NULL)
		return (m_SendingQue.Deque(&m_pPendingSend, 0) && m_pPendingSend != NULL);
	
	return TRUE;
}

long CSockStream::_OnDataSentDirect(IIOContext* pContext, DWORD dwTransferred)
{
	_ASSERT(pContext->GetContextType() == CONTEXT_TYPE_MSG);
	_ASSERT(m_ctxWrite.IsEmpty() == TRUE);

	// 내 생각이 맞다면 반드시 posting한 byte만큼 completion이 일어나야 한다!
	// 그렇지 않으면 overlapped i/o 의 존재 이유가 없다!
#ifdef _DEBUG
	/*
	DWORD dwPredictedSize = 0;

	CMsg* pSentMsg = (CMsg*)pContext;

	if (CheckEncodingOption(BSNET_MSG_OPT_ENC) && pSentMsg->IsEncMsg())
		dwPredictedSize = m_BF.GetOutputLength(pSentMsg->GetMsgSize() - MSG_ENC_BEGIN) + MSG_ENC_BEGIN;
	else
		dwPredictedSize = pSentMsg->GetMsgSize(); 

	_ASSERT(dwPredictedSize == dwTransferred);
	*/
#endif

	engine_->DelMsg((CMsg*)pContext);
	
	return NERR_GOOD;
}

long CSockStream::_OnDataSentBuffered(IIOContext* pContext, DWORD dwTransferred)
{	
	if (dwTransferred != 0)
		m_ctxWrite.IOCompleted(dwTransferred);

	long nDataTobeSent  = 0;
	long nActualWritten	= 0;
	BYTE* pCurBuf		= NULL;

	while (GetMsgToSend())
	{
		if (m_pPendingSend->IsEncMsg())
		{
			nDataTobeSent = m_BF.GetOutputLength(m_pPendingSend->GetMsgSize() - MSG_ENC_BEGIN) + MSG_ENC_BEGIN - m_dwCurSentBytes;
		}
		else if (m_pPendingSend)
		{
			nDataTobeSent = m_pPendingSend->GetMsgSize() - m_dwCurSentBytes;
		}
		
		if (nDataTobeSent > 0)
		{
			pCurBuf = m_pPendingSend->GetBufferAt(m_dwCurSentBytes);
			nActualWritten = m_ctxWrite.WriteBytes(pCurBuf, nDataTobeSent);

			m_dwCurSentBytes += nActualWritten;

			if (nActualWritten == nDataTobeSent)
			{
				if ( m_pPendingSend && m_pPendingSend->GetOperationTarget() == IO_TARGET_FILE)
				{
					// DelMsg 는 OnFileSent 에서 해준다.
					_OnFileSent(m_pPendingSend, m_pPendingSend->GetMsgSize());
				}
			}

			break;
		}

		engine_->DelMsg(m_pPendingSend);
		m_pPendingSend = nullptr;
		m_dwCurSentBytes = 0;
	}

	if (!m_ctxWrite.IsEmpty())
		return PostSend(&m_ctxWrite);

	return NERR_GOOD;
}

long CSockStream::_OnFileSent(IIOContext* pContext, DWORD dwTransferred)
{
	if (pContext != NULL)
	{
		_ASSERT(pContext->GetContextType() == CONTEXT_TYPE_MSG);
		_ASSERT(((CMsg*)pContext)->GetRefCnt() == 1);
		engine_->DelMsg((CMsg*)pContext);
	}

	CSession* pSession = GetSession();
	if (pSession != NULL)
	{
		IFile* pFile = pSession->GetWorkingFile();
		if (pFile == NULL)
		{
			//PutLog(LOG_FATAL, _T("CSockStream::_OnFileSent() - Session의 WorkingFile이 NULL이다"));
			_ASSERT(FALSE);
			return NERR_INVALID_TARGET_FILE;
		}

		BOOL bSendCompleted = pSession->OnFileDataSent(dwTransferred > 0 ? dwTransferred - MSG_HEADER_SIZE : 0);
		if (bSendCompleted == TRUE)
		{
			pSession->FileTransferCompleted(IO_READ, TRUE);
			return NERR_GOOD;
		}
		else 
		{
			CMsg* pFileMsg = engine_->NewMsg(FALSE);
			pFileMsg->SetMsgID(ENGINEMSG_FILE_DATA);
			
			DWORD dwActualRead = pFile->Read(GetSessionID(), pFileMsg->GetWrBuffer(), (DEFAULT_MSGBUF_SIZE - MSG_HEADER_SIZE));

			pFileMsg->SetDataSizeAtHeader(dwActualRead);
			pFileMsg->SetOperationTarget(IO_TARGET_FILE);

			pFileMsg->SetRdPos(dwActualRead, TRUE);
			pFileMsg->SetWrPos(dwActualRead, TRUE);
			
			if (engine_->GetIOMode(m_SessionType) == IO_MODE_BUFFERED)
			{
				m_SendingQue.Enque(pFileMsg);
	
				if (m_ctxWrite.IsEmpty())
					return _OnDataSentBuffered(&m_ctxWrite, 0);
				
				return NERR_GOOD;
			}
			else
			{
				return PostSend(pFileMsg);
			}
		}
	}
	else
	{
		//PutLog(LOG_FATAL, _T("CSockStream::_OnFileSent() - Bind된 세션이 NULL이다!"));
//		_ASSERT(FALSE);
		return NERR_INVALID_SESSION;
	}
}

// <<<<<<<<<<<<<<< recv series >>>>>>>>>>>>>>>>>>>
long CSockStream::RecvFile()
{
	if (this == NULL)
		return NERR_INVALID_SESSION;
	
	return NERR_GOOD;
}

long CSockStream::PostRecv()
{
	m_ctxRead.TrimBuffer();
	
	_ASSERT(m_ctxRead.GetRemainingBytes() < MSG_HEADER_SIZE);
	
	m_ctxRead.PrepareOperation();

	_ASSERT(m_ctxRead.m_wsaBuf[0].len > 0);
	
	DWORD dwFlags	= 0;
	DWORD dwRecved	= 0;

	int rVal = ::WSARecv(m_hSocket, (WSABUF*)&m_ctxRead.m_wsaBuf[0], m_ctxRead.m_nBufNum, &dwRecved, &dwFlags, (LPWSAOVERLAPPED)&m_ctxRead.m_Overlapped, NULL);
	if( rVal == SOCKET_ERROR ) 
	{		
		long nLastErr = ::WSAGetLastError();
		if (nLastErr == WSA_IO_PENDING)
		{
			return NERR_GOOD;
		}
		else if (nLastErr == WSAEWOULDBLOCK || nLastErr == WSA_NOT_ENOUGH_MEMORY || nLastErr == WSAENOBUFS)	// There are too many outstanding overlapped I/O requests
		{
			return NERR_TOO_MANY_OUTSTANDING;
		}
		else
		{
			return NERR_UNKNOWN;
		}
	}
	
	return NERR_GOOD;
}

////////////////////////////////////////// 
// completion handler
////////////////////////////////////////// 
long CSockStream::_OnDataRecved(IIOContext* pContext, DWORD dwTransferred)
{
	TimeStamp();

	if (m_pTask == NULL)
	{
		return NERR_NO_MSGTARGET;
	}

	if (m_pSession == nullptr)
	{
		return NERR_SESSION_LOST;
	}
	
	if (dwTransferred != 0)
		m_ctxRead.IOCompleted(dwTransferred);

	long error_code = NERR_UNKNOWN;
	while (1)
	{
		if (m_pPendingRecv != NULL)
		{
			DWORD RemainBytesToCompleteMsg = 0;	// 앞으로 몇바이트 더 읽어야 이 메시지 다 읽은거지?

			if (CheckEncodingOption(BSNET_MSG_OPT_ENC) && m_pPendingRecv->IsEncMsg())
				RemainBytesToCompleteMsg = m_BF.GetOutputLength(m_pPendingRecv->GetMsgSize() - MSG_ENC_BEGIN) + MSG_ENC_BEGIN - m_dwCurRecvBytes;
			else
				RemainBytesToCompleteMsg = m_pPendingRecv->GetMsgSize() - m_dwCurRecvBytes; 

			if (RemainBytesToCompleteMsg > m_pPendingRecv->GetAvailableBufSize())	// Msg size exceeded
			{
				error_code = NERR_INVALID_MSGSIZE;
				goto __FAILED_RECV;
			}

			BYTE* pCurBuf = m_pPendingRecv->GetBufferAt(m_dwCurRecvBytes);				
			DWORD ActualRead = m_ctxRead.ReadBytes(pCurBuf, RemainBytesToCompleteMsg);
			
			m_dwCurRecvBytes += ActualRead;
			
			if (ActualRead == RemainBytesToCompleteMsg)	// 메시지가 완성되았구나?
			{
				DWORD DataSize = 0;
				error_code = NeutralizeMsg(DataSize);

				if (error_code != NERR_GOOD)
				{
					goto __FAILED_RECV;
				}

				if (m_pPendingRecv->SetRdPos(0, TRUE) == FALSE)
				{
					error_code = NERR_INVALID_MSGSIZE;
					goto __FAILED_RECV;
				}

				if (m_pPendingRecv->SetWrPos(DataSize, TRUE) == FALSE)
				{
					error_code = NERR_INVALID_MSGSIZE;
					goto __FAILED_RECV;
				}

				m_pPendingRecv->SetSessionID(GetSessionID());

				error_code = _MsgReceived(m_pPendingRecv);
				if( error_code != NERR_GOOD )
				{
					goto __FAILED_RECV;
				}

				m_pPendingRecv	 = NULL;
				m_dwCurRecvBytes = 0;
			}
			else	// 그렇지 않다면... 아직 네떡에서 데이터를 다 못받은거니까... 담기회로...
			{
				break;
			}
		}
		
		if (m_pPendingRecv == NULL)
		{
			if (m_ctxRead.GetRemainingBytes() >= MSG_HEADER_SIZE)	// 최소한 HEADER_SIZE 와 같거나 더 많은 데이터 있을때만 읽는다. 
			{
				m_pPendingRecv = engine_->NewMsg(FALSE);
				m_dwCurRecvBytes = m_ctxRead.ReadBytes(m_pPendingRecv->GetBufferAt(0), MSG_HEADER_SIZE);	// header size 만큼만 읽고, 이후 처리는 윗쪽 if 블럭에서...
				
				_ASSERT(m_dwCurRecvBytes == MSG_HEADER_SIZE);

				DWORD wMsgSize = m_pPendingRecv->GetMsgSize();
				if (wMsgSize > DEFAULT_MSGBUF_SIZE)
				{
					if (wMsgSize > msg_size_limit_)
					{						
					#if SERVER_BUILD
						JSONValue log;
						log["LOG_EVENT"] = "MSG_ERROR";
						log["IP"] = inet_ntoa(GetPeerIp().sin_addr);
						log["SOCKET"] = static_cast<DWORD>(GetHandle());
						log["MSG_SIZE"] = wMsgSize;
						log["MSG_ID"] = FormatString("0x%x", m_pPendingRecv->GetMsgID());
						log["SESSION_ID"] = m_pSession->GetID();
						if (m_pSession->serial_number())
						{
							std::stringstream ss;
							ss << m_pSession->serial_number();
							log["SERIAL_NUMBER"] = ss.str().c_str();
						}
						PutLog(LOG_FATAL_FILE, log);
					#endif
						error_code = NERR_INVALID_MSGSIZE;
						goto __FAILED_RECV;
					}
					else
					{
						m_pPendingRecv->Resize(wMsgSize);
					}
				}
			}
			else			
			{
				break;
			}
		}
	}

	return PostRecv();

/////////////////////////////////  i like 'goto' :)
__FAILED_RECV:
	m_pPendingRecv->ForceStopRead();
	engine_->DelMsg(m_pPendingRecv);
	
	m_pPendingRecv	 = NULL;
	m_dwCurRecvBytes = 0;
	
	return error_code;
}

long CSockStream::_OnMsgReceivedBeforeHandshake(CMsg* pRecvMsg)
{
	long error_code = NERR_GOOD;
		
	bool session_hand_shake = false;
	
	if( m_SessionType == SESSION_TYPE_ACTIVE )
	{
		if (pRecvMsg->GetMsgID() != ENGINEMSG_HANDSHAKING_REQ)
		{
			//PutLog(LOG_WARNING_FILE, _T("_OnMsgReceivedBeforeHandshake() - Handshake 이전에 이상한 메시지가 날아왔다[MsgID: 0x%x]"), pRecvMsg->GetMsgID());
			error_code = NERR_NET_ACCESSED_BEFORE_HANDSHAKE;
			goto __ERR;
		}
	
		EncodeCtx ec;
		*pRecvMsg >> ec.options;
		
		if( ec.options & BSNET_MSG_OPT_ENC )
		{
			pRecvMsg->ReadBytes(&ec.bf_key[0], ENC_KEY_LEN);
		}

		if( ec.options & BSNET_MSG_OPT_SEQ_n_CHECKSUM )
		{
			*pRecvMsg >> ec.seq_seed >> ec.crc_seed;
		}

		if( ec.options & BSNET_MSG_OPT_SECURITY_RECIPIENT )
		{
			_DHSignature ServerSignature;
			
			*pRecvMsg >> ServerSignature;
			
			if( !m_Recipient.VerifySenderSignature( ServerSignature ) )
			{
				//PutLog(LOG_WARNING_FILE, _T("ACTIVE_SESSION::_OnMsgReceivedBeforeHandshake() - Handshake ServerSignature Error[MsgID: 0x%x]"), pRecvMsg->GetMsgID());
				error_code = NERR_NET_ACCESSED_BEFORE_HANDSHAKE;
				goto __ERR;
			}
			
			// Client Key 등록
			_DHBlowfishKey KeyRecipient;
			m_Recipient.GetEncryptionKey( KeyRecipient );
			
			m_BF.Initialize( &KeyRecipient.Key[0], ENC_KEY_LEN );
			
			// 완료 통보
			CMsg* pAck = engine_->NewMsg(FALSE);
			pAck->SetMsgID(ENGINEMSG_HANDSHAKING_ACK);
			
			pAck->SetSequence( m_SeqGenerator.GenSeqNum() );
			pAck->SetCheckSum( m_Crc.Generate( pAck->GetBufferAt(0), pAck->GetMsgSize() ) );
			
			error_code = (this->*FP_INVOKER_SENDDATA)(pAck);	// 아직 handshake 전이라서... SendMsg() 호출하면 빠꾸 당하거덩...
			pAck->ForceStopRead();
			engine_->DelMsg(pAck);		

			session_hand_shake = true;
		}
		else
		{
			_SetEncodeCtx(ec);
			
			if( ec.options & BSNET_MSG_OPT_SECURITY_SENDER )
			{			
				_DHSenderInfo SenderInfo;

				*pRecvMsg >> SenderInfo;
				m_Recipient.SetSenderInfo( SenderInfo );

				_DHRecipientInfo RecipientInfo;
				m_Recipient.MakeRecipientInfo( RecipientInfo );

				// Client -> Sercer ( Certificate:_DHSenderInfo )
				CMsg* pReq = engine_->NewMsg( FALSE );
				pReq->SetMsgID( ENGINEMSG_HANDSHAKING_REQ );

				*pReq << RecipientInfo;
				
				pReq->SetSequence( m_SeqGenerator.GenSeqNum() );
				pReq->SetCheckSum( m_Crc.Generate( pReq->GetBufferAt(0), pReq->GetMsgSize() ) );
				
				error_code = (this->*FP_INVOKER_SENDDATA)(pReq);	
				pReq->ForceStopRead();
				engine_->DelMsg(pReq);
			}
			else
			{				
				CMsg* pAck = engine_->NewMsg(FALSE);
				pAck->SetMsgID(ENGINEMSG_HANDSHAKING_ACK);
				
				if( ec.options & BSNET_MSG_OPT_SEQ_n_CHECKSUM )
				{
					pAck->SetSequence( m_SeqGenerator.GenSeqNum() );
					pAck->SetCheckSum( m_Crc.Generate( pAck->GetBufferAt(0), pAck->GetMsgSize() ) );
				}
				else
				{
					pAck->SetSequence(0);
					pAck->SetCheckSum(0);
				}

				error_code = (this->*FP_INVOKER_SENDDATA)(pAck);	// 아직 handshake 전이라서... SendMsg() 호출하면 빠꾸 당하거덩...
				pAck->ForceStopRead();
				engine_->DelMsg(pAck);

				session_hand_shake = true;
			}
		}
	}
	else
	{
		if( m_HandShake == BSNET_MSG_OPT_NOT_INITIALIZED )
		{
			//PutLog(LOG_WARNING, _T("_OnMsgReceivedBeforeHandshake() - Initialize [MsgID: 0x%x]"), pRecvMsg->GetMsgID());
			error_code = NERR_NET_ACCESSED_BEFORE_HANDSHAKE;
			goto __ERR;
		}
		
		if( pRecvMsg->GetMsgID() == ENGINEMSG_HANDSHAKING_REQ )
		{
			if( !( ( m_HandShake & BSNET_MSG_OPT_ENC ) && ( m_HandShake & BSNET_MSG_OPT_SEQ_n_CHECKSUM ) && ( m_HandShake & BSNET_MSG_OPT_SECURITY_SENDER ) ) )
			{
				//PutLog(LOG_WARNING_FILE, _T("_OnMsgReceivedBeforeHandshake() - Handshake Failed:%d [MsgID: 0x%x]"), m_HandShake, pRecvMsg->GetMsgID());
				error_code = NERR_NET_ACCESSED_BEFORE_HANDSHAKE;
				goto __ERR;
			}

			if( pRecvMsg->GetDataSize() != sizeof( _DHRecipientInfo ) )
			{
				//PutLog(LOG_WARNING_FILE, _T("_OnMsgReceivedBeforeHandshake() - Handshake Recipient Size:%d [MsgID: 0x%x]"), pRecvMsg->GetDataSize(), pRecvMsg->GetMsgID());
				error_code = NERR_NET_ACCESSED_BEFORE_HANDSHAKE;
				goto __ERR;
			}

			_DHRecipientInfo RecipientInfo;

			*pRecvMsg >> RecipientInfo;
			
			if( !m_Sender.SetRecipientInfo( RecipientInfo ) )
			{
				//PutLog(LOG_WARNING_FILE, _T("_OnMsgReceivedBeforeHandshake() - Handshake RecipientInfo Error [MsgID: 0x%x]"), pRecvMsg->GetMsgID());
				error_code = NERR_NET_ACCESSED_BEFORE_HANDSHAKE;
				goto __ERR;
			}

			// Server -> Client ( Certificate:_DHRecipientInfo )
			CMsg* pReq = engine_->NewMsg();
			pReq->SetMsgID( ENGINEMSG_HANDSHAKING_REQ );
		
			*pReq << (BYTE)BSNET_MSG_OPT_SECURITY_RECIPIENT;
			
			_DHSignature ServerSignature;
			m_Sender.MakeSenderSignature( ServerSignature );
			
			*pReq << ServerSignature;
			
			error_code = (this->*FP_INVOKER_SENDDATA)(pReq);
			pReq->ForceStopRead();
			engine_->DelMsg(pReq);

			// Server Key 등록
			_DHBlowfishKey KeySender;
			m_Sender.GetEncryptionKey( KeySender );
			
			m_BF.Initialize( &KeySender.Key[0], ENC_KEY_LEN );
			
			m_HandShake |= BSNET_MSG_OPT_SECURITY_RECIPIENT;
		}
		else if( pRecvMsg->GetMsgID() == ENGINEMSG_HANDSHAKING_ACK )
		{
			if( SecurityModeCheck() )
			{
				session_hand_shake = true;
			}						
			else
			{
				//PutLog(LOG_WARNING, _T("SecurityModeCheck - Mode:%d, Handshake:%d"), CNetEngine::GetEngine()->GetEngineConfig().SecurityMode, m_HandShake );
				error_code = NERR_NET_ACCESSED_BEFORE_HANDSHAKE;
				goto __ERR;
			}
		}
		else
		{
			//PutLog(LOG_WARNING, _T("_OnMsgReceivedBeforeHandshake() - Handshake 이전에 이상한 메시지가 날아왔다[MsgID: 0x%x]"), pRecvMsg->GetMsgID());
			error_code = NERR_NET_ACCESSED_BEFORE_HANDSHAKE;
			goto __ERR;
		}
	}
	
	if( error_code == NERR_GOOD )
	{
		pRecvMsg->ForceStopRead();
		engine_->DelMsg( pRecvMsg );
		
		if( session_hand_shake )
		{
			_SessionHandshaked();
		}
	}
		
	return error_code;

__ERR:
	m_HandShake = BSNET_MSG_OPT_NOT_INITIALIZED;

	return error_code;
}

BOOL CSockStream::SecurityModeCheck( void )
{
	if( engine_->GetEngineConfig().SecurityMode & BSNET_DEFAULT && !( m_HandShake & BSNET_MSG_OPT_SECURITY_RECIPIENT ) )
	{
		if( m_HandShake & BSNET_MSG_OPT_ENC && m_HandShake & BSNET_MSG_OPT_SEQ_n_CHECKSUM )
		{
			return TRUE;
		}
	}	

	if( engine_->GetEngineConfig().SecurityMode & BSNET_DIFFIEHELLMAN_DEEPDARK )
	{
		if( m_HandShake & BSNET_MSG_OPT_ENC && 
			m_HandShake & BSNET_MSG_OPT_SEQ_n_CHECKSUM && 
			m_HandShake & BSNET_MSG_OPT_SECURITY_SENDER && 
			m_HandShake & BSNET_MSG_OPT_SECURITY_RECIPIENT )
		{
			return TRUE;
		}
	}

	return FALSE;
}

long CSockStream::_WriteFileData(CMsg* pFileMsg)
{
	long error_code = NERR_GOOD;
	DWORD dwActualWritten = 0;
	BOOL bCompleted = FALSE;
	IFile* pFile = NULL;
	
	CSession* pSession = GetSession();
	if (pSession == NULL)
	{
		error_code = NERR_INVALID_SESSION;
		goto _FAILED_WRITE_FILE;
	}

	pFile = pSession->GetWorkingFile();
	if (pFile == NULL || pFile->IsValidFile() == NULL)
	{
		error_code = NERR_INVALID_TARGET_FILE;
		goto _FAILED_WRITE_FILE;
	}
	
	dwActualWritten = pFile->Write(GetSessionID(), pFileMsg->GetRdBuffer(), pFileMsg->GetDataSize());
	bCompleted = pSession->OnFileDataReceived(dwActualWritten);
	if (bCompleted == TRUE)
		pSession->FileTransferCompleted(IO_WRITE, TRUE);

_FAILED_WRITE_FILE:
	
	// 어떤 경우에도 Msg Leak은 피해야지...
	pFileMsg->ForceStopRead();
	engine_->DelMsg(pFileMsg);

	return error_code;
}

long CSockStream::_OnMsgReceivedAfterHandshake(CMsg* pRecvMsg)
{	
	if (pRecvMsg->GetMsgID() == ENGINEMSG_FILE_DATA)
		return _WriteFileData(pRecvMsg);
	else
	{		
		if (m_pSession && !m_pSession->ReceiveMsg(pRecvMsg))
		{
			engine_->DelMsg(pRecvMsg);
			return NERR_GOOD;
		}

		if (m_pTask != NULL)
			m_pTask->EnqueMsg(pRecvMsg);
		else
			return NERR_TARGET_TASK_LOST;
	}

	return NERR_GOOD;
}

void CSockStream::_SessionHandshaked( void )
{	
	m_SessionState		  = AFTER_HANDSHAKE;
	FP_RECVED_MSG_HANDLER = &CSockStream::_OnMsgReceivedAfterHandshake;

	if (m_pSession != nullptr)
		m_pSession->SessionHandshakingCompleted();
}

long CSockStream::_MsgReceived(CMsg* pRecvedMsg)
{	
	_ASSERT(FP_RECVED_MSG_HANDLER != NULL);
	return (this->*FP_RECVED_MSG_HANDLER)(pRecvedMsg);
}

void CSockStream::SetDisconnectReason( DWORD dwDisconnectReason )
{
	SCOPED_LOCK_SINGLE(&m_CS);

	m_dwdisconnectreason = dwDisconnectReason;
}

///////////////////////////////////////////////////////////////
// For Listening Socket
///////////////////////////////////////////////////////////////
long CSockListener::s_nCurPostedAccept = 0;

CSockListener::CSockListener() : CBaseSocket(), m_AcceptCS( TRUE )
{
	m_nBackLog = SOMAXCONN;
	m_Type = eST_Listener;
}

CSockListener::~CSockListener()
{
	Close();
}

BOOL CSockListener::Create(CNetEngine* engine, BSSOCKADDR_IN* addr_bind, int backlog /* DEFAULT_BACK_LOG */)
{	
	if (CBaseSocket::Create(engine, SESSION_TYPE_PASSIVE) == FALSE)
		return FALSE;

	int	reuse = 1;
	::setsockopt(m_hSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse));

	int nRet = ::bind(m_hSocket, (sockaddr*)addr_bind, sizeof(BSSOCKADDR_IN));
    if (SOCKET_ERROR == nRet) 
		return FALSE;

	m_nBackLog = backlog;
	s_nCurPostedAccept = 0;
	
	return TRUE;
}

long CSockListener::Close()
{
	SCOPED_LOCK_SINGLE(&m_CS);

	if (m_hSocket != INVALID_SOCKET)
	{
		::closesocket(m_hSocket);
		m_hSocket = INVALID_SOCKET;
	}

	return 0;
}

/////////////////////////////////////////////////////////////////////
//	Use IOCP	- 리슨하면서 acceptex호출 미리 소켓을 생성하고 기다린다...
//	Use Event	- 리슨만 하고 이벤트를 기다린다....
/////////////////////////////////////////////////////////////////////
BOOL CSockListener::Listen(CServiceObject* pServiceObj)
{
	_ASSERT(m_hSocket != INVALID_SOCKET && pServiceObj != NULL);

	int back_log = (engine_->GetEngineConfig().dwConfigFlags & ENGINE_CONFIG_MASSIVE_CLIENT) ? engine_->GetConcurrentAcceptNum() : m_nBackLog;
	int nRet = ::listen(m_hSocket, back_log);
	
	if (SOCKET_ERROR == nRet) 
	{
		WSAERROR(TEXT("Failed to listen"), TEXT("CSockListener::Listen"));
		return FALSE;
	}

	if (RegisterHandleTo(pServiceObj, (HANDLE)GetHandle()) == FALSE)
	{
		return FALSE;
	}
	
	SCOPED_LOCK_SINGLE(&m_CS);
	
	if(engine_->GetProactorType() == PROACTOR_IOCP)
	{
		int max_concurrent_accept = engine_->GetConcurrentAcceptNum();

#ifdef SINGLE_POST_ACCEPT
		max_concurrent_accept = 1;
#else
		for (int i = 0; i < max_concurrent_accept; i++)
#endif
		{
			if (engine_->PostNewAccept() == TRUE)
			{
				++s_nCurPostedAccept;
			}
			else
			{
				return FALSE;
			}
		}
	}
	
	return TRUE;
}

long CSockListener::Accept(CSockStream* pSock)
{	
	if (pSock == NULL)
		return NERR_FAILED_TO_ACCEPT;

	if (pSock->OnAccepted(GetHandle( )))
		return NERR_FAILED_TO_ACCEPT;

	if (engine_->CreateNewSession(pSock, engine_->GetDefaultTaskID(SESSION_TYPE_PASSIVE)) == NULL)
		return NERR_FAILED_TO_ACCEPT;
	
	return NERR_GOOD;
}

BOOL CSockListener::DisconnectCheck( CSockStream* pSock, DWORD dwReason )
{	
	SCOPED_LOCK_SINGLE(&m_CS);

	if( pSock->GetType() != CBaseSocket::eST_Listener )
		return engine_->CloseSession(pSock, dwReason);

	return TRUE;
}
//////////////////////////////////////////////////////////////////////////
// IOCP Accept	- pCompleted : CIOContext*
// Event Accept - pCompleted : NULL
//
// IOCP인경우는 미리 생성한 socket context의 정보가 넘어오지만
// Event인경우는 미리 socket context를 생성하지 않기 때문에
// 이때 소켓생성및 accept가 이루어져야 한다...
/////////////////////////////////////////////////////////////////////////
long CSockListener::HandleEvent(DWORD dwTransferred, void* pCompleted)
{
	SCOPED_LOCK_SINGLE(&m_CS);
	
	if( m_hSocket == INVALID_SOCKET )
	{
		return NERR_GOOD;
	}

	long rval = NERR_UNKNOWN;

	CSockStream* pNewSock = NULL;

	if (engine_->GetProactorType() == PROACTOR_IOCP)
	{
		CIOContext* pContext = (CIOContext*)pCompleted;
 
 		_ASSERT(pContext != NULL);
		_ASSERT(pContext->GetOwner() != NULL);		

		pNewSock = (CSockStream*)pContext->GetOwner();

		PostNewAccept();

		rval = Accept(pNewSock);

		if( rval == NERR_FAILED_TO_ACCEPT )
		{
			if( pNewSock->Disconnect( DISCONNECT_REASON_OVERLAPPED ) == FALSE )
				engine_->CloseSession( pNewSock->GetSessionID(), pNewSock->GetDisconnectReason() );
		}
	}
	else
	{	
		_ASSERT(engine_->GetProactorType() == PROACTOR_EVENT);

		//2006. 7. 13 by jshstorm
		//passive socket을 얻어와야죠!
		pNewSock = engine_->AllocPassiveNewSock();
		if (pNewSock->CreatePassiveSession(engine_, m_hSocket) == FALSE)
		{			
			//요것도 passive socket pool에 도로 넣어줍니다.
			engine_->FreePassiveSocket(pNewSock);
			return NERR_FAILED_TO_ACCEPT;
		}
		
		rval = Accept(pNewSock);
	}

	return rval;
}

/////////////////////////////////////////////////////////////////////
//	Use IOCP  - nNumToPostAccept: m_nConcurrentAcceptNum - m_nCurPostedAccept
//	Use Event - nNumToPostAccept: 1
/////////////////////////////////////////////////////////////////////
void CSockListener::PostNewAccept()
{	
	SCOPED_LOCK_SINGLE(&m_AcceptCS);
	
	int max_concurrent_accept = engine_->GetConcurrentAcceptNum();
	--s_nCurPostedAccept;
		
#ifndef SINGLE_POST_ACCEPT
	long nNumToPostAccept = 0;
	if( max_concurrent_accept > s_nCurPostedAccept )
	{
		if (engine_->GetProactorType() == PROACTOR_IOCP)
		{
			nNumToPostAccept = (max_concurrent_accept - s_nCurPostedAccept);
		}
		
		for (int i = 0; i < nNumToPostAccept; )
		{
			if (engine_->PostNewAccept() == TRUE)
			{
				++s_nCurPostedAccept;
			}		
			++i;
		}
	}
#endif
}

BOOL CSockListener::PostReuseAccept(CSockStream* pSock)
{	
	if( pSock->CreatePassiveSession(engine_, m_hSocket) )
	{	
		pSock->SetDisconnectReason( NULL );
		
		m_AcceptCS.Lock();
		++s_nCurPostedAccept;
		m_AcceptCS.Unlock();
		
		return TRUE;
	}
	
	return FALSE;
}

BOOL CSockListener::RegisterHandleTo(CServiceObject* pServiceObj, HANDLE hHandle)
{
	if (!pServiceObj)
		return FALSE;

	return ((CServiceObjectSock*)pServiceObj)->RegisterHandle(hHandle, (ULONG_PTR)this);
}
