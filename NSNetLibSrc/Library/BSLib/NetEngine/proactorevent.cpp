#include "StdAfx.h"
#include "ProactorEvent.h"
#include "NetEngine.h"

BS_IMPLEMENT_DYNCREATE(CProactorEvent, CServiceObjectSock)

#define EVENT_EXIT_THREAD		0
#define EVENT_BREAK_WAIT		1

////////////////////////////////////////////////////////////////////////
// Event Handle만 미리 생성해놓자...
////////////////////////////////////////////////////////////////////////
CProactorEvent::CProactorEvent()
{
	::ZeroMemory(m_Events, sizeof(m_Events));
	::ZeroMemory(m_Contexts, sizeof(m_Contexts));
	
	// prepare dummy event pool
	HANDLE hDummyEvent = NULL;
	for (int i = 0; i < WSA_MAXIMUM_WAIT_EVENTS; ++i)
	{
		hDummyEvent = ::WSACreateEvent();
		_ASSERT(hDummyEvent != NULL);
		m_DummyEvents.push_back(hDummyEvent);
	}

	// reserved index (0, 1)
	m_Events[EVENT_EXIT_THREAD]	= PopDummyEvent();	// For End of Event Loop
	m_Events[EVENT_BREAK_WAIT]  = PopDummyEvent();	// Dummy Event (NULL 넣으면 Invalid Handle이라서 WFME 함수가 바로 리턴해 버리거덩...)

	m_nCurActiveThreadNum = 0;
	m_dwEventCountToWait  = 0;

	RefreshEventCountToWait();
}

CProactorEvent::~CProactorEvent()
{
	SCOPED_LOCK_SINGLE(&m_CS);

	// 사용중인 dummy event들 반납시키고
	for (int i = 0; i < WSA_MAXIMUM_WAIT_EVENTS; i += 2)
	{
		// context가 NULL 이라면 dummy event가 세팅되어 있을 수 있다는 얘기...
		// (부분 부정을 한 이유는 빠진 이빨 매꾸는 경우에만 Dummy Event가 세팅 되거덩... 
		// 아예 뒷쪽이면 context, event 둘다 세팅되지 않는다구...)
		if (m_Contexts[i] == NULL)
		{
			_ASSERT(m_Contexts[i + 1] == NULL);

			PushDummyEvent(m_Events[i]);
			PushDummyEvent(m_Events[i + 1]);
		}
	}
	
	// 홀랑 이벤트 삭제 시키자.
	_ASSERT(m_DummyEvents.size() == WSA_MAXIMUM_WAIT_EVENTS);	// 최초에 만든만큼 전부 반납 했냐?

	HANDLES::iterator it = m_DummyEvents.begin();
	HANDLES::iterator it_end = m_DummyEvents.end();
	for (; it != it_end; ++it)
		::WSACloseEvent((*it));
	m_DummyEvents.clear();
}

BOOL CProactorEvent::EraseSlot(int nIndex)
{
	if (nIndex < 2 || nIndex >= WSA_MAXIMUM_WAIT_EVENTS)
		return FALSE;

	_ASSERT(nIndex % 2 == 0);

	DWORD dwOldEventCount = m_dwEventCountToWait;
	
	m_Events[nIndex]		= NULL;
	m_Events[nIndex + 1]	= NULL;
	
	m_Contexts[nIndex]		= NULL;
	m_Contexts[nIndex + 1]	= NULL;

	RefreshEventCountToWait();

	if (dwOldEventCount == m_dwEventCountToWait)
	{
		// 이빨을 빼먹었구나... dummy event 세팅해주자...
		m_Events[nIndex]		= PopDummyEvent();
		m_Events[nIndex + 1]	= PopDummyEvent();
	}

	return TRUE;
}

void CProactorEvent::RefreshEventCountToWait()
{
	m_dwEventCountToWait = 0;
	for (int i = 0; i < WSA_MAXIMUM_WAIT_EVENTS; ++i)
	{
		if (m_Events[i] != NULL)
			++m_dwEventCountToWait;
	}

	::SetEvent(m_Events[EVENT_BREAK_WAIT]);
}

