#pragma once

#include "iocontext.h"
#include "SocketTCP.h"

class CAcceptorBSD : public CServiceObjectSock
{
	BS_DECLARE_DYNCREATE(CAcceptorBSD)
protected:
	CAcceptorBSD();
	
public:
	virtual ~CAcceptorBSD(void);

protected:
	SOCKET			m_hListen;
	CEventHandler*	m_pEventHandler;

public:
	virtual int		DoWork(void* pArg, DWORD dwThreadID) override;
	virtual BOOL	EndWork() override;

	virtual BOOL	RegisterHandle(HANDLE hObject, ULONG_PTR CompletionKey) override;
	virtual BOOL	CircleOfTheLife(CEventHandler* pHandler, DWORD dwData) override { return TRUE; }
};


///////////////////////////////////////////////////////
// CReactorAccept
///////////////////////////////////////////////////////
#define ACCEPTOR_EVENT_NUM				2
class CAcceptorEvent : public CServiceObjectSock
{
	BS_DECLARE_DYNCREATE(CAcceptorEvent)

protected:
	CAcceptorEvent();
	
public:
	virtual ~CAcceptorEvent(void);

protected:	
	WSAEVENT		m_Events[ACCEPTOR_EVENT_NUM];	
	SOCKET			m_hObject;	
	CEventHandler*	m_pEventHandler;	

	void			SetSockHandle(SOCKET hSock) {m_hObject = hSock;}
	SOCKET			GetSockHandle(){return m_hObject;}
	
public:
	virtual int		DoWork(void* pArg, DWORD dwThreadID) override;
	virtual BOOL	EndWork() override;

	virtual BOOL	RegisterHandle(HANDLE hObject, ULONG_PTR Data) override;
	virtual BOOL	CircleOfTheLife(CEventHandler* pHandler, DWORD dwData = 0) override;
};