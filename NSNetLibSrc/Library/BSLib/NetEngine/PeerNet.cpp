#include "stdafx.h"
#include "PeerNet.h"

#include "NetEngine.h"
#include "PeerProcess.h"

CPeerNet::CPeerNet(void) : engine_(nullptr)
{
	m_pSocket			= NULL;
	m_dwMainTask		= 0;
}

CPeerNet::~CPeerNet(void)
{
	//Finalize();
}

BOOL CPeerNet::StartPeerTask( CNetEngine* engine, DWORD dwTaskID )
{
	m_dwMainTask = dwTaskID;

	CRuntime* pPeerServiceObjIO = BS_RUNTIME_CLASS(CPeerProcessBlocking);

	if( engine->RegisterTask(TASK_SYS_PEER_NETIO, NULL, pPeerServiceObjIO,0) == FALSE )
		return FALSE;

	engine_ = engine;

	return TRUE;
}

BOOL CPeerNet::StartPeerNetService( WORD port )
{
	//IOTASK에 작업 결과를 enque를 하도록 하자!
	CTask* pIOTask = engine_->system().GetTaskManager()->GetTask( m_dwMainTask );	
	if( pIOTask == NULL )
	{
		_ASSERT( pIOTask != NULL );
		return FALSE;
	}

	m_pSocket = engine_->AllocDGramNewSock();
	if( m_pSocket->Create(port) == FALSE )
	{
		_ASSERT( FALSE );
		return FALSE;
	}

	m_pSocket->SetCurTask( pIOTask );

	//실제 operator는 PEER_NETIO task이다!!
	pIOTask = engine_->system().GetTaskManager()->GetTask( TASK_SYS_PEER_NETIO );
	_ASSERT(pIOTask != NULL);

	CServiceObject* pIOService = pIOTask->GetServiceObject();
	if( m_pSocket->LetsRock(pIOService) == FALSE )
		return FALSE;

	if( engine_->ActivateService(TASK_SYS_PEER_NETIO, 1,THREAD_PRIORITY_NORMAL, 0, 0, NULL) == FALSE )
		return FALSE;

	return TRUE;
}

void CPeerNet::Finalize()
{
// 	if( m_pSocket != NULL )
// 	{
// 		if( CNetEngine::GetEngine() != NULL )
// 			CNetEngine::GetEngine()->FreeDGramSocket( m_pSocket );
// 
// 		m_pSocket = NULL;
// 	}
}

/*
long CPeerNet::SendPeerMsg( SOCKADDR_IN& addr, CMsg* pMsg )
{
	long rcount_prev = pMsg->GetRefCnt();

	long result = m_pSocket->SendMsg( &addr, pMsg );
	if( result != NERR_GOOD )
	{
		if( pMsg->GetRefCnt() > rcount_prev )
			CNetEngine::GetEngine()->DelMsg( pMsg );

		return result;
	}

	return NERR_GOOD;
}


BOOL CPeerNet::GetPeerNetSockAddr( SOCKADDR_IN& addr )
{
	if( m_pSocket == NULL )
		return FALSE;

	return m_pSocket->GetSockAddr( addr );
}
*/