#pragma once

#include "sockettcp.h"

class CMsg;
class CSession;
class CSockDatagram : public CBaseSocket
{
public:
	CSockDatagram(void);
	~CSockDatagram(void);

protected:
	CTask*			m_pTask;

protected:
	CIOContext		m_ctxRead;
	
protected:
	//PEERNET은 하나의 datagram 소켓을 사용하고
	//recvfrom 은 반드시 post-> completion -> post 형태로 이루어지기 때문에
	//소켓 당 하나의 addr이 필요하지만
	//send는 post send -> post send -> n... -> completion 형태로 이루어지기 때문에
	//Msg별로 SOCKADDR을 들고 있게 된다! (CMsg에 SOCKADDR을 가지고 있다!!)
	unsigned short	m_uCreatedPort;

	//recv전용 SOCKADDR..
	int				m_lFromSize;

public:
	////////////////////////////////////////// completion handlers
	long			(CSockDatagram::*FP_UDP_EVENTHANDLER[IOTYPE_NUM][IO_TARGET_NUM])(IIOContext* pContext, DWORD dwTransferred);
	long			(CSockDatagram::*FP_UDP_RECVED_MSG_HANDLER)(CMsg* pRecvedMsg);
	long			(CSockDatagram::*FP_UDP_INVOKER_SENDDATA)( SOCKADDR_IN* pAddr, CMsg* pMsg );

protected:
	void			PrepareSocket();

public:
	long			PostRecv();
	BOOL			ReallocSocket();

protected:
	long			PostSend( SOCKADDR_IN* pAddr, IIOContext* pIOContext );

	// send
	long			_SendMsgDirect_Overlapped( SOCKADDR_IN* pAddr, CMsg* pMsg );
	
	// recv
	long			_MsgReceived( CMsg* pRecvedMsg );

	long			_ForgeMsgToSend( CMsg*& pMsg );

public:
	void			Init();
	BOOL			Create( unsigned short uPort );
	void			SetCurTask(CTask* pTask){ _ASSERT(pTask != NULL); SCOPED_LOCK_SINGLE(&m_CS); m_pTask = pTask; }

	BOOL			LetsRock(CServiceObject* pServiceObject) { return RegisterHandleTo(pServiceObject, (HANDLE)GetHandle()); }
	long			SendMsg( SOCKADDR_IN* pAddr, CMsg* pMsg );
	
	virtual long	Close();
	virtual long	HandleEvent(DWORD dwTransferred, void* pCompleted);
	virtual BOOL	RegisterHandleTo(CServiceObject* pServiceObj, HANDLE hHandle);

	virtual CIOContext*	GetContext(long nOP){ return NULL; }
	virtual CIOContext*	GetMsgToRecv(){ return &m_ctxRead; }

	BOOL			GetSockAddr( SOCKADDR_IN& addr );
};
