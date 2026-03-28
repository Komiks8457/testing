#pragma once

//////////////////////////////////////////////////////////////////////////
// 상속받아서 쓰면 되겠다....
// ThreadProc 는 DWORD __stdcall  ThreadProc(LPVOID pParam) 를 오버라이딩 시켜서 쓰면된다...
// 생성시 Suspend상태로 생성 Start() 해줘야 Resume상태가 된다...
//////////////////////////////////////////////////////////////////////////
#define TC_TIMEOUT_QUIT			3000

class CSimpleThread
{
public:
	CSimpleThread() : m_hThread(NULL), m_bSuspended(TRUE), m_dwThreadID(0) {}  
	virtual ~CSimpleThread(){Stop();}  

public:
	BOOL 	Stop();  

	BOOL	Create(LPVOID lpParameter, DWORD dwStackSize, int nPriority);
	BOOL	Resume();  
	BOOL	Start(){return Resume();}  
	BOOL	Suspend();  

	void	SetName(const char* name);

	int		GetPriority(){return GetThreadPriority(m_hThread);}  
	BOOL	SetPriority(int iPriority){ return (TRUE == SetThreadPriority(m_hThread, iPriority));}  
	
	BOOL	IsCreated(){ return (m_hThread != NULL);}  
	BOOL	IsSuspended(){return m_bSuspended;}
	DWORD	GetThreadID(){return m_dwThreadID;}
	HANDLE	GetThreadHandle() { return m_hThread; }
	
	operator	HANDLE() const;

protected:
//	virtual unsigned int __stdcall  ThreadProc(LPVOID pParam){ return 0; };
	virtual void __stdcall  ThreadProc(LPVOID pParam) {}

	static unsigned __stdcall ThreadEntry(void* param)
	{
		((CSimpleThread*)param)->ThreadProc(param);
		return 0;
	}

protected:	
	HANDLE	m_hThread;  
	BOOL	m_bSuspended;
	DWORD	m_dwThreadID;
};
