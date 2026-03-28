#include "stdafx.h"
#include <process.h>
#include "simplethread.h"

extern bool g_bTerminateProcess;

CSimpleThread::operator HANDLE() const
{ 
	return (this == NULL) ? NULL : m_hThread; 
}

BOOL CSimpleThread::Stop()
{
	if (m_hThread != NULL)
	{
//		if (::WaitForSingleObject(m_hThread, TC_TIMEOUT_QUIT) == WAIT_TIMEOUT)
//		{
//			DWORD dwThreadExitCode = -3;
		//	if (::GetExitCodeThread(m_hThread, &dwThreadExitCode))
//			{
		//		if (dwThreadExitCode == STILL_ACTIVE)
		//			::TerminateThread(m_hThread, 1);
		//		else
//					::TerminateThread(m_hThread, dwThreadExitCode);
//			}		
//			
//			Put(_T("Worker Thread 강제 종료 - ID: %d "), m_dwThreadID);
//		}
//		else
//			Put(_T("Worker Thread 정상 종료 - ID: %d"), m_dwThreadID);
		if( !g_bTerminateProcess)
			::WaitForSingleObject(m_hThread, INFINITE) ;
		else
			::WaitForSingleObject(m_hThread, 100);
		
		CloseHandle(m_hThread);
		m_hThread	 = NULL;
		m_bSuspended = TRUE;

		return TRUE;
	}

	return FALSE;
}

#ifdef _MT
typedef UINT (WINAPI *PTHREAD_ROUTINE)(LPVOID lpThreadParameter);
typedef PTHREAD_ROUTINE		LPTHREAD_ROUTINE;
#endif 

BOOL CSimpleThread::Create(LPVOID lpParameter, DWORD dwStackSize, int nPriority)
{	
	if (NULL == m_hThread)
	{
		_ASSERT(lpParameter == this);
		m_hThread = (HANDLE)_beginthreadex((LPSECURITY_ATTRIBUTES)NULL, 
											  (DWORD)dwStackSize, 
											  &CSimpleThread::ThreadEntry, 
											  lpParameter, 
											  (DWORD)CREATE_SUSPENDED, 
											  (UINT*)&m_dwThreadID);
		if (m_hThread != NULL)
		{
			SetThreadPriority(m_hThread, nPriority);
			return TRUE;
		}
		else
			return FALSE;
	}
	return TRUE; 
}

BOOL CSimpleThread::Resume()
{
	DWORD dwResumeCount = 0;
	if (m_hThread != NULL && m_bSuspended == TRUE)
	{
		dwResumeCount = ::ResumeThread(m_hThread);
		if (dwResumeCount == 0xFFFFFFFF)	// failed to resume thread
			return FALSE;
		else if (dwResumeCount > 1)
		{
			for (DWORD i = 0; i < dwResumeCount - 1; i++)
				::ResumeThread(m_hThread);
		}
		
		m_bSuspended = FALSE;		
	}
	return TRUE;
}

BOOL CSimpleThread::Suspend()
{	
	DWORD dwPrevSuspendCount = 0;
	if (m_hThread != NULL && m_bSuspended == FALSE)
	{	
		dwPrevSuspendCount = ::SuspendThread(m_hThread);
		if (dwPrevSuspendCount == 0xFFFFFFFF)
		{
			if (dwPrevSuspendCount == MAXIMUM_SUSPEND_COUNT)
				return TRUE;
			else
			{
				m_bSuspended = FALSE;
				return FALSE;
			}
		}
		else if (dwPrevSuspendCount > 0)
		{
			for (DWORD i = 0; i < dwPrevSuspendCount; i++)
				::ResumeThread(m_hThread);
		}
		
		m_bSuspended = TRUE;		
	}
	return TRUE;
}

void CSimpleThread::SetName(const char* name)
{
	const DWORD MS_VC_EXCEPTION = 0x406D1388;
#pragma pack(push,8)
	typedef struct tagTHREADNAME_INFO
	{
		DWORD dwType; // Must be 0x1000.
		LPCSTR szName; // Pointer to name (in user addr space).
		DWORD dwThreadID; // Thread ID (-1=caller thread).
		DWORD dwFlags; // Reserved for future use, must be zero.
	} THREADNAME_INFO;
#pragma pack(pop)

	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = name;
	info.dwThreadID = m_dwThreadID;
	info.dwFlags = 0;

	__try { RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), reinterpret_cast<ULONG_PTR*>(&info)); } __except (EXCEPTION_CONTINUE_EXECUTION) {}
}
