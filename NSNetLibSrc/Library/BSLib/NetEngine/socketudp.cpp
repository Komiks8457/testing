#include "stdafx.h"
#include "socketudp.h"
#include "NetEngine.h"
#include "ws2tcpip.h"

CSockDatagram::CSockDatagram(void) : CBaseSocket() , m_ctxRead(IO_READ)
{	
	Init();
	
	// reset handlers
	::ZeroMemory(FP_UDP_EVENTHANDLER, sizeof(FP_UDP_EVENTHANDLER));

	FP_UDP_RECVED_MSG_HANDLER = NULL;
	FP_UDP_INVOKER_SENDDATA = NULL;

	m_Type = eST_DGram;
}

CSockDatagram::~CSockDatagram(void)
{
	Close();
}

BOOL CSockDatagram::Create( unsigned short uPort )
{		
	if( m_hSocket == INVALID_SOCKET )
	{
		m_hSocket = ::socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
		if( m_hSocket == INVALID_SOCKET )
		{
			WSAERROR(TEXT("WSASocket()"), TEXT("CSockDatagram::Create"));
			Close();
			return FALSE;
		}
	}

	//set current sock addr
	SOCKADDR_IN		addr;
	::ZeroMemory( &addr, sizeof(addr) );

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = ADDR_ANY;
	addr.sin_port   = ::htons(uPort);	

	int nRet = ::bind(m_hSocket, (sockaddr*)&addr, sizeof(addr) );
	if( nRet == SOCKET_ERROR )
	{
		DWORD dwError = ::WSAGetLastError();
		VERIFYA1( FALSE, _T("bind udp sock error[%d]"), dwError );
		return FALSE;
	}

	m_uCreatedPort = GetLocalPort();

	PrepareSocket();

	return TRUE;
}

long CSockDatagram::Close()
{
	if (this == NULL)
	{
		return NERR_INVALID_SESSION;
	}

	SCOPED_LOCK_SINGLE(&m_CS);

	if (m_hSocket != INVALID_SOCKET)
	{
		::closesocket(m_hSocket);
		m_hSocket = INVALID_SOCKET;
	}

	if( engine_->IsShutdown() == FALSE)
	{
		Init();
	}

	return 0;
}

void CSockDatagram::Init()
{
	SCOPED_LOCK_SINGLE(&m_CS);

	m_pTask			= NULL;
	m_uCreatedPort	= 0;
	m_lFromSize		= sizeof( SOCKADDR_IN );
}

BOOL CSockDatagram::ReallocSocket()
{
	if( m_hSocket != INVALID_SOCKET )
		::closesocket( m_hSocket );
	
	m_hSocket = ::socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
	if( m_hSocket == INVALID_SOCKET )
	{
		VERIFY( FALSE );
		return FALSE;
	}

	//set current sock addr
	SOCKADDR_IN addr;
	::ZeroMemory( &addr, sizeof(addr) );

	addr.sin_family			= AF_INET;
	addr.sin_addr.s_addr	= ADDR_ANY;
	addr.sin_port			= ::htons( m_uCreatedPort );
	
	int nRet = ::bind( m_hSocket, (sockaddr*)&addr, sizeof(addr) );
	if( nRet == SOCKET_ERROR ) 
	{		
		DWORD dwError = ::WSAGetLastError();
		VERIFYA1( FALSE, _T("bind udp sock error[%d]"), dwError );
		return FALSE;
	}

	return TRUE;
}

void CSockDatagram::PrepareSocket()
{
	FP_UDP_INVOKER_SENDDATA = &CSockDatagram::_SendMsgDirect_Overlapped;
}

long CSockDatagram::SendMsg( SOCKADDR_IN* pAddr, CMsg* pMsg )
{
	_ASSERT( FP_UDP_INVOKER_SENDDATA != NULL );
	return (this->*FP_UDP_INVOKER_SENDDATA)( pAddr, pMsg );
}

long CSockDatagram::_ForgeMsgToSend( CMsg*& pMsg )
{
	if (m_hSocket == INVALID_SOCKET) 
		return NERR_UNKNOWN;

	pMsg->SetOperationTarget(IO_TARGET_DATA);

	return NERR_GOOD;
}

