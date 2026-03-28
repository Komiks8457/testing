#include "StdAfx.h"
#include "Accept.h"
#include "NetEngine.h"

///////////////////////////////////////////////////////
// CAcceptorBSD
///////////////////////////////////////////////////////
BS_IMPLEMENT_DYNCREATE(CAcceptorBSD, CServiceObjectSock)

CAcceptorBSD::CAcceptorBSD()
{
	m_hListen		= INVALID_SOCKET;
	m_pEventHandler	= NULL;
}

CAcceptorBSD::~CAcceptorBSD()
{
	EndWork();
}

BOOL CAcceptorBSD::RegisterHandle(HANDLE hObject, ULONG_PTR CompletionKey)
{
	_ASSERT(m_hListen == INVALID_SOCKET);

	m_hListen = (SOCKET)hObject;	
	m_pEventHandler = (CEventHandler*)CompletionKey;

	return TRUE;	
}

int CAcceptorBSD::DoWork(void* pArg, DWORD dwThreadID)
{
	BOOL* bAborted = (BOOL*)pArg;

	if ((*bAborted) != 0)
		return 0;
	
	struct sockaddr_in	addr;
	int	nDummy = sizeof(addr);

	// event loop.
	while ((*bAborted) == 0)
	{
		if (m_hListen != INVALID_SOCKET)
		{		
			SOCKET s = ::accept(m_hListen, (struct sockaddr*)&addr, &nDummy);

			if (s == INVALID_SOCKET)
				break;
			
			long nEventResult = NERR_GOOD;

			if ((nEventResult = m_pEventHandler->HandleEvent(0, (void*)s)) != NERR_GOOD)
				continue;
		}
	}

	return 0;
}

BOOL CAcceptorBSD::EndWork()
{	
	return TRUE;
}

///////////////////////////////////////////////////////
// CAcceptor
///////////////////////////////////////////////////////
BS_IMPLEMENT_DYNCREATE(CAcceptorEvent, CServiceObjectSock)

CAcceptorEvent::CAcceptorEvent()
{	
	m_pEventHandler	= NULL;
	m_hObject		= INVALID_SOCKET;

	m_Events[0] = ::WSACreateEvent();	// accept new connection !
	m_Events[1] = ::WSACreateEvent();	// finish loop !

}

CAcceptorEvent::~CAcceptorEvent()
{
	SCOPED_LOCK_SINGLE(&m_CS);	

	::WSACloseEvent(m_Events[0]);
	::WSACloseEvent(m_Events[1]);
}

//	Use WSAEventSelect - only use FD_ACCEPT bit
BOOL CAcceptorEvent::RegisterHandle(HANDLE hObject, ULONG_PTR Data)
{
	_ASSERT(Data != NULL);

	SCOPED_LOCK_SINGLE(&m_CS);	
	
	if (::WSAEventSelect((SOCKET)hObject, m_Events[0], FD_ACCEPT) == SOCKET_ERROR)
	{		
//		AssertString("BSNet", __LINE__, __FILE__, "RegisterHandle");
		::closesocket((SOCKET)hObject);
		return FALSE;
	}
	
	SetSockHandle((SOCKET)hObject);
	m_pEventHandler	= (CEventHandler*)Data;	

	return TRUE;
}

BOOL CAcceptorEvent::CircleOfTheLife(CEventHandler* pHandler, DWORD dwData)
{
	return TRUE;
}

int CAcceptorEvent::DoWork(void* pArg, DWORD dwThreaID)
{	
	BOOL* bAborted = (BOOL*)pArg;

	WSANETWORKEVENTS	events;		
//	CEventHandler*		pHandler	= NULL;
	long				nResult		= 0;
	long				nEventResult = NERR_GOOD;

	::InterlockedIncrement(&m_nCurActiveThreadNum);
	
	DWORD dwIndex = (DWORD) -1;

	// event loop.
	while ((*bAborted) == 0)
	{	
		dwIndex = WSAWaitForMultipleEvents(ACCEPTOR_EVENT_NUM, m_Events, FALSE, WSA_INFINITE, FALSE);
		if (dwIndex != WSA_WAIT_EVENT_0)
			break;
		
		if (WSAEnumNetworkEvents(GetSockHandle(), m_Events[WSA_WAIT_EVENT_0], &events) == SOCKET_ERROR)	
			break;		
		
		if (events.lNetworkEvents & FD_ACCEPT)
		{
			if (events.iErrorCode[FD_ACCEPT_BIT] != 0)
				break;
			
			if ((nEventResult = m_pEventHandler->HandleEvent(0, (void*)NULL)) != NERR_GOOD)
			{
				if (IS_CRITICAL(nEventResult))
					break;
				
				continue;
			}
		}
	}

	::InterlockedDecrement(&m_nCurActiveThreadNum);
	return nResult;
}

BOOL CAcceptorEvent::EndWork()
{		
	SCOPED_LOCK_SINGLE(&m_CS);
	
	::WSASetEvent(m_Events[1]);		
	return TRUE;	
}

