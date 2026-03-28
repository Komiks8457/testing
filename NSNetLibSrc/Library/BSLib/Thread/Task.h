#pragma once

#include "../util/que.h"

class CMsg;
class CNetEngine;
struct ProcessorAffinity;

class CTask
{
typedef std::list<DWORD>	DWLIST;
public:
	CTask();
	virtual ~CTask();

protected:	
	CQueMT<CMsg*>		m_MsgQue;
		
	DWLIST				m_ThreadIDs;
	CThreadManager*		m_pThreadMgr;
	THREADCONF			m_TC;
	CCriticalSectionBS	m_CS;
	CNetEngine*			engine_;
	
	DWORD				m_nThreadCount;
public:
	BOOL	Create(CNetEngine* engine, CThreadManager* pThreadMgr, CRuntime* pRT_ServiceObj, ULONG_PTR Data);
	BOOL	WaitForExitThreads();
	BOOL	Suspend();
	BOOL	Resume();
	size_t	GetQueuedMsgCount() { return m_MsgQue.GetSize(); }
//	BOOL	StartJob(long ThreadNumToSpawn, long nPriority = THREAD_PRIORITY_NORMAL, DWORD dwStackSize = 0, DWORD dwFlags = 0, ProcessorAffinity* pPA = NULL);
	BOOL	StartJob(long ThreadNumToSpawn, long nPriority = THREAD_PRIORITY_NORMAL, DWORD dwStackSize = (1024 * 1024 * 2), DWORD dwFlags = 0, ProcessorAffinity* pPA = NULL);
	BOOL	AppendThread();
	BOOL	EnqueMsg(CMsg* pMsg) { return m_MsgQue.Enque(pMsg); }
	BOOL	DequeMsg(CQueMT<CMsg*>::Items& msgs, DWORD dwTimeout);
	BOOL	DequeMsg(CMsg** ppMsg, DWORD dwTimeout,BOOL bForCleanup = FALSE) { return m_MsgQue.Deque(ppMsg, dwTimeout, bForCleanup); }
	BOOL	IsQueueEmpty(){ return m_MsgQue.IsEmpty(); }
	BOOL	DeactivateMsgQue() { return m_MsgQue.Deactivate(); }
	CServiceObject*	GetServiceObject() const { return m_TC.pServiceObject; }
	DWORD	GetThreadCount(){ return m_nThreadCount;}
	CNetEngine* engine() const { return engine_; }
};
