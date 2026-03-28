#include "stdafx.h"
#include "bsthread.h"

CThreadManager::CThreadManager() 
	: m_dwIDToAlloc(0)
{
	_SetThreadIdealProcessor = NULL;
	
	if (IsNT() == TRUE)
	{
		HINSTANCE _kernel_32 = LoadLibrary(_T("kernel32.dll"));
		if (_kernel_32 != NULL)
		{
			_SetThreadIdealProcessor = (LPFN_SETTHREADIDEAL_PROCESSOR)::GetProcAddress(_kernel_32, "SetThreadIdealProcessor");
			FreeLibrary(_kernel_32);
		}
	}
}
 
CThreadManager::~CThreadManager()
{
	Close();
}

DWORD CThreadManager::AllocID()
{
	m_dwIDToAlloc++;
	if (m_dwIDToAlloc == 0xFFFFFFFF)	
		m_dwIDToAlloc = 1;
	return m_dwIDToAlloc;
}

void CThreadManager::Close()
{
	if (m_Threads.empty() == FALSE)
		CleanupThreads(&m_Threads);
}

void CThreadManager::CleanupThreads(THREADMAP* pThreads)
{	
	CThreadControl* pThreadCtrl = NULL;
	for (THREADMAP::reverse_iterator it = pThreads->rbegin(); it != pThreads->rend(); ++it)
	//for (THREADMAP::iterator it = pThreads->begin(); it != pThreads->end(); ++it)
	{
		pThreadCtrl = (*it).second;
		pThreadCtrl->Kill();
		delete pThreadCtrl;
		pThreadCtrl = NULL;
	}			
	pThreads->clear();
}

BOOL CThreadManager::IsNT()
{
	OSVERSIONINFOEX osvi;

	::ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

	BOOL bOsVersionInfoEx = GetVersionEx( (OSVERSIONINFO*) &osvi );
	if( !bOsVersionInfoEx )
	{
		osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
		if (!GetVersionEx((OSVERSIONINFO*) &osvi)) 
			return FALSE;
	}

	if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT)
		return TRUE;
	else
		return FALSE;
}

BOOL CThreadManager::SpawnThreads(long n, DWORD ThreadIDs[], THREADCONF* pThreadConf, DWORD dwFlags, ProcessorAffinity* pPA)
{
	_ASSERT(pThreadConf != NULL);

	SCOPED_LOCK_SINGLE(&m_CS);	

	int prev_processor_id = -1;

	if (pPA != NULL)
	{
		if (IsNT() != NULL)
		{
			SYSTEM_INFO sysinfo;
			::GetSystemInfo(&sysinfo);

			sysinfo.dwNumberOfProcessors;

			CLAMP(pPA->begin, 0, (int)sysinfo.dwNumberOfProcessors - 1);
			CLAMP(pPA->end,   0, (int)sysinfo.dwNumberOfProcessors - 1);

			prev_processor_id = pPA->begin;
		}
		else
			pPA = NULL;
	}
	
	for (long i = 0; i < n; i++)
	{		
		CThreadControl* pNewThread = AppendThread((ThreadIDs == NULL) ? NULL : &ThreadIDs[i], pThreadConf, dwFlags);
		if (pNewThread == NULL)
			return FALSE;

		if (pPA != NULL)
		{
			int new_one = pPA->SelectProcessor(prev_processor_id);
			if (new_one >= 0 && _SetThreadIdealProcessor != NULL)
			{
				_SetThreadIdealProcessor(pNewThread->GetThreadHandle(), new_one);
				prev_processor_id = new_one;
			}
		}
	}
	return TRUE;
}

CThreadControl* CThreadManager::AppendThread(DWORD* pdwThreadID, THREADCONF* pThreadConf, DWORD dwFlags)
{
	DWORD dwBSNetThreadID = AllocID();
	
	_ASSERT(dwBSNetThreadID != NULL);

	CThreadControl* pNewThread = new CThreadControl;
	if (pNewThread->Create(this, pThreadConf, dwBSNetThreadID) == FALSE)
	{
		delete pNewThread;
		return NULL;
	}
	
	DWORD dwID = pNewThread->GetThreadID();
	if (pdwThreadID != NULL)	
		*pdwThreadID = dwID;	

	m_Threads.insert(THREADMAP::value_type(dwBSNetThreadID, pNewThread));	

	if (dwFlags == CREATE_SUSPENDED)
		pNewThread->Suspend();
	else	
		pNewThread->Resume();
		
	return pNewThread;
}

BOOL CThreadManager::SuspendThread(CThreadControl* pThreadCtrl)
{
	_ASSERT(pThreadCtrl);
	return ((pThreadCtrl->Suspend()) ? TRUE : FALSE);
}

BOOL CThreadManager::ResumeThread(CThreadControl* pThreadCtrl)
{
	_ASSERT(pThreadCtrl);
	return ((pThreadCtrl->Resume()) ? TRUE : FALSE);
}

BOOL CThreadManager::KillThread(CThreadControl* pThreadCtrl)
{
	_ASSERT(pThreadCtrl);
	return ((pThreadCtrl->Kill()) ? TRUE : FALSE);	
}

BOOL CThreadManager::ApplyToTask(CTask* pPAsk, THREAD_MGR_MEMBER_FUNC MemberFunc)
{
	SCOPED_LOCK_SINGLE(&m_CS);
	
	for each( THREADMAP::value_type it in m_Threads )
	{
		CThreadControl* pThreadCtrl = it.second;
		if (pThreadCtrl->GetTask() == pPAsk)
		{
			if (!(this->*MemberFunc)(pThreadCtrl))
			{
				Put(_T("ApplyToTask Error : Task ->%x : ThreadControl -> %x"), pPAsk, pThreadCtrl);
			}			
		}
	}	

	return TRUE;
}


BOOL CThreadManager::SuspendTask(CTask* pPAsk)
{
	_ASSERT(pPAsk != NULL);
	return ApplyToTask(pPAsk, THREAD_MGR_MEMBER_FUNC(&CThreadManager::SuspendThread));
}

BOOL CThreadManager::ResumeTask(CTask* pPAsk)
{
	_ASSERT(pPAsk != NULL);
	return ApplyToTask(pPAsk, THREAD_MGR_MEMBER_FUNC(&CThreadManager::ResumeThread));
}

BOOL CThreadManager::KillTask(CTask* pPAsk)
{
	_ASSERT(pPAsk != NULL);
	return ApplyToTask(pPAsk, THREAD_MGR_MEMBER_FUNC(&CThreadManager::KillThread));
}
