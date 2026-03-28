#include "stdafx.h"
#include "PeerProcess.h"

#include "NetEngine.h"

/**************************************************************************************
//OVERLAPPED I/O MODEL
**************************************************************************************/
#define EVENT_EXIT_THREAD		0
#define EVENT_BREAK_WAIT		1

BS_IMPLEMENT_DYNCREATE(CPeerProcessOverlapped, CServiceObjectSock)
CPeerProcessOverlapped::CPeerProcessOverlapped(void)
{
	m_pSendingQueue = NULL;
	
	//사실 BREAK_WAIT, EXIT_THREAD, READ, WRITE 빼고 등록되지 않는다!!
	//하나의 소켓으로 몽땅 처리할꺼니깐.
	::ZeroMemory(m_Events, sizeof(m_Events));
	::ZeroMemory(m_Contexts, sizeof(m_Contexts));

	// reserved index (0, 1)
	m_Events[EVENT_EXIT_THREAD]	= ::WSACreateEvent();
	m_Events[EVENT_BREAK_WAIT]  = ::WSACreateEvent();

	RefreshEventCountToWait();
}

CPeerProcessOverlapped::~CPeerProcessOverlapped(void)
{
	SCOPED_LOCK_SINGLE(&m_CS);
	
	//그 외의 것들은 나중에 CIOContext 소멸자에서 알아서 Close해준다!!
	if( m_Events[EVENT_EXIT_THREAD] != NULL )
		::WSACloseEvent( m_Events[EVENT_EXIT_THREAD] );
	if( m_Events[EVENT_BREAK_WAIT] != NULL )
		::WSACloseEvent( m_Events[EVENT_BREAK_WAIT] );

	SAFE_DELETE( m_pSendingQueue );
}

bool CPeerProcessOverlapped::Init( CTask* pTask, ULONG_PTR Param )
{
	CServiceObject::Init( pTask, Param );

	SAFE_DELETE( m_pSendingQueue );

	m_pSendingQueue = new CQueMT9x<CMsg*>;
	m_pSendingQueue->Activate();

	return true;
}

//*
void CALLBACK send_completion( DWORD dwError, DWORD dwBytesTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags )
{
	CMsg* pMsg = (CMsg*)lpOverlapped;
	_ASSERT( pMsg->GetWrPos() == dwBytesTransferred );

	CSockDatagram* pSock = (CSockDatagram*)pMsg->m_Overlapped.hEvent;
	pSock->HandleEvent( dwBytesTransferred, pMsg );
}

void CALLBACK recv_completion( DWORD dwError, DWORD dwBytesTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags )
{

}
//*/

int CPeerProcessOverlapped::DoWork(void* pArg, DWORD dwThreadID)
{
	return -1;
	/*
	while( 1 )
	{		
		//for waiting completion callback!!
		::SleepEx( 1, TRUE );

		if( m_dwEventCountToWait == 0 )
			break;
				
		DWORD dwRval = ::WSAWaitForMultipleEvents( m_dwEventCountToWait, m_Events, FALSE, 1, FALSE );
		
		if( dwRval == EVENT_EXIT_THREAD )
			break;

		if( dwRval != WSA_WAIT_TIMEOUT )
		{
			if( dwRval == WSA_WAIT_FAILED )
			{
				DWORD dwError = ::WSAGetLastError();
				Put( _T("PEER ProcessOverlapped has occured! close worker thread!! error_code[%d]"), dwError );
				break;
			}
			else if( dwRval == EVENT_BREAK_WAIT )
			{
				::WSAResetEvent( m_Events[EVENT_BREAK_WAIT] );
				continue;
			}
			else
			{
				//gogogo!
				DWORD dwIndex = dwRval - WSA_WAIT_EVENT_0;	// 사실 바로 index로 써도 상관 없는데...

				::WSAResetEvent( m_Events[dwIndex] );

				CIOContext* pIOContext = GetCompletedOperationSrc( dwIndex );
				if( pIOContext == NULL )
					continue;

				CBaseSocket* pHandler = (CBaseSocket*)pIOContext->GetOwner();

				DWORD Flags;
				DWORD dwTransferred = 0;
				if( ::WSAGetOverlappedResult(pHandler->GetHandle(),(LPWSAOVERLAPPED)&pIOContext->m_Overlapped,&dwTransferred,FALSE,&Flags) == FALSE )
				{
					DWORD dwError = ::WSAGetLastError();
					if( dwError != WSA_IO_INCOMPLETE )
						CircleOfTheLife( pHandler, dwIndex );
				}
				else
				{
					int nEventResult;
					if( (nEventResult = pHandler->HandleEvent(dwTransferred,pIOContext)) != NERR_GOOD )
					{
						if( IS_CRITICAL(nEventResult) )
							CircleOfTheLife( pHandler, dwIndex );
					}
				}
			}
		}

		//check it out is there standing out what to post send!
		CBaseSocket* pSendingSock = CPeerNet::GetEngine()->m_pSocket;
		CMsg* pMsg;
		while( m_pSendingQueue->Deque(&pMsg,1) == TRUE )
		{
			if( pMsg == NULL )
				break;
			
			pMsg->m_Overlapped.hEvent = pSendingSock;

			DWORD dwSent;
			int rVal = ::WSASendTo( pSendingSock->GetHandle(),
									(WSABUF*)&pMsg->m_wsaBuf[0], pMsg->m_nBufNum,
									&dwSent, 0,
									(sockaddr*)&pMsg->m_addrRelated, sizeof(sockaddr),
									(LPWSAOVERLAPPED)&pMsg->m_Overlapped, send_completion );

			if( rVal == SOCKET_ERROR )
			{
				long nWSALastErr = ::WSAGetLastError();

				if( nWSALastErr != WSA_IO_PENDING )
				{
					CNetEngine::GetEngine()->DelMsg( pMsg );
					if( nWSALastErr == WSAEWOULDBLOCK || nWSALastErr == WSA_NOT_ENOUGH_MEMORY || nWSALastErr == WSAENOBUFS )
					{
						PutLog( LOG_NOTIFY_FILE, _T("WSASend() - NERR_TOO_MANY_OUTSTANDING: %d"), nWSALastErr );
					}
					else 
					{
						PutLog( LOG_NOTIFY_FILE, _T("WSASend() - Unknow NERROR: %d"), nWSALastErr );
					}
				}
			}
		}
	}

	return 0;
	*/
}

