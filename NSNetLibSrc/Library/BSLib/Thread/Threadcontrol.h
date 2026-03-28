#ifndef __BSTHREAD_THREADCONTROL_H__
#define __BSTHREAD_THREADCONTROL_H__

class CThreadManager;
class CTask;

/////////////////////////////////////////////////////////////////////////
// Processor Affinity
/////////////////////////////////////////////////////////////////////////
struct ProcessorAffinity
{
	ProcessorAffinity()
	{
		policy  = PA_POLICY_ROUNDROBIN;
		begin   = -1;
		end		= -1;
	}

	enum PA_POLICY
	{
		PA_POLICY_ROUNDROBIN = 0,
		PA_POLICY_RANDOM,
	};

	PA_POLICY policy;

	int begin;
	int end;

	int SelectProcessor(int prev_processor)
	{
		int selected = -1;

		int assigned_cpu_count = (end - begin) + 1;

		if (policy == PA_POLICY_ROUNDROBIN)
		{
			if (prev_processor < 0)
				prev_processor = 0;
			else if (prev_processor >= MAXIMUM_PROCESSORS)
				prev_processor = MAXIMUM_PROCESSORS - 1;

			selected = prev_processor + 1;
			
			selected %= assigned_cpu_count;
			selected += begin;

			selected = prev_processor;
		}
		else
		{
			int rnd = rand() % assigned_cpu_count;

			selected = begin + rnd;
		}

		return selected;
	}
};

/////////////////////////////////////////////////////////////////////////
// Thread Conf
/////////////////////////////////////////////////////////////////////////
#define TC_TIMEOUT_QUIT			3000
#define INVALID_BSNET_THREADID	0

struct THREADCONF
{
	THREADCONF() { Initialize(); }

	void Initialize()
	{
		pTask			= NULL;
		pServiceObject	= NULL;
		nPriority		= 0;
		nGroupID		= 0;
		dwStackSize		= 0;
	}

	void THREADCONF::Clear()
	{
		::ZeroMemory(this, sizeof(THREADCONF));
		nGroupID = -1;
	}

	CTask*			pTask;
	CServiceObject* pServiceObject;
	long			nPriority;
	long			nGroupID;	
	DWORD			dwStackSize;	

	THREADCONF operator=(THREADCONF arg)
	{
		pTask			= arg.pTask;
		pServiceObject	= arg.pServiceObject;
		nPriority		= arg.nPriority;
		nGroupID		= arg.nGroupID;
		dwStackSize		= arg.dwStackSize;

		return *this;
	}
};

class CThreadControl : public CSimpleThread
{
public:
	CThreadControl();
	virtual ~CThreadControl();

protected:
	THREADCONF	m_ThreadConf;
	
	DWORD		m_dwBSNetThreadID;			// BSNet이 할당하는 ThreadID
	
	BOOL		m_bJobTerminated;	
	UINT		m_uiErrCode;
	
	CThreadManager*		m_pThreadMgr;
	CCriticalSectionBS	m_CS;

protected:	
	DWORD	GetBSNetThreadID() { return m_dwBSNetThreadID; }	
	BOOL	IsJobTerminated() { return m_bJobTerminated; }
	BOOL	Kill();
	CTask*	GetTask() { return m_ThreadConf.pTask; }

public:
	BOOL	Create(CThreadManager* pThreadMgr, THREADCONF* pThreadConf, DWORD dwBSNetThreadID);
	
/*	
	virtual UINT __stdcall ThreadProc(LPVOID pThreadCtrl)
	{		
		if (pThreadCtrl == NULL)
		{
			Put("CThreadControl == NULL");
			return -1;
		}

		CServiceObject* pJob	= NULL;
		m_uiErrCode				= -1;

		pJob = m_ThreadConf.pServiceObject;
		if (pJob == NULL)
			return m_uiErrCode;		
		
//		while (!m_bKillThread)
//		{
		m_uiErrCode = pJob->DoWork(&m_bJobTerminated, GetBSNetThreadID() - 1);
//		}
		
		return m_uiErrCode;
	}
*/
	virtual void __stdcall ThreadProc(LPVOID pThreadCtrl);

	friend class	CThreadManager;
};

#endif __BSTHREAD_THREADCONTROL_H__