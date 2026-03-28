#pragma once

#include "swsession.h"

//////////////////////////////////////////////////////////
// Session Manager
//////////////////////////////////////////////////////////

typedef std::list<SKeepAlive*>	SESSIONS_TO_CONNECT;

typedef std::map<DWORD, CSession*>	mapSESSION;

class CSessionMgr
{
public:
	CSessionMgr() : m_CS(TRUE), m_SessionPool(DEFAULT_CHUNK_SIZE, NULL, 0, _T("NetEngine::SessionPool") )
	{
	}

	~CSessionMgr()
	{
		Cleanup();
	}

protected:
	CCriticalSectionBS			m_CS;

	CIDPool						m_IDPool;

	ChunkAllocatorST<CSession>	m_SessionPool;
	
	mapSESSION					m_ActiveSessions;

	std::list<DWORD>			free_active_sessions_;

public:
	void	Cleanup()
	{
		for each (auto session in m_ActiveSessions)
			(session.second)->Close(DISCONNECT_REASON_ENGINE_SHUTDOWN);

		m_ActiveSessions.clear();
	}

	BOOL	Create(int PoolChunkSize)
	{
		SCOPED_LOCK_SINGLE(&m_CS);

		if (m_IDPool.Create(PoolChunkSize) == FALSE)
			return FALSE;

		return TRUE;
	}

	long	CloseStaleSessions(DWORD interval, DWORD dwTimeout);
	void	CloseKeepAliveSessions(SESSIONS_TO_CONNECT& keep_alive_sessions);
	BOOL	FreeSession(CSockStream* pFreeSock, DWORD dwReason);
	DWORD	CreateNewSession(CSockStream* pNewSock, DWORD dwDefaultTask);
	bool	SetMsgSizeLimit(DWORD session_id, DWORD msg_size_limit);
	bool	SetSendQueDepth(DWORD session_id, DWORD send_que_depth);
	
	DWORD	GetSessionCount() const
	{ 
		return static_cast<DWORD>(m_ActiveSessions.size());
	}

	CSession*	GetSession(DWORD dwSession) 
	{ 
		SCOPED_LOCK_SINGLE(&m_CS);

		mapSESSION::iterator iter = m_ActiveSessions.find(dwSession);
		if (iter == m_ActiveSessions.end())
			return nullptr;

		return iter->second;
	}

	void	InvalidateAllSessions();
	void	GetUsage(size_t& Total, size_t& Used, size_t& Free);
	void	CloseAllPassiveSession();
};





















