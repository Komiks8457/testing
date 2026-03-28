#pragma once

////////////////////////////////////////////////////////////////////////////////
// TASK ID BUILDER
////////////////////////////////////////////////////////////////////////////////

// 혹시 DEFINE_TASKID_XXX() 를 사용하지 않고 임의로 ID 지정하는 경우를 detect하기 위해
// tasktype_mask 는 반드시 값을 가지도록 했다. 0 을 쓰지 않았다는 거지...
// 그래야 실수로 만약 DEFINE_TASKID_SYS <-- 요 매크로 안쓰고 
// #define MY_TASK_ID	1  <-- 요런 식으로 쓰면 우끼고 자빠진 ID라는 사실을 detect하지...

#define	TASKTYPE_MASK_SYSTEM	0x10000000
#define TASKTYPE_MASK_USER		0x01000000

inline DWORD __task_id_builder(DWORD mask, DWORD id)
{
	_ASSERT(mask == TASKTYPE_MASK_SYSTEM || mask == TASKTYPE_MASK_USER);
	_ASSERT((id & TASKTYPE_MASK_SYSTEM) == 0 && (id & TASKTYPE_MASK_USER) == 0);
	return (mask | id);
}

#define DEFINE_TASKID_SYS(simbol, id)	\
	const DWORD	simbol = __task_id_builder(TASKTYPE_MASK_SYSTEM, (id));

#define DEFINE_TASKID_USER(simbol, id)	\
	const DWORD	simbol = __task_id_builder(TASKTYPE_MASK_USER, (id));


#define IS_VALID_TASK_ID(id)	(((id) & TASKTYPE_MASK_SYSTEM) != 0 || ((id) & TASKTYPE_MASK_USER) != 0)

////////////////////////////////////////////////////////////////////////////////
// CTaskManager
////////////////////////////////////////////////////////////////////////////////
class CTaskManager
{
	typedef std::map<DWORD, CTask*>		TASKS;
public:

	CTaskManager(void) {}
	virtual ~CTaskManager(void)
	{
		KillAllTasks();
	}

protected:
	TASKS	m_Tasks;
	CCriticalSectionBS	m_CS;
	
public:
	BOOL	RegisterTask(DWORD dwTaskID, CTask* pTask);
	CTask*	GetTask(DWORD dwTaskID);
	BOOL	RemoveTask(DWORD dwTaskID);
	BOOL	SuspendTask(DWORD dwTaskID);
	BOOL	ResumeTask(DWORD dwTaskID);
	
	TASKS::iterator LockIterator(size_t& nSize)
	{
		m_CS.Lock();
		nSize = m_Tasks.size(); 
		return m_Tasks.begin();
	}
	
	void	UnlockIterator()
	{
		m_CS.Unlock();
	}	

	void	KillAllTasks()
	{
		for each( TASKS::value_type it in m_Tasks )
		{
			auto pTask = it.second;
			delete pTask;
		}

		m_Tasks.clear();
	}	

	void	DeactivateAllTasks()
	{
		for each( TASKS::value_type it in m_Tasks )
		{
			CTask* pTask = it.second;
			pTask->DeactivateMsgQue();
		}
	}
};

inline CTask* CTaskManager::GetTask(DWORD dwTaskID)
{
	IS_VALID_TASK_ID(dwTaskID);
	
	SCOPED_LOCK_SINGLE(&m_CS);
	
	TASKS::iterator it = m_Tasks.find(dwTaskID);
	if (it != m_Tasks.end())
		return (*it).second;

	return NULL;
}

inline BOOL CTaskManager::RegisterTask(DWORD dwTaskID, CTask* pTask)
{
	IS_VALID_TASK_ID(dwTaskID);
	
	SCOPED_LOCK_SINGLE(&m_CS);
	
	TASKS::iterator it = m_Tasks.find(dwTaskID);
	if (it != m_Tasks.end())
		return FALSE;

	m_Tasks[dwTaskID] = pTask;

	return TRUE;
}


inline BOOL CTaskManager::RemoveTask(DWORD dwTaskID)
{
	IS_VALID_TASK_ID(dwTaskID);

	SCOPED_LOCK_SINGLE(&m_CS);

	TASKS::iterator it = m_Tasks.find(dwTaskID);
	if (it != m_Tasks.end())
	{
		delete (*it).second;
		m_Tasks.erase(it);
		return TRUE;
	}

	return FALSE;
}

inline BOOL CTaskManager::SuspendTask(DWORD dwTaskID)
{
	IS_VALID_TASK_ID(dwTaskID);

	SCOPED_LOCK_SINGLE(&m_CS);

	TASKS::iterator it = m_Tasks.find(dwTaskID);
	if (it != m_Tasks.end())
	{
		CTask* pTask = (*it).second;
		return pTask->Suspend();
	}
	return FALSE;
}

inline BOOL CTaskManager::ResumeTask(DWORD dwTaskID)
{
	IS_VALID_TASK_ID(dwTaskID);

	SCOPED_LOCK_SINGLE(&m_CS);

	TASKS::iterator it = m_Tasks.find(dwTaskID);
	if (it != m_Tasks.end())
	{
		CTask* pTask = (*it).second;
		return pTask->Resume();
	}
	return FALSE;
}