BOOL CPeerProcessOverlapped::EndWork()
{
	m_pSendingQueue->Deactivate();

	::WSASetEvent( m_Events[EVENT_EXIT_THREAD] );
	
	CMsg* pMsg = NULL;
	while( m_pSendingQueue->Deque(&pMsg,1,TRUE) == TRUE )
		m_pOwner->engine()->DelMsg( pMsg );

	return TRUE;
}

BOOL CPeerProcessOverlapped::RegisterHandle(HANDLE hObject, ULONG_PTR Data)
{
	_ASSERT(Data != NULL);
	SCOPED_LOCK_SINGLE(&m_CS);

	CSockDatagram* pSock = (CSockDatagram*)Data;

	CIOContext* pCtxRead  = pSock->GetContext(IO_READ);
	
	// set i/o contexts
	if( pCtxRead != NULL )
	{
		_ASSERT( m_Events[2] == NULL && m_Contexts[2] == NULL );
		m_Events[2]			= pCtxRead->GetEventHandle();
		m_Contexts[2]		= (ULONG_PTR)pCtxRead;
	}

	RefreshEventCountToWait();
	
	return TRUE;
}

BOOL CPeerProcessOverlapped::CircleOfTheLife(CEventHandler* pHandler, DWORD dwData)
{
	//do nothing!!
	return TRUE;
}

CIOContext* CPeerProcessOverlapped::GetCompletedOperationSrc(DWORD dwIndex)
{
	SCOPED_LOCK_SINGLE(&m_CS);

	if (m_Contexts[dwIndex] != NULL)
		return (CIOContext*)m_Contexts[dwIndex];	

	return NULL;
}

void CPeerProcessOverlapped::RefreshEventCountToWait()
{
	m_dwEventCountToWait = 0;
	for (int i = 0; i < WSA_MAXIMUM_WAIT_EVENTS; ++i)
	{
		if (m_Events[i] != NULL)
			++m_dwEventCountToWait;
	}

	::SetEvent(m_Events[EVENT_BREAK_WAIT]);
}


/**************************************************************************************
//IOCP MODEL
**************************************************************************************/
BS_IMPLEMENT_DYNCREATE(CPeerProcessIOCP, CServiceObjectSock)
CPeerProcessIOCP::CPeerProcessIOCP(void)
{
	m_nCurActiveThreadNum = 0;
	m_hIOCP = INVALID_HANDLE_VALUE;

	m_hIOCP = ::CreateIoCompletionPort( INVALID_HANDLE_VALUE, 0, 0, 2 );
}

CPeerProcessIOCP::~CPeerProcessIOCP(void)
{
	Close();
}

bool CPeerProcessIOCP::Init(CTask* pTask, ULONG_PTR Param)
{
	CServiceObject::Init( pTask, Param ); 

	return true;
}

void CPeerProcessIOCP::Close()
{	
	EndWork();

	if (m_hIOCP != NULL)
	{
		::CloseHandle(m_hIOCP);
		m_hIOCP = NULL;
	}
}

