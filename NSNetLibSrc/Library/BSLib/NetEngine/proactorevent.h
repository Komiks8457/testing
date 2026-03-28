#pragma once 

#include "IOContext.h"
#include "SocketTCP.h"

typedef std::list<HANDLE>		HANDLES;

#define EVENT_TIMEOUT			WSA_INFINITE

class CProactorEvent : public CServiceObjectSock
{
	BS_DECLARE_DYNCREATE(CProactorEvent)

protected:
	CProactorEvent();
	
public:
	virtual ~CProactorEvent(void);

protected:	
	ULONG_PTR		m_Contexts[WSA_MAXIMUM_WAIT_EVENTS];
	WSAEVENT		m_Events[WSA_MAXIMUM_WAIT_EVENTS];

	DWORD			m_dwEventCountToWait;
	HANDLES			m_DummyEvents;

protected:
	HANDLE			PopDummyEvent();
	void			PushDummyEvent(HANDLE handle);
	int				FindEmptySlot();
	BOOL			EraseSlot(int nIndex);
	void			RefreshEventCountToWait();
	
public:
	virtual int		DoWork(void* pArg, DWORD dwThreadID) override;
	virtual BOOL	EndWork() override;
	
	virtual BOOL	RegisterHandle(HANDLE hObject, ULONG_PTR Data) override;
	virtual BOOL	CircleOfTheLife(CEventHandler* pHandler, DWORD dwEventSlot) override;

	CIOContext*		GetCompletedOperationSrc(DWORD dwIndex);
	BOOL			IsEventLoopDone();	
};

inline HANDLE CProactorEvent::PopDummyEvent()
{
	HANDLE handle = m_DummyEvents.front();
	_ASSERT(handle != NULL);
	m_DummyEvents.pop_front();
	return handle;
}

inline void CProactorEvent::PushDummyEvent(HANDLE handle)
{
	if (handle == NULL)
		return;

	m_DummyEvents.push_back(handle);
}

inline int CProactorEvent::FindEmptySlot()
{
	for (int i = 2; i < WSA_MAXIMUM_WAIT_EVENTS; i += 2)
	{
		if (m_Contexts[i] == NULL)
			return i;
	}

	return -1;
}

