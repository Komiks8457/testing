#pragma once
#include "SockSystem.h"

class CSockSystem;
class CTaskManager;
class CThreadManager;

class CSystem
{
public:
	CSystem();
	~CSystem();

	void Initialize();
	void Finalize();

protected:
	CSockSystem*	m_pSockSystem;
	CTaskManager*	m_pTaskManager;
	CThreadManager*	m_pThreadManager;	

public:
	CSockSystem*	GetSockSystem()	{ return m_pSockSystem; }
	CTaskManager*	GetTaskManager() { return m_pTaskManager; }
	CThreadManager*	GetThreadManager() { return m_pThreadManager; }
};
