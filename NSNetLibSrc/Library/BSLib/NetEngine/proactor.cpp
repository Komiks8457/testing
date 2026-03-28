#include "StdAfx.h"
#include "Proactor.h"
#include "NetEngine.h"

BS_IMPLEMENT_DYNCREATE(CProactorIOCP, CServiceObjectSock)

CProactorIOCP::CProactorIOCP(long nConcurrentThreadNum)
{	
	m_hIOCP = NULL;

	m_nCurActiveThreadNum  = 0;
	m_nConcurrentThreadNum = nConcurrentThreadNum;

	if (nConcurrentThreadNum == 0)
		m_nConcurrentThreadNum = CNetEngine::GetEngineConfig().io_thread_count;

	m_hIOCP = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, m_nConcurrentThreadNum);
	_ASSERT(m_hIOCP != NULL);
}

CProactorIOCP::~CProactorIOCP()
{
	Close();
}

void CProactorIOCP::Close()
{
	EndWork();

	if (m_hIOCP != NULL)
	{
		::CloseHandle(m_hIOCP);
		m_hIOCP = NULL;
	}
}

BOOL CProactorIOCP::RegisterHandle(HANDLE hObject, ULONG_PTR CompletionKey)
{
	_ASSERT(m_hIOCP != NULL);

	HANDLE h = ::CreateIoCompletionPort(hObject, m_hIOCP, CompletionKey, m_nConcurrentThreadNum);
	if (h == NULL && GetLastError() != ERROR_INVALID_PARAMETER )	
	{
		/*PutLog(LOG_FATAL, _T("케엑! CreateIoCompletionPort(%d, %d, %d, %d) 실패!!! [LastError: %d]"),
		hObject, m_hIOCP, dwCompletionKey, m_nConcurrentThreadNum, GetLastError());*/
		_ASSERT(FALSE);
		return FALSE;
	}

	return TRUE;	
}

int CProactorIOCP::DoWork(void* pArg, DWORD dwThreadID)
{
	BOOL* bAborted = (BOOL*)pArg;

	if ((*bAborted) != 0)
		return 0;

	::InterlockedIncrement(&m_nCurActiveThreadNum);

	DWORD dwTransferred			= 0;
	ULONG_PTR CompletionKey		= 0;
	IIOContext* pIOContext		= nullptr;
	CEventHandler* pHandler		= nullptr;
	BOOL bGQCS = FALSE;
	long nResult = 0, nEventResult = NERR_GOOD;

	CNetEngine* engine = m_pOwner->engine();

	// event loop.
	while ((*bAborted) == 0)
	{
		bGQCS = ::GetQueuedCompletionStatus(m_hIOCP, &dwTransferred, &CompletionKey, (LPOVERLAPPED*)&pIOContext, INFINITE);
	
		// 호오~ 쑤레드 빠져나오라구?
		if (CompletionKey == 0)
		{			
			nResult = 0;
			break;
		}
		else
		{
			pHandler = (CEventHandler*)CompletionKey;
			
			_ASSERT(IsValidAddress(pHandler, sizeof(CEventHandler)));

			if( pIOContext == NULL )
			{
				ASSERT(FALSE);
				continue;
			}
			
			if( pIOContext->GetOperationMode() == IO_DISCONNECT )
			{
				CircleOfTheLife(pHandler, DISCONNECT_REASON_OVERLAPPED);
				continue;
			}

			CBaseSocket* pSock = (CBaseSocket*)pHandler;

			if ((nEventResult = pHandler->HandleEvent(dwTransferred, pIOContext)) != NERR_GOOD)
			{
				//////////////////////////////////////////////////////////
				//	CMsg를 Overlapped Struct로 사용했을때 
				//	MSG를 FREE하지 못했기때문에 일단 DELMSG하자				
				//////////////////////////////////////////////////////////
				if (pIOContext->GetContextType() == CONTEXT_TYPE_MSG)
					engine->DelMsg((CMsg*)pIOContext);

				if (nEventResult == NERR_SESSION_LOST)
				{
					if (bGQCS)
					{
						CircleOfTheLife(pHandler, DISCONNECT_REASON_CLOSED_BY_PEER);
					}
					else
					{
						if (pSock->GetType() == CBaseSocket::eST_Stream)
							static_cast<CSockStream*>(pSock)->set_disconnect_error_code(GetLastError());

						CircleOfTheLife(pHandler, DISCONNECT_REASON_SESSION_LOST);
					}
				}
				else
				{
					if (pSock->GetType() == CBaseSocket::eST_Stream)
						static_cast<CSockStream*>(pSock)->set_disconnect_error_code(nEventResult);

					CircleOfTheLife(pHandler, DISCONNECT_REASON_NET_ENGINE_ERROR);				
				}							
				continue;
			}
		}
	}

	::InterlockedDecrement(&m_nCurActiveThreadNum);

	return nResult;
}

BOOL CProactorIOCP::EndWork()
{
	SCOPED_LOCK_SINGLE(&m_CS);

	if (m_nCurActiveThreadNum != 0)
        return PostWakeupCompletionsToExit(m_nCurActiveThreadNum);
	
	return TRUE;
}

BOOL CProactorIOCP::IsEventLoopDone()
{
	long nActiveThreadNum = 0;
	::InterlockedExchange(&nActiveThreadNum, m_nCurActiveThreadNum);
	return (nActiveThreadNum == 0);
}

BOOL CProactorIOCP::PostCompletion(ULONG_PTR CompletionKey, CIOContext* pContext)
{
	if (m_hIOCP == NULL)
		return FALSE;

	// Post a completion packet
	if (::PostQueuedCompletionStatus(m_hIOCP, sizeof(CIOContext), CompletionKey, (LPOVERLAPPED)pContext) == FALSE)
		return FALSE;

	return TRUE;
}

BOOL CProactorIOCP::PostWakeupCompletionsToExit(int nHowMany)
{
	for (int i = 0; i < nHowMany; i++)
	{
		if (PostCompletion(NULL, NULL) == -1)
			return FALSE;
	}

	return TRUE;
}
 
long CProactorIOCP::GetActiveThreadNum(void)
{
	long nActiveThreadNum = 0;
	::InterlockedExchange(&nActiveThreadNum, m_nCurActiveThreadNum);
	return nActiveThreadNum;
}

BOOL CProactorIOCP::CircleOfTheLife(CEventHandler* pHandler, DWORD dwDisconnectReason)
{
//	AssertString("BSNet", __LINE__, __FILE__, "CircleOfTheLife");
	_ASSERT(IsValidAddress(pHandler, sizeof(pHandler)));
	
	CSockStream* pSock = (CSockStream*)pHandler;

	return m_pOwner->engine()->DisconnectCheck( pSock, dwDisconnectReason );
}