long CSockDatagram::_SendMsgDirect_Overlapped( SOCKADDR_IN* pAddr, CMsg* pMsg )
{
	SCOPED_LOCK_SINGLE(&m_CS);

	long rval = _ForgeMsgToSend(pMsg);
	if (rval != NERR_GOOD)
		return rval;

	pMsg->PrepareOperation();

	int len;
	do 
	{
		len = ::sendto( m_hSocket, (char*)pMsg->m_wsaBuf[0].buf, pMsg->m_wsaBuf[0].len, 0, (sockaddr*)pAddr, sizeof(sockaddr) );

	} while( len == 0 );

	return NERR_GOOD;
}

long CSockDatagram::_MsgReceived( CMsg* pRecvedMsg )
{
	if( m_pTask == NULL )
	{
		pRecvedMsg->ForceStopRead();
		engine_->DelMsg( pRecvedMsg );
		return NERR_TARGET_TASK_LOST;
	}
	m_pTask->EnqueMsg( pRecvedMsg );
	return NERR_GOOD;
}

long CSockDatagram::PostRecv()
{
	CMsg* pMsg = engine_->NewMsg( FALSE );

	sockaddr addr;
	int recv_data_len = ::recvfrom( m_hSocket, (char*)pMsg->GetBufferAt(0), DEFAULT_MSGBUF_SIZE, 0, (sockaddr*)&addr, &m_lFromSize );
	if( recv_data_len == SOCKET_ERROR )
	{
		DWORD wsa_error = ::WSAGetLastError();
		if( wsa_error == WSAECONNRESET )
			return NERR_PEER_DOESNT_RESPONSE;

		if( wsa_error == WSAEINTR || wsa_error == WSANOTINITIALISED )
			return NERR_INVALID_SOCKET;

		VERIFYA3( FALSE, _T("wsaerror[%d] sock[%d] sock_addr[%d]"), wsa_error, (DWORD)m_hSocket, m_lFromSize );
		return NERR_UNKNOWN;
	}

	long error_code = NERR_UNKNOWN;

	// 최소한 HEADER_SIZE 와 같거나 더 많은 데이터 있을때만 읽는다.
	if( recv_data_len < MSG_HEADER_SIZE )
		goto __FAILED_RECV;

	DWORD DataSize = pMsg->GetDataSize();
	if (pMsg->SetRdPos(0, TRUE) == FALSE)
	{
		error_code = NERR_INVALID_MSGSIZE;
		goto __FAILED_RECV;
	}

	if (pMsg->SetWrPos(DataSize, TRUE) == FALSE)
	{
		error_code = NERR_INVALID_MSGSIZE;
		goto __FAILED_RECV;
	}

	error_code = _MsgReceived( pMsg );
	return error_code;

__FAILED_RECV:
	engine_->DelMsg( pMsg );
	return error_code;
}

long CSockDatagram::HandleEvent(DWORD dwTransferred, void* pCompleted)
{
	return NERR_GOOD;
}

BOOL CSockDatagram::RegisterHandleTo(CServiceObject* pServiceObj, HANDLE hHandle)
{
	if( pServiceObj == NULL )
	{
		//뭐야 장난쳐?
		_ASSERT( pServiceObj != NULL );
		return FALSE;
	}
	
	if( ((CServiceObjectSock*)pServiceObj)->RegisterHandle(hHandle,(ULONG_PTR)this) == FALSE )
		return FALSE;

	return TRUE;
}

BOOL CSockDatagram::GetSockAddr( SOCKADDR_IN& addr )
{
	int len = sizeof(addr);
	::ZeroMemory( &addr, len );

	if( ::getsockname(m_hSocket,(sockaddr*)&addr,&len) != 0 )
		return FALSE;

 	PHOSTENT phe = ::gethostbyname("localhost");
	if( phe == NULL ) 
		return FALSE;

	phe = ::gethostbyname( phe->h_name );
	if( phe == NULL )
		return FALSE;

	addr.sin_addr.s_addr = ((LPIN_ADDR)phe->h_addr)->s_addr;

	return TRUE;
}