/////////////////////////////////////////////////////////////////////////
// 얘는 클라이언트 목적으로만 쓰이니깐... 사실 이렇게 많이
// 생길 일도 없겠지만서도... 암튼 각 소켓Wrapper당 Read/Write용으로
// 하나씩 있으니... 총 31개까지만 커버해준다.(처음 2개는 각각 [루프 종료용 + 이빨 채우기용] 이벤트로 사용한다....)
////////////////////////////////////////////////////////////////////////
BOOL CProactorEvent::RegisterHandle(HANDLE hObject, ULONG_PTR Data)
{
	_ASSERT(Data != NULL);

	SCOPED_LOCK_SINGLE(&m_CS);

	CSockStream* pSock = (CSockStream*)Data;
	
	int nIndex = FindEmptySlot();
	if (nIndex < 0)
	{
		_ASSERT(FALSE);	// 빈 슬롯이 없다는데?
		return FALSE;
	}

	_ASSERT((nIndex % 2) == 0);

	// release dummy handles and set real event handles
	PushDummyEvent(m_Events[nIndex]);	// NULL 이면 무시되니깐... 신경쓰지 말고 걍 호출해 버리자
	PushDummyEvent(m_Events[nIndex + 1]);

	CIOContext* pCtxRead  = pSock->GetContext(IO_READ);
	CIOContext* pCtxWrite = pSock->GetContext(IO_WRITE);

	m_Events[nIndex]		= pCtxRead->GetEventHandle();
	m_Events[nIndex + 1]	= pCtxWrite->GetEventHandle();
	
	// set i/o contexts
	m_Contexts[nIndex]		= (ULONG_PTR)pCtxRead;
	m_Contexts[nIndex + 1]	= (ULONG_PTR)pCtxWrite;

	RefreshEventCountToWait();

	return TRUE;
}

CIOContext* CProactorEvent::GetCompletedOperationSrc(DWORD dwIndex)
{
	SCOPED_LOCK_SINGLE(&m_CS);

	if (m_Contexts[dwIndex] != NULL)
		return (CIOContext*)m_Contexts[dwIndex];	
	
	return NULL;
}

BOOL CProactorEvent::CircleOfTheLife(CEventHandler* pHandler, DWORD dwEventSlot)
{
	SCOPED_LOCK_SINGLE(&m_CS);

	// 각 세션별로 R/W 용으로 2개의 슬롯이 할당되므로, 인자로 넘어오는 
	// dwEventSlot이 'W' 용으로 할당된 경우 그냥 사용해 버리면 배열 뒷쪽에 있는 session의 R 슬롯을
	// 홀랑 조져버리는 수가 있기땜에 시작 인덱스를 찾기 위해서 이런 수작을 부렸다. (ex: 3 -> 2, 1 -> 0, ... )
	long nBeginSlot = (dwEventSlot >> 1) << 1;
	
	EraseSlot(nBeginSlot);

	CSockStream* pSock = (CSockStream*)pHandler;
	if (pSock != NULL)
	{
//		PutLog(LOG_NOTIFY_FILE, "CloseSession() called from CircleOfTheLife - can't find socket object");
		return m_pOwner->engine()->CloseSession(pSock->GetSessionID(), DISCONNECT_REASON_CLOSED_BY_PEER);
	}
	else
		return FALSE;				
}

