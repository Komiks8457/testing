#pragma once

#include "Interface.h"

class CMaintainer : public CServiceObject
{
	BS_DECLARE_DYNCREATE(CMaintainer)

public:
	CMaintainer();
	~CMaintainer();

protected:
	HANDLE				m_hEventExit;
	MaintainerConfig	m_Config;
	CScheduledCallbacker<CMaintainer, DWORD> m_Jobs;

protected:
	void	RemoveStaleSessions();
	void	RemoveInactiveFiles();
	void	ConnectOfflinedActiveSessions();

public:
	virtual bool	Init(CTask* pTask, ULONG_PTR Param) override;
	virtual int		DoWork(void* pArg, DWORD dwThreadID) override;
	virtual BOOL	EndWork() override;

	BOOL	CircleOfTheLife(CEventHandler* pHandler, DWORD dwData = 0) { return TRUE; }
};
