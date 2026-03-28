#include "StdAfx.h"
#include "SessionMgr.h"
#include "NetEngine.h"

//////////////////////////////////////////////////////////////////////////
//  dwDefaultTask - 디폴트 유저 테스크 아이디...
//////////////////////////////////////////////////////////////////////////
void CSessionMgr::GetUsage(size_t& Total, size_t& Used, size_t& Free)
{
	SCOPED_LOCK_SINGLE(&m_CS);

	m_SessionPool.GetUsage(Total, Used, Free);
}

void CSessionMgr::InvalidateAllSessions()
{
	SCOPED_LOCK_SINGLE(&m_CS);

	for each (auto iter in m_ActiveSessions)
		(iter.second)->Invalidate();
}

DWORD CSessionMgr::CreateNewSession(CSockStream* pNewSock, DWORD dwDefaultTask)
{
	_ASSERT(IS_VALID_TASK_ID(dwDefaultTask));
	
	if (pNewSock == NULL)
	{
		return NULL;
	}

	SCOPED_LOCK_SINGLE(&m_CS);

	CSession* pSession = NULL;
	DWORD dwNewSession = NULL;
	
	pSession = m_SessionPool.NewItem();
	if (pSession == NULL)
	{
		PutLog(LOG_WARNING, _T("CreateNewSession() - New Session 할당 실패!"));
		_ASSERT(FALSE);
		return NULL;
	}

	dwNewSession = m_IDPool.AllocID();
	if (dwNewSession == NULL) 
	{
		PutLog(LOG_WARNING, _T("CreateNewSession() - 세션 아이디 할당 실패!"));
		m_SessionPool.FreeItem(pSession);
		_ASSERT(FALSE);
		return NULL;
	}

	_ASSERT(pSession->GetSocket() == NULL);
	
	if (pSession->Create(pNewSock, dwNewSession, dwDefaultTask) == false)
	{
		pSession->Reset();
		m_SessionPool.FreeItem(pSession);
		m_IDPool.FreeID(dwNewSession);
		return NULL;
	}

	m_ActiveSessions.insert(mapSESSION::value_type(dwNewSession, pSession));
	return dwNewSession;
}

bool CSessionMgr::SetMsgSizeLimit(DWORD session_id, DWORD msg_size_limit)
{
	SCOPED_LOCK_SINGLE(&m_CS);

	mapSESSION::iterator iter = m_ActiveSessions.find(session_id);
	if( iter == m_ActiveSessions.end() )
		return false;

	CSockStream* socket = iter->second->m_pSocket;
	if (socket == nullptr)
		return false;

	socket->set_msg_size_limit(msg_size_limit);
	return true;
}

bool CSessionMgr::SetSendQueDepth(DWORD session_id, DWORD send_que_depth)
{
	SCOPED_LOCK_SINGLE(&m_CS);

	mapSESSION::iterator iter = m_ActiveSessions.find(session_id);
	if( iter == m_ActiveSessions.end() )
		return false;

	CSockStream* socket = iter->second->m_pSocket;
	if (socket == nullptr)
		return false;

	socket->set_send_queue_depth(send_que_depth);
	return true;
}

BOOL CSessionMgr::FreeSession(CSockStream* pFreeSock, DWORD dwReason)
{
	if (pFreeSock == NULL)
	{
		return TRUE;
	}
	
	SCOPED_LOCK_SINGLE(&m_CS);
	
	DWORD dwSession = pFreeSock->GetSessionID();
	std::map<DWORD, CSession*>::iterator iter = m_ActiveSessions.find(dwSession);
	if(dwSession == NULL || iter == m_ActiveSessions.end() )
	{
		CNetEngine* engine = pFreeSock->engine();
		if (pFreeSock->GetSessionType() == SESSION_TYPE_PASSIVE)
			engine->FreePassiveSocket(pFreeSock);
		else if (pFreeSock->GetSessionType() == SESSION_TYPE_ACTIVE)
			engine->FreeActiveSocket(pFreeSock);

		return TRUE;
	}

	CSession* pSession = pFreeSock->GetSession();
	CSockStream* socket_stream = pSession->GetSocket();
	SESSION_TYPE session_type = SESSION_TYPE_INVALID;
	if (socket_stream)
	{
		session_type = socket_stream->GetSessionType();
		switch (session_type)
		{
		case SESSION_TYPE_PASSIVE:
			{
				if (dwReason != DISCONNECT_REASON_OVERLAPPED)
				{
					BYTE error_check = socket_stream->Disconnect(dwReason);
					if (error_check == DISCONNECT_RESULT_SUCCESS || error_check == DISCONNECT_RESULT_PENDING)
						return TRUE;
				}				
			}		
			break;
		case SESSION_TYPE_ACTIVE:
			{			
				free_active_sessions_.push_back(dwSession);
			}
			break;
		}
	}

	if ((session_type == SESSION_TYPE_PASSIVE || dwSession == DISCONNECT_REASON_SESSION_LOST) && !pSession->IsReliableSession())
		m_IDPool.FreeID(dwSession);
	
	pSession->Close(dwReason);

	if (!pSession->IsReliableSession())
	{
		m_ActiveSessions.erase(iter);
		m_SessionPool.FreeItem(pSession);		
	}
	
	return TRUE;
}