int CProactorEvent::DoWork(void* pArg, DWORD dwThreadID)
{
	BOOL* bAborted = (BOOL*)pArg;

	if ((*bAborted) != 0)
		return 0;	

	::InterlockedIncrement(&m_nCurActiveThreadNum);

	DWORD dwRval			= 0;
	DWORD dwIndex			= 0;
	DWORD dwTransferred		= 0;
	DWORD Flags				= 0;
//	WSAEVENT* lpEvents		= NULL;
	CIOContext* pIOContext	= NULL;
	CSockStream* pHandler	= NULL;

	long nResult = 0, nEventResult = NERR_GOOD;	
	
	while ((*bAborted) == 0)
	{
		if (m_dwEventCountToWait == 0)
			break;

		dwRval = ::WSAWaitForMultipleEvents(m_dwEventCountToWait, m_Events, FALSE, INFINITE, FALSE);
		
		if (dwRval == WSA_WAIT_FAILED)
			break;
		else if (dwRval == EVENT_EXIT_THREAD)
			break;
		else if (dwRval == EVENT_BREAK_WAIT)
		{
			::WSAResetEvent(m_Events[EVENT_BREAK_WAIT]);
			continue;
		}
		else if (dwRval == WSA_WAIT_TIMEOUT)		
			continue;	
		
		dwIndex = dwRval - WSA_WAIT_EVENT_0;	// 사실 바로 index로 써도 상관 없는데...

		::WSAResetEvent(m_Events[dwIndex]);

		pIOContext = GetCompletedOperationSrc(dwIndex);
		
		if (pIOContext == NULL)
			continue;		
		
		pHandler = (CSockStream*)pIOContext->GetOwner();
		
		dwTransferred = 0;

		if (::WSAGetOverlappedResult(pHandler->GetHandle(), (LPWSAOVERLAPPED)&pIOContext->m_Overlapped, &dwTransferred, FALSE, &Flags) == FALSE)
		{		
			if (::WSAGetLastError() != WSA_IO_INCOMPLETE)
				CircleOfTheLife(pHandler, dwIndex);
		}
		else
		{
			if (dwTransferred == 0)
			{
				CircleOfTheLife(pHandler, dwIndex);
				continue;
			}
			else
			{
				if ((nEventResult = pHandler->HandleEvent(dwTransferred, pIOContext)) != NERR_GOOD)
				{
					if (IS_CRITICAL(nEventResult))
						CircleOfTheLife(pHandler, dwIndex);
				}
			}
		}
	}

	::InterlockedDecrement(&m_nCurActiveThreadNum);

	return nResult;
}

BOOL CProactorEvent::EndWork()
{
	::WSASetEvent(m_Events[EVENT_EXIT_THREAD]);
		
	return TRUE;
}

BOOL CProactorEvent::IsEventLoopDone()
{
	long cur_active_threads = 0;
	::InterlockedExchange(&cur_active_threads, m_nCurActiveThreadNum);
	return (cur_active_threads == 0);
}


/**********************************************
// WaitForMultipleObjects Test

HANDLE hChange = NULL;
HANDLE Handles[2] = {NULL, };
BOOL bExit = FALSE;

DWORD WINAPI ThreadProc(LPVOID lpParameter)
{
	OutputDebugString("Thread Proc Has Started\n");

	SetEvent(hChange);

	DWORD dwRval = ::WaitForMultipleObjects(2, Handles, FALSE, INFINITE);

	int idx = dwRval - WAIT_OBJECT_0;

	char buf[256];
	if ( S_OK != SF_sprintf(buf, ("%d번째 event가 signaled 되었다!\n"), idx) )
	{
		_ASSERT(FALSE);
		return 0;
	}

	OutputDebugString(buf);

	bExit = TRUE;

	return 0;
}

void main()
{
	hChange = CreateEvent(NULL, FALSE, FALSE, NULL);
	Handles[0] = CreateEvent(NULL, FALSE, FALSE, NULL);
	Handles[1] = CreateEvent(NULL, FALSE, FALSE, NULL);

	::CreateThread(NULL, 0, ThreadProc, 0, 0, NULL);
	
	WaitForSingleObject(hChange, INFINITE);

	OutputDebugString("Change the handle array\n");

	HANDLE temp = Handles[0];
	Handles[0] = Handles[1];
	Handles[1] = temp;

	SetEvent(Handles[0]);

	while (!bExit)
	{

	}
}

간단하게 함 만들어서 돌려봤는데 
이 테스트의 결과로 보아서... 당연한 것이긴 하지만 WaitForMultipleObjects는
내부적으로 인자로 넘어가는 event 배열의 복사본을 유지하는 듯 하다.
즉... 어떤 session이 close될때 event 배열에 이빨이 빠지는 것을 
방지하기 위해서 빈칸 매꾸기를 할 경우... multi-threading 환경이라면
엉뚱한 slot으로 completion event가 날아 갈 수 있기땜에 골때린 경우가
발생할 수 있겠다... 물론 이놈은 win9x 시리즈 클라이언트 전용이라 
적어도 SRO 프로젝트라면 하나의 session만 생성되기땜에 큰 하자는 없지만
차후를 위해서 다소 overhead를 안고 가더라도 한번 특정 세션에 slot index가 할당되면 
변경되는 일이 없도록 했다.
**************************************************/