int CPeerProcessIOCP::DoWork(void* pArg, DWORD dwThreadID)
{	
	::InterlockedIncrement(&m_nCurActiveThreadNum);

	DWORD dwTransferred			= 0;
	ULONG_PTR CompletionKey		= 0;
	IIOContext* pIOContext		= nullptr;
	CEventHandler* pHandler		= nullptr;
	BOOL bGQCS = FALSE;
	long nResult = 0, nEventResult = NERR_GOOD;

	CNetEngine* engine = m_pOwner->engine();

	while( 1 )
	{
		bGQCS = ::GetQueuedCompletionStatus(m_hIOCP, &dwTransferred, &CompletionKey, (LPOVERLAPPED*)&pIOContext, INFINITE);
		if (CompletionKey == 0)
		{
			nResult = 0;
			break;
		}
		else
		{
			pHandler = (CEventHandler*)CompletionKey;
			if( bGQCS == FALSE )
			{
				if( pIOContext != NULL && pIOContext->GetContextType() == CONTEXT_TYPE_MSG )
				{
					engine->DelMsg( (CMsg*)pIOContext );
				}
			}

			if( (nEventResult = pHandler->HandleEvent(dwTransferred, pIOContext)) != NERR_GOOD )
			{
				if (pIOContext != NULL && pIOContext->GetContextType() == CONTEXT_TYPE_MSG)
				{
					engine->DelMsg((CMsg*)pIOContext);
				}
			}
		}
	}

	::InterlockedDecrement(&m_nCurActiveThreadNum);

	return nResult;
}

BOOL CPeerProcessIOCP::EndWork()
{
	SCOPED_LOCK_SINGLE(&m_CS);

	if (m_nCurActiveThreadNum != 0)
		return PostWakeupCompletionsToExit(m_nCurActiveThreadNum);

	return TRUE;
}

BOOL CPeerProcessIOCP::RegisterHandle( HANDLE hObject, ULONG_PTR CompletionKey )
{
	_ASSERT( m_hIOCP != NULL );

	HANDLE h = ::CreateIoCompletionPort( hObject, m_hIOCP, CompletionKey, m_nCurActiveThreadNum );
	if( h == NULL && GetLastError() != ERROR_INVALID_PARAMETER )
	{
		_ASSERT(FALSE);
		return FALSE;
	}

	return TRUE;
}

BOOL CPeerProcessIOCP::CircleOfTheLife(CEventHandler* pHandler, DWORD dwData)
{

	return TRUE;
}

BOOL CPeerProcessIOCP::PostCompletion(ULONG_PTR CompletionKey, CIOContext* pContext)
{
	if (m_hIOCP == NULL)
		return FALSE;

	// Post a completion packet
	if (::PostQueuedCompletionStatus(m_hIOCP, sizeof(CIOContext), CompletionKey, (LPOVERLAPPED)pContext) == FALSE)
		return FALSE;

	return TRUE;
}

BOOL CPeerProcessIOCP::PostWakeupCompletionsToExit(int nHowMany)
{
	for (int i = 0; i < nHowMany; i++)
	{
		if (PostCompletion(NULL, NULL) == -1)
			return FALSE;
	}

	return TRUE;
}

long CPeerProcessIOCP::GetActiveThreadNum(void)
{
	long nActiveThreadNum = 0;
	::InterlockedExchange(&nActiveThreadNum, m_nCurActiveThreadNum);
	return nActiveThreadNum;
}

/**************************************************************************************
//BLOCKING MODEL
**************************************************************************************/
BS_IMPLEMENT_DYNCREATE(CPeerProcessBlocking, CServiceObjectSock)
CPeerProcessBlocking::CPeerProcessBlocking(void)
{
	m_nCurActiveThreadNum = 0;
	m_bActivateThread = false;
	peer_net_ = nullptr;
}

CPeerProcessBlocking::~CPeerProcessBlocking(void)
{
	Close();
}

bool CPeerProcessBlocking::Init(CTask* pTask, ULONG_PTR Param)
{
	CServiceObject::Init( pTask, Param ); 

	m_bActivateThread = true;

	return true;
}

void CPeerProcessBlocking::Close()
{	
	EndWork();

	m_nCurActiveThreadNum = 0;
}

int CPeerProcessBlocking::DoWork(void* pArg, DWORD dwThreadID)
{	
	::InterlockedIncrement(&m_nCurActiveThreadNum);

	if( m_nCurActiveThreadNum > 1 )
		return NERR_UNKNOWN;

	CSockDatagram* pSocket = peer_net_->m_pSocket;
		
	do 
	{
		int nresult = pSocket->PostRecv();
		if( nresult != NERR_GOOD )
		{
//			if( nresult == NERR_PEER_DOESNT_RESPONSE )
//				continue;

			if( nresult == NERR_UNKNOWN || nresult == NERR_INVALID_SOCKET )
				break;
		}

	} while( m_bActivateThread == true );

	::InterlockedDecrement(&m_nCurActiveThreadNum);

	return NERR_GOOD;
}

BOOL CPeerProcessBlocking::EndWork()
{
	SCOPED_LOCK_SINGLE(&m_CS);

	m_bActivateThread = false;
	
	if( peer_net_ != NULL && peer_net_->m_pSocket != NULL )
	{
		peer_net_->m_pSocket->Close();
		peer_net_->m_pSocket =  NULL;
	}

	return TRUE;
}

BOOL CPeerProcessBlocking::RegisterHandle( HANDLE hObject, ULONG_PTR CompletionKey )
{

	return TRUE;
}

BOOL CPeerProcessBlocking::CircleOfTheLife(CEventHandler* pHandler, DWORD dwData)
{

	return TRUE;
}