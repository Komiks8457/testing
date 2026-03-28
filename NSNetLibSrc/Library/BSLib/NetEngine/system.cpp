#include "StdAfx.h"
#include "system.h"
#include "../thread/bsthread.h"

CSystem::CSystem() : m_pSockSystem(nullptr), m_pTaskManager(nullptr), m_pThreadManager(nullptr) {}

CSystem::~CSystem()
{
	Finalize();
}

void CSystem::Initialize()
{
	_ASSERT( m_pSockSystem == nullptr && m_pTaskManager == nullptr && m_pThreadManager == nullptr );

	m_pSockSystem	= new CSockSystem;
	m_pTaskManager	= new CTaskManager;
	m_pThreadManager= new CThreadManager;
}

void CSystem::Finalize()
{
	if (m_pSockSystem)
	{
		// Maintainer에서 지속적으로 시도하는 Permanent Session Connect 가 Blocking Job 이기 땜에
		// Connection Fail 날때까지... thread가 정지 되어 있거덩... 그래서, 서버가 바로 종료되지 않거덩?
		// 이런 전차로, connect 즉시 실패 나도록 sock system 자체를 cleanup 시켜서 connect가 바로 
		// 실패하며 return 되도록 한거다... 
		m_pSockSystem->ReleaseWinsock();
	}

	if( m_pTaskManager)
		m_pTaskManager->DeactivateAllTasks();

	if (m_pThreadManager)
	{
		delete m_pThreadManager;
		m_pThreadManager = NULL;
	}

	if (m_pTaskManager)
	{
		// Task객체 delete시에 new한 DLL과 delete하는 DLL이 달라서 에러남
		// BSLib가 static lib로 바뀌면 자연히 해결될것 같음
		delete m_pTaskManager;
		m_pTaskManager = NULL;
	}
	
	if (m_pSockSystem)
	{
		delete m_pSockSystem;
		m_pSockSystem = NULL;
	}
}