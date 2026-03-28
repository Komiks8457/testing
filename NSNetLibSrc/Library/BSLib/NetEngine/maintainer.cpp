#include "StdAfx.h"
#include "maintainer.h"
#include "NetEngine.h"

BS_IMPLEMENT_DYNCREATE(CMaintainer, CServiceObject)

CMaintainer::CMaintainer()
{
	m_hEventExit = ::CreateEvent(NULL, FALSE, FALSE, NULL);
}

CMaintainer::~CMaintainer()
{
	if (m_hEventExit)
	{
		::CloseHandle(m_hEventExit);
		m_hEventExit = NULL;
	}
}

bool CMaintainer::Init(CTask* pTask, ULONG_PTR Param)
{ 
	CServiceObject::Init(pTask, Param); 
	
	MaintainerConfig* pConf = (MaintainerConfig*)Param;	
	
	m_Config = *pConf;

	if (pConf->target_flags & MT_MASK_STALE_SESSIONS)
		m_Jobs.RegisterCallback(this, pConf->target_entries[MAINTAIN_TARGET_STALE_SESSIONS].interval, &CMaintainer::RemoveStaleSessions);

	// 일단은 필요 없다... 화일시스템에서 자동으로 갱신한다... 
	//if (pConf->dwTargetFlags & MT_MASK_RARELY_USED_FILES)
	//	m_Jobs.RegisterCallback(this, pConf->TargetEntries[MAINTAIN_TARGET_RARELY_USED_FILES].dwInterval, RemoveInactiveFiles);

	if (pConf->target_flags & MT_MASK_KEEP_ALIVE_SESSION)
		m_Jobs.RegisterCallback(this, pConf->target_entries[MAINTAIN_TARGET_KEEP_ALIVE_SESSION].interval, &CMaintainer::ConnectOfflinedActiveSessions);

	return true;
}

// remove stale or idle sessions
int CMaintainer::DoWork(void* pArg, DWORD dwThreadID)
{
	BOOL* bAborted = (BOOL*)pArg;
	if ((*bAborted) != 0)
		return 0;

	::InterlockedIncrement(&m_nCurActiveThreadNum);

	ULONGLONG prev_tick = 0;
	ULONGLONG next_tick = 0;

	while (*bAborted == 0)
	{
		prev_tick = GetTickCount64();
		
		if (::WaitForSingleObject(m_hEventExit, m_Config.resolution) != WAIT_TIMEOUT)
			break;		

		next_tick = GetTickCount64();

		m_Jobs.Update(static_cast<DWORD>(next_tick - prev_tick));
	}
	
	::InterlockedDecrement(&m_nCurActiveThreadNum);

	return 0;
}

void CMaintainer::RemoveStaleSessions()
{
	const MaintainerEntry& entry = m_Config.target_entries[MAINTAIN_TARGET_STALE_SESSIONS];
	m_pOwner->engine()->RemoveStaleSessions(entry.interval, entry.timeout);
}

void CMaintainer::RemoveInactiveFiles()
{
//	CNetEngine::GetEngine()->RemoveInactiveFiles();
}

void CMaintainer::ConnectOfflinedActiveSessions()
{
	m_pOwner->engine()->ConnectOfflinedKeepAliveSessions();
}

BOOL CMaintainer::EndWork()
{
	SCOPED_LOCK_SINGLE(&m_CS);

	if (m_hEventExit != NULL)
		::SetEvent(m_hEventExit);

	return TRUE;
}