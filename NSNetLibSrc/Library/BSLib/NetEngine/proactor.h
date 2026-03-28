#pragma once

#include "iocontext.h"
#include "SocketTCP.h"

class CProactorIOCP : public CServiceObjectSock
{
	BS_DECLARE_DYNCREATE(CProactorIOCP)
protected:
	CProactorIOCP(long nConcurrentThreadNum = 0);
	
public:
	virtual ~CProactorIOCP(void);
	
protected:
	HANDLE		m_hIOCP;
	long		m_nConcurrentThreadNum;	

public:
	virtual void	Close();
	virtual int		DoWork(void* pArg, DWORD dwThreadID) override;
	virtual BOOL	EndWork() override;
	
	virtual BOOL	RegisterHandle(HANDLE hObject, ULONG_PTR CompletionKey) override;
	virtual BOOL	CircleOfTheLife(CEventHandler* pHandler, DWORD dwData = 0) override;

	BOOL	IsEventLoopDone();
	BOOL	PostCompletion(ULONG_PTR CompletionKey, CIOContext* pContext);
	BOOL	PostWakeupCompletionsToExit(int nHowMany);
	long	GetActiveThreadNum(void);
	long	GetConcurrentThreadNum() const { return m_nConcurrentThreadNum; }
};

