#pragma once

#include "iocontext.h"
#include "socketudp.h"

typedef std::list<HANDLE>		HANDLES;
#define EVENT_TIMEOUT			WSA_INFINITE

class CPeerProcessOverlapped : public CServiceObjectSock
{
	BS_DECLARE_DYNCREATE(CPeerProcessOverlapped)
public:
	CPeerProcessOverlapped(void);
	~CPeerProcessOverlapped(void);

public:
	IQue<CMsg*>*		m_pSendingQueue;
		
	WSAEVENT			m_Events[WSA_MAXIMUM_WAIT_EVENTS];
	ULONG_PTR			m_Contexts[WSA_MAXIMUM_WAIT_EVENTS];
	DWORD				m_dwEventCountToWait;
	
public:
	virtual bool	Init(CTask* pTask, ULONG_PTR Param) override;
	virtual int		DoWork(void* pArg, DWORD dwThreadID) override;
	virtual BOOL	EndWork() override;

	virtual BOOL	RegisterHandle(HANDLE hObject, ULONG_PTR Data) override;
	virtual BOOL	CircleOfTheLife(CEventHandler* pHandler, DWORD dwData = 0) override;

	virtual IQue<CMsg*>* GetSendingQueue(){	return m_pSendingQueue; }

protected:
	void			RefreshEventCountToWait();
	CIOContext*		GetCompletedOperationSrc(DWORD dwIndex);
};


class CPeerProcessIOCP : public CServiceObjectSock
{
	BS_DECLARE_DYNCREATE(CPeerProcessIOCP)
public:
	CPeerProcessIOCP(void);
	~CPeerProcessIOCP(void);

public:
	HANDLE				m_hIOCP;
	long				m_nCurActiveThreadNum;

public:
	virtual bool	Init(CTask* pTask, ULONG_PTR Param) override;
	virtual void	Close();
	virtual int		DoWork(void* pArg, DWORD dwThreadID) override;
	virtual BOOL	EndWork() override;

	virtual BOOL	RegisterHandle(HANDLE hObject, ULONG_PTR CompletionKey) override;
	virtual BOOL	CircleOfTheLife(CEventHandler* pHandler, DWORD dwData = 0) override;

public:
	BOOL	PostCompletion(ULONG_PTR CompletionKey, CIOContext* pContext);
	BOOL	PostWakeupCompletionsToExit(int nHowMany);
	long	GetActiveThreadNum(void);
};

class CPeerNet;

class CPeerProcessBlocking : public CServiceObjectSock
{
	BS_DECLARE_DYNCREATE(CPeerProcessBlocking)
public:
	CPeerProcessBlocking(void);
	virtual ~CPeerProcessBlocking(void);

	void set_peer_net(CPeerNet* peer_net) { peer_net_ = peer_net; }

private:
	long			m_nCurActiveThreadNum;

	bool			m_bActivateThread;

	CPeerNet*		peer_net_;

public:
	virtual bool	Init(CTask* pTask, ULONG_PTR Param) override;
	virtual void	Close();
	virtual int		DoWork(void* pArg, DWORD dwThreadID) override;
	virtual BOOL	EndWork() override;

	virtual BOOL	RegisterHandle(HANDLE hObject, ULONG_PTR CompletionKey) override;
	virtual BOOL	CircleOfTheLife(CEventHandler* pHandler, DWORD dwData = 0) override;
};