long CSessionMgr::CloseStaleSessions(DWORD interval, DWORD dwTimeout)
{
	SCOPED_LOCK_SINGLE(&m_CS);

	CSession* pSession	= NULL;	
	CSockStream* socket_stream = nullptr;
	DWORD dwSession = 0;

	ULONGLONG current_tick = GetTickCount64();

	mapSESSION::iterator iter = m_ActiveSessions.begin();
	mapSESSION::iterator end_iter = m_ActiveSessions.end();

	for (iter; iter != end_iter;)
	{
		pSession = (iter->second);
		if (pSession->IsValidSession(current_tick, interval, dwTimeout))
		{
			++iter;
			continue;
		}
				
		socket_stream = pSession->GetSocket();
		if (socket_stream)
		{
			dwSession = pSession->GetID();
			switch (socket_stream->GetSessionType())
			{
			case SESSION_TYPE_PASSIVE:
				{
					if (socket_stream->GetDisconnectReason() != NULL || socket_stream->Disconnect(DISCONNECT_REASON_STALE_SESSION) == DISCONNECT_RESULT_SUCCESS)
					{
						++iter;
						continue;
					}

					if (!pSession->IsReliableSession())
					{
						m_IDPool.FreeID(dwSession);
					}
				}
				break;
			case SESSION_TYPE_ACTIVE:
				{
					socket_stream->SetDisconnectReason(NULL);
					socket_stream->Disconnect(DISCONNECT_REASON_STALE_SESSION);
					free_active_sessions_.push_back(dwSession);
				}
				break;
			}
		}
		
		pSession->Close(DISCONNECT_REASON_STALE_SESSION);

		if (!pSession->IsReliableSession())
		{
			iter = m_ActiveSessions.erase(iter);
			m_SessionPool.FreeItem(pSession);
		}		
	}
	
	return static_cast<DWORD>(m_ActiveSessions.size());
}

void CSessionMgr::CloseKeepAliveSessions(SESSIONS_TO_CONNECT& keep_alive_sessions)
{
	SCOPED_LOCK_SINGLE(&m_CS);

	for each (DWORD session_id in free_active_sessions_)
	{
		bool reliable_session = false;
		for (SESSIONS_TO_CONNECT::iterator it = keep_alive_sessions.begin(); it != keep_alive_sessions.end(); ++it)
		{
			SKeepAlive* keep_alive = *it;

			if (keep_alive->assoc_session_id == session_id)	// 음... 이넘 끊겼구나!
			{				
				if (keep_alive->register_session_id == session_id)
					reliable_session = keep_alive->reliable_session;

				keep_alive->assoc_session_id = 0;
				break;
			}
		}

		if (!reliable_session)
			m_IDPool.FreeID(session_id);
	}

	free_active_sessions_.clear();
}

void CSessionMgr::CloseAllPassiveSession()
{
	SCOPED_LOCK_SINGLE(&m_CS);
	DWORD dwSessionID;
	CSession* pSession = nullptr;

	mapSESSION::iterator iter = m_ActiveSessions.begin();
	mapSESSION::iterator end_iter = m_ActiveSessions.end();

	for (iter; iter != end_iter;)
	{
		pSession = iter->second;

		if( pSession->GetSessionType() != SESSION_TYPE_PASSIVE || pSession->IsReliableSession() == TRUE)
		{
			++iter;
			continue;
		}

		dwSessionID = pSession->GetID();
		pSession->Close(DISCONNECT_REASON_INTENTIONAL_KILL);

		iter = m_ActiveSessions.erase(iter);

		m_SessionPool.FreeItem( pSession);
		m_IDPool.FreeID( dwSessionID);
	}
}
