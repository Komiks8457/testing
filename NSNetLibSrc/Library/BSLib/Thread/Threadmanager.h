#pragma once

class CThreadControl;
class CThreadManager;
struct THREADCONF;	
struct ProcessorAffinity;

typedef int (CThreadManager::*THREAD_MGR_MEMBER_FUNC)(CThreadControl*);
typedef BOOL (PASCAL FAR* LPFN_SETTHREADIDEAL_PROCESSOR)(HANDLE hThread, DWORD dwIdealProcessor);

class CThreadManager
{
	typedef std::map<DWORD,CThreadControl*>	THREADMAP;

public:
	CThreadManager();
	virtual ~CThreadManager();

protected:
	DWORD				m_dwIDToAlloc;		
	THREADMAP			m_Threads;	
	CCriticalSectionBS	m_CS;

	LPFN_SETTHREADIDEAL_PROCESSOR	_SetThreadIdealProcessor;

protected:
	void	CleanupThreads(THREADMAP* pThreads);
	BOOL	SuspendThread(CThreadControl* pThreadCtrl);
	BOOL	ResumeThread(CThreadControl* pThreadCtrl);
	BOOL	KillThread(CThreadControl* pThreadCtrl);

	DWORD	AllocID();
	
	BOOL	ApplyToTask(CTask* pTask, THREAD_MGR_MEMBER_FUNC MemberProc);

	BOOL	IsNT();

public:
	void	Close();
	BOOL	SpawnThreads(long n, DWORD ThreadIDs[], THREADCONF* pThreadConf, DWORD dwFlags, ProcessorAffinity* pPA);
	CThreadControl*	AppendThread(DWORD* pdwThreadID, THREADCONF* pThreadConf, DWORD dwFlags);

	BOOL	SuspendTask(CTask* pTask);
	BOOL	ResumeTask(CTask* pTask);
	BOOL	KillTask(CTask* pTask);
};
