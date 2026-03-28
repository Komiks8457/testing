#include "stdafx.h"
//#include "bslib/util/util.h"
#include "bsthread.h"

CTask::CTask() : m_MsgQue(0), m_nThreadCount(0)
{
	m_pThreadMgr		= NULL;
	m_TC.pServiceObject = NULL;
	m_TC.pTask			= NULL;
	engine_ = nullptr;
}

BOOL CTask::Create(CNetEngine* engine, CThreadManager* pThreadMgr, CRuntime* pRT_ServiceObj, ULONG_PTR Data)
{
	_ASSERT(pThreadMgr != NULL && m_pThreadMgr == NULL);
	_ASSERT(pRT_ServiceObj != NULL);

	m_pThreadMgr = pThreadMgr;

	OSVERSIONINFOEX osvi;
	::ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

	if (GetVersionEx((OSVERSIONINFO*) &osvi) == FALSE)
	{
		osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
		if (GetVersionEx((OSVERSIONINFO*) &osvi) == FALSE) 
			return FALSE;
	}

	if (osvi.dwPlatformId != VER_PLATFORM_WIN32_NT)
		return FALSE;	

	if (m_MsgQue.Open() == FALSE)
		return FALSE;

	m_TC.pTask			= this;
	m_TC.pServiceObject = (CServiceObject*)pRT_ServiceObj->CreateObject();
	engine_ = engine;
	
	return m_TC.pServiceObject->Init(this, Data);
}

CTask::~CTask()
{
	m_pThreadMgr = NULL;
	
	DeactivateMsgQue();
	if( m_TC.pServiceObject)
	{
		m_TC.pServiceObject->EndWork();
	}

	WaitForExitThreads();

	if (m_TC.pServiceObject)
	{
		delete m_TC.pServiceObject;
		m_TC.pServiceObject = NULL;
	}

	m_MsgQue.Close();
	m_ThreadIDs.clear();
	m_TC.Clear();
}

BOOL CTask::WaitForExitThreads()
{
	SCOPED_LOCK_SINGLE(&m_CS);

	if (m_pThreadMgr && m_ThreadIDs.size() > 0)
	{
		BOOL bResult = m_pThreadMgr->KillTask(this);		
		m_ThreadIDs.clear();

		return bResult;
	}	
	return FALSE;
}


BOOL	CTask::Suspend()
{
	SCOPED_LOCK_SINGLE(&m_CS);
	
	if (m_pThreadMgr && m_ThreadIDs.size() > 0)
		return m_pThreadMgr->SuspendTask(this);
	
	return FALSE;
}

BOOL	CTask::Resume()
{
	SCOPED_LOCK_SINGLE(&m_CS);
	
	if (m_pThreadMgr && m_ThreadIDs.size() > 0)
		return m_pThreadMgr->ResumeTask(this);
	
	return FALSE;
}

BOOL	CTask::StartJob(long ThreadNumToSpawn, long nPriority, DWORD dwStackSize, DWORD dwFlags, ProcessorAffinity* pTA)
{
	SCOPED_LOCK_SINGLE(&m_CS);

	_ASSERT(m_pThreadMgr && m_TC.pServiceObject);	

	if (!m_ThreadIDs.empty())
		return TRUE;	

	m_TC.nPriority			= nPriority;
	m_TC.dwStackSize		= dwStackSize;	

	DWORD* pIDs = new DWORD[ThreadNumToSpawn];
	::ZeroMemory(pIDs, sizeof(DWORD) * ThreadNumToSpawn);

	m_nThreadCount = ThreadNumToSpawn;
	m_MsgQue.SetMaxConcurrentThreads(m_nThreadCount);
	BOOL bResult = m_pThreadMgr->SpawnThreads(ThreadNumToSpawn, pIDs, &m_TC, dwFlags, pTA);
	
	if (bResult == TRUE)
	{		
		for (int i = 0; i < ThreadNumToSpawn; i++)
			m_ThreadIDs.push_back(pIDs[i]);
	}
	else
		m_pThreadMgr->KillTask(this);
	
	delete [] pIDs;
	return bResult;
}

BOOL CTask::AppendThread()
{
	SCOPED_LOCK_SINGLE(&m_CS);

	CThreadControl* pNewThread = m_pThreadMgr->AppendThread(NULL, &m_TC, 0);
	if (pNewThread == nullptr)
		return FALSE;

	++m_nThreadCount;
	m_MsgQue.SetMaxConcurrentThreads(m_nThreadCount);
	m_ThreadIDs.push_back(pNewThread->GetThreadID());
	return TRUE;
}

BOOL CTask::DequeMsg(CQueMT<CMsg*>::Items& msgs, DWORD dwTimeout)
{
	return m_MsgQue.Deque(msgs, dwTimeout);
}
