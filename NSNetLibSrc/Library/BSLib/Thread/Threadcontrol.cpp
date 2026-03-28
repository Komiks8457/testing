#include "stdafx.h"
#include "bsthread.h"

#include <process.h>
#include <time.h>

//////////////////////////////////////////////////////////////////////
// CThreadControl
//////////////////////////////////////////////////////////////////////
CThreadControl::CThreadControl()
{
	m_pThreadMgr	 = NULL;
	m_bJobTerminated = FALSE;
	m_bSuspended	 = TRUE;
	
	m_hThread		 = NULL;
	m_dwThreadID	 = NULL;
	m_uiErrCode		 = 0;

	m_dwBSNetThreadID= NULL;	
}

CThreadControl::~CThreadControl()
{
	Kill();
}

BOOL CThreadControl::Create(CThreadManager* pThreadMgr, THREADCONF* pThreadConf, DWORD dwBSNetThreadID)
{
	_ASSERT(pThreadMgr != NULL && pThreadConf != NULL && dwBSNetThreadID != NULL);

	m_pThreadMgr = pThreadMgr;

	m_ThreadConf = *pThreadConf;

	m_dwBSNetThreadID	= dwBSNetThreadID;

	_ASSERT(m_hThread == NULL && m_ThreadConf.pTask != NULL && m_ThreadConf.pServiceObject != NULL);

	if (CSimpleThread::Create(this, pThreadConf->dwStackSize, pThreadConf->nPriority) == FALSE)
		return FALSE;
	
	_ASSERT(m_hThread != NULL);

	return TRUE;
}

BOOL CThreadControl::Kill()
{
	SCOPED_LOCK_SINGLE(&m_CS);

	if (m_hThread == NULL)	
		return FALSE;

	/*
	if (m_ThreadConf.pServiceObject)
	{
		Put(_T("Thread 종료 시작 : %S"), m_ThreadConf.pServiceObject->GetRuntimeClass()->m_lpszClassName);
	}
	*/

	Resume();
	
	m_bJobTerminated	= TRUE;

	if (m_ThreadConf.pServiceObject)
		m_ThreadConf.pServiceObject->EndWork();
	
	CSimpleThread::Stop();

	//Put(_T("Thread 종료 완료 : %S"), m_ThreadConf.pServiceObject->GetRuntimeClass()->m_lpszClassName);

	m_ThreadConf.pServiceObject = NULL;
	return TRUE;
}

void CThreadControl::ThreadProc(LPVOID pThreadCtrl)
{		
	if (pThreadCtrl == NULL)
	{
		//Put(_T("CThreadControl == NULL"));
		return;
	}

	CServiceObject* pJob	= NULL;
	m_uiErrCode				= (UINT)-1;

	pJob = m_ThreadConf.pServiceObject;
	if (pJob == NULL)
		return;

	::srand(1);
	::srand((unsigned)time(NULL));
	
//	while (!m_bKillThread)
//	{
		m_uiErrCode = pJob->DoWork(&m_bJobTerminated, GetBSNetThreadID() - 1);
//	}

	_endthreadex(m_uiErrCode);
}

