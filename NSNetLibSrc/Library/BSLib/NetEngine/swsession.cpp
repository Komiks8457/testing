#include "StdAfx.h"
#include "swsession.h"
#include "System.h"
#include "NetEngine.h"
#include <intrin.h>

volatile __int64 CSession::s_send_msg_sequence_ = GetTickCount64();

CSession::CSession(void) : m_CS(TRUE), post_msgs_cs_(TRUE)
{
	Reset();
}

CSession::~CSession(void)
{
	Close(0);
}

void CSession::Close(DWORD dwReason)
{
	if (m_dwTargetTask != NULL)
	{
		CTask* pTargetTask = _GetTargetTask();
		if (pTargetTask != NULL)
		{
			_PostMessagetoTargetService(ENM_SESSION_LOST, m_dwID, (m_pSocket) ? m_pSocket->GetDisconnectReason() : dwReason);
		}
	}

	CNetEngine* engine = GetEngine();

	if (engine != nullptr && !engine->IsShutdown())
	{	
		if (m_pSocket)
		{
			ULONGLONG latest_tick = m_pSocket->GetLatestAccessTick();
			ULONGLONG current_tick = GetTickCount64();
			DWORD elapsed_tick = static_cast<DWORD>(current_tick - latest_tick);

			if (keep_alive_count_ || elapsed_tick >= MAINTAINER_STALE_SESSION_INTERVAL)
			{
				sockaddr_in addr = m_pSocket->GetPeerIp();
				std::wstring ip;
				CSockSystem::GetIP((DWORD)(addr.sin_addr.S_un.S_addr), &ip);

				PutLog(LOG_FATAL_FILE, L"keep alive session close - type:%d, id:%u, serial:%llu, ip:%s, warning:%u, recovery:%u, latest:%llu, current:%llu, elapsed:%d",
					m_pSocket->GetSessionType(), m_dwID, serial_number_, ip.c_str(), keep_alive_warning_, keep_alive_recovery_, latest_tick, current_tick, elapsed_tick);
			}

			engine->FreeTypeSocket(m_pSocket);
			m_pSocket = nullptr;
		}

		CloseFile();
	}

	if (linked_session_)
	{
		linked_session_->DetachLinkedSocket();
		linked_session_ = nullptr;
	}

	if (!IsReliableSession())
		Reset();
}

void CSession::Invalidate()
{
	if (m_pSocket != nullptr)
		m_pSocket->Invalidate();
}

void CSession::Reset()
{
	m_pFile	= nullptr;
	m_dwID = NULL;
	m_pSocket = nullptr;
	m_dwTargetTask = NULL;
	m_dwCurFileIOBytes = 0;
	m_bIsReliableSession = false;
	keep_alive_count_ = 0;
	keep_alive_warning_ = 0;
	keep_alive_recovery_ = 0;
	serial_number_ = 0;	
	linked_session_ = nullptr;
	linked_socket_ = nullptr;
	receive_msg_sequence_ = 0;
	send_msg_sequence_ = 0;
}

///////////////////////////////////////////////////////////////////////////
// 여기선 DelMsg해주지 않는다. 
// 왜? CSockTCP의 SendMsg()가 불리지 않았기 때문에
// Reference Count가 1이거덩... 
// 그니까 여기서 지우고, 실제로 사용하는 곳에서도 지우게 되면 ref_count 가 -1이 되어버리니깐...
///////////////////////////////////////////////////////////////////////////
void CSession::_PostMessagetoTargetService(WORD wNotifyMsg, DWORD dwData1, ULONG_PTR Data2)
{	
	CTask* pTargetTask = _GetTargetTask();
	if (pTargetTask != NULL)
	{
		CServiceObject* pAssociatedService = pTargetTask->GetServiceObject();
		if (pAssociatedService != NULL)	// service object가 없다면 message 날려봐야 뽑아줄 넘이 없는 것이거덩...
		{	
			CNetEngine* engine = GetEngine();
			if (engine)
			{
				// Engine Notify Message는 4Byte 헤더로 간다 :)
				CMsg* pNM = engine->NewMsg(FALSE);
				pNM->SetMsgID(wNotifyMsg);
				pNM->SetSessionID( dwData1);
				*pNM << dwData1 << Data2;	
				pNM->SetRdPos(0, TRUE);
				pTargetTask->EnqueMsg(pNM);
			}
		}
	}
}

bool CSession::Create(CSockStream* pSock, DWORD dwID, DWORD dwTargetTask )
{
	_ASSERT(m_pSocket == NULL);
	_ASSERT(pSock != NULL && dwID != NULL);
	_ASSERT(pSock->IsValidSock() == TRUE);

	SCOPED_LOCK_SINGLE(&m_CS);

	m_pSocket	= pSock;
	m_dwID		= dwID;

	if (_SetTask(dwTargetTask) == FALSE)
		return false;

	CTask* io_task = _GetTask(TASK_SYS_NETIO);
	if (io_task == nullptr)
		return false;

	if (m_pSocket->LetsRock(io_task->GetServiceObject()) == FALSE)
		return false;

	m_pSocket->SetSession(this);

	_PostMessagetoTargetService(ENM_SESSION_CREATED, dwID, m_pSocket->GetSessionType());
	return true;
}

void CSession::SessionHandshakingCompleted()
{	
	// 양쪽에서 session encoding 방식이 결정되었으므로 
	// app 측의 Task에게 Session 생성됬음을 알려주자.
	_PostMessagetoTargetService(ENM_SESSION_HANDSHAKED, m_dwID, MSG_HEADER_SIZE);
}

CTask* CSession::_GetTargetTask()
{
	return _GetTask(m_dwTargetTask);
}

CTask* CSession::_GetTask(DWORD dwTaskID)
{
	if (IS_VALID_TASK_ID(dwTaskID) == FALSE)
		return NULL;

	CNetEngine* engine = GetEngine();
	if (engine == nullptr)
		return nullptr;

	CTaskManager* task_manager = engine->system().GetTaskManager();
	if (task_manager == NULL)
		return NULL;

	return task_manager->GetTask(dwTaskID);
}

BOOL CSession::_SetTask(DWORD dwTaskID)
{
	if (IS_VALID_TASK_ID(dwTaskID) == FALSE)
		return FALSE;

	CNetEngine* engine = GetEngine();
	if (engine == nullptr)
		return FALSE;

	CTaskManager* task_manager = engine->system().GetTaskManager();
	if (task_manager == nullptr)
		return FALSE;

	CTask* pTask = task_manager->GetTask(dwTaskID);
	if (pTask == NULL)
		return FALSE;

	m_pSocket->SetCurTask(pTask);
	m_dwTargetTask = dwTaskID;

	return TRUE;
}

bool CSession::IsValidSession(ULONGLONG current_tick, DWORD interval, DWORD dwTimeout)
{
	SCOPED_LOCK_SINGLE(&m_CS);
	
	if (m_pSocket == NULL)
		return IsReliableSession();

	if (m_pSocket->IsValidSock() == FALSE)
		return false;

	switch (m_pSocket->GetDisconnectReason())
	{
	case DISCONNECT_REASON_OVERLAPPED:
	case DISCONNECT_REASON_STALE_SESSION:
		return false;
	}
	
	if (linked_socket_ == m_pSocket)
		return true;

	ULONGLONG latest_tick = m_pSocket->GetLatestAccessTick();
	DWORD elapsed_tick = (current_tick > latest_tick) ? static_cast<DWORD>(current_tick - latest_tick) : 0;
	if (elapsed_tick >= interval)
	{
		++keep_alive_count_;
		++keep_alive_warning_;		

		if (elapsed_tick > dwTimeout)
		{
			sockaddr_in addr = m_pSocket->GetPeerIp();
			std::wstring ip;
			CSockSystem::GetIP((DWORD)(addr.sin_addr.S_un.S_addr), &ip);

			PutLog(LOG_FATAL_FILE, L"session keep alive timeout - type:%d, id:%d, serial:%llu, ip:%s, warning:%u, recovery:%u, latest:%llu, current:%llu, elapsed:%u",
				m_pSocket->GetSessionType(), m_dwID, serial_number_, ip.c_str(), keep_alive_warning_, keep_alive_recovery_, latest_tick, current_tick, elapsed_tick);

			return false;
		}
	}
	else if (keep_alive_count_ > 0)
	{
		--keep_alive_count_;
		++keep_alive_recovery_;
	}

	return true;
}

bool CSession::ReceiveMsg(CMsg* msg)
{	
	if (linked_session_)
	{		
		if (!linked_session_->ReceiveMsg(msg))
			return false;
	}
	else
	{
		if (m_bIsReliableSession)
		{
			if (linked_socket_)
			{
				if (linked_socket_ == m_pSocket)
					msg->SetSessionID(GetID());
				else
					msg->SetSequence(0);
			}

			ULONGLONG msg_sequence = msg->GetSequence();
			if (msg_sequence > 0)
			{
				if (msg_sequence <= receive_msg_sequence_)
				{
					PutLog(LOG_FATAL_FILE, L"ReceiveMsg - id:%u, seq:(%llu-%llu), session:%u", msg->GetMsgID(), receive_msg_sequence_, msg_sequence, m_dwID);
					return false;
				}
				
				receive_msg_sequence_ = msg_sequence;
			}
		}

		msg->SetSequence(0);
	}

	return true;
}

BOOL CSession::SetEncodeContext(EncodeCtx& ec)
{
	SCOPED_LOCK_SINGLE(&m_CS);

	if (m_pSocket != NULL)
	{
		return m_pSocket->ProtectSession(ec);
	}

	return FALSE;
}

/******************************************************************
// file transfering Invokers
******************************************************************/
BOOL CSession::RecvFile(FILE_TYPE file_type, std::wstring strFileFullPath, DWORD dwFileSize, long nOffset)
{
	if (m_pFile != NULL)	// 윽! 설마 이전에 작업하던 화일 Close 안한 것은 아니겠지!!!
	{
		return FALSE;
	}

	if (m_pSocket == NULL || dwFileSize == 0)
	{
		return FALSE;
	}
	
	if (_PrepareFileTransfer(file_type, strFileFullPath, dwFileSize, nOffset) == FALSE)
	{
		FileTransferCompleted(IO_WRITE, FALSE);
		return FALSE;
	}

	if (m_pSocket->RecvFile() != NERR_GOOD)
	{
		FileTransferCompleted(IO_WRITE, FALSE);
		return FALSE;
	}

	return TRUE;
}

BOOL CSession::SendFile(FILE_TYPE file_type, std::wstring strFileFullPath, long nOffset)
{
	if (m_pFile != NULL)	// 윽! 설마 이전에 작업하던 화일 Close 안한 것은 아니겠지!!!
	{
		PutLog(LOG_FATAL, _T("CSession::SendFile() - 켁 m_pFile이 NULL이 아니다! 이전에 Send하던 화일이 아직 남아있는 거냐?"));
		return FALSE;
	}
	
	if (_PrepareFileTransfer(file_type, strFileFullPath, 0, nOffset) == FALSE)
	{
		PutLog(LOG_FATAL, _T("CSession::SendFile() - _PrepareFileTransfer() 실패!"));
		FileTransferCompleted(IO_READ, FALSE);
		return FALSE;
	} 

	if (m_pSocket == NULL)
	{
		PutLog(LOG_FATAL, _T("CSession::SendFile() - 켁 Socket Pointer가 NULL 이다!"));
		return FALSE;
	}
	
	if (m_pSocket == NULL || m_pSocket->SendFile(m_pFile, nOffset) != NERR_GOOD)
	{
		PutLog(LOG_FATAL, _T("CSession::SendFile() - m_pSocket->SendFile() 실패"));
		FileTransferCompleted(IO_READ, FALSE);
		return FALSE;
	}

	return TRUE;
}

////////////////////////////
// dwForWrite:
// read  경우는 0
// write 경우는 file_size 
// 가 들어가야 한다.
////////////////////////////
BOOL CSession::_PrepareFileTransfer(FILE_TYPE file_type_to_open, std::wstring& strFileFullPath, DWORD dwForWrite, long nOffset)
{
	if (file_type_to_open != FILE_TYPE_MEM && 
		file_type_to_open != FILE_TYPE_LEGACY &&
		file_type_to_open != FILE_TYPE_MEMORYMAPPED)
	{
		return FALSE;
	}

	if (m_pFile != NULL)
	{
		CloseFile();
	}

	DWORD dwFileHandle = NULL;

	long rval = FILESYSTEM.OpenFile(file_type_to_open, strFileFullPath, dwForWrite, m_dwID, dwFileHandle);
	if (rval == FSERR_CANT_OPEN_FILE)
	{
		if (dwForWrite > 0)	// write 용으로 open한 거면
		{
			// path를 이루는 폴더들 전부 만들고 재시도 해보자...
			if (_GeneratePathDir(strFileFullPath) == FALSE)	
				return FALSE;

			rval = FILESYSTEM.OpenFile(file_type_to_open, strFileFullPath, dwForWrite, m_dwID, dwFileHandle);
		}
	}

	if (rval != FSERR_GOOD)
		return FALSE;

	_ASSERT(dwFileHandle != NULL);

	m_pFile = FILESYSTEM.GetFile(dwFileHandle);
	if (m_pFile == NULL)
	{
		return FALSE;
	}

	m_dwCurFileIOBytes = 0;
	
	if (nOffset > 0)	// 이어받기
	{
		if ((DWORD)nOffset >= m_pFile->GetFileSize())
		{
			CloseFile();
			return FALSE;
		}

		m_pFile->Seek(m_dwID, 0);
		m_dwCurFileIOBytes = nOffset;
	}

	return TRUE;
}

BOOL CSession::_GeneratePathDir(std::wstring& strFullPath)
{
	std::wstring strTemp;
	WCHAR string[1024];
	
	WCHAR str_drive[MAX_PATH];
	WCHAR str_dir[ MAX_PATH];

	// folder를 분리해서
	_wsplitpath_s( strFullPath.c_str(), str_drive, MAX_PATH, str_dir, MAX_PATH, NULL, 0, NULL, 0 );

	_wmakepath_s( string, str_drive, str_dir, NULL, NULL);
	
	WCHAR seps[] = L"\\";
	WCHAR* szNextTok = NULL;

	WCHAR* token = ::wcstok_s( string, seps, &szNextTok );
	while (token != NULL)
	{ 
		strTemp += token;

		// 가능한 error_code는 ERROR_ALREADY_EXISTS 인데... 이건 어차피 무시해야 하니깐... 에러처리 하지 않는다.
		::CreateDirectory(strTemp.c_str(), NULL);
						
		strTemp += L"\\";
		
		token = ::wcstok_s( seps, seps, &szNextTok );
	}

	return TRUE;
}

/******************************************************************
// file transfering event handlers
******************************************************************/

///////////////////////////////////////////////////////////////////
// 화일 데이터 전송 event handler
///////////////////////////////////////////////////////////////////
BOOL CSession::OnFileDataSent(DWORD dwSendBytes)
{
	m_dwCurFileIOBytes += dwSendBytes;

	if (m_pFile->GetFileSize() <= m_dwCurFileIOBytes)
	{
		return TRUE;
	}
	else	
		return FALSE;	
}

///////////////////////////////////////////////////////////////////
// 화일 데이터 수신 event handler
///////////////////////////////////////////////////////////////////

// 실제루 화일에 몇바이트 씌여졌는지 알려주께...
int CSession::_GetCurProgressPercent()
{
	_ASSERT(m_pFile != NULL);

	int Unit = m_pFile->GetFileSize() / 100;
	int nProgress = 0;

	if (Unit == 0)	// 화일 사이즈가 100바이트도 안되나보군...
	{
		if (m_dwCurFileIOBytes == 0)
			nProgress = 0;
		else
			nProgress = 100;
	}
	else
		nProgress = (m_dwCurFileIOBytes / Unit);

	return nProgress;
}

BOOL CSession::OnFileDataReceived(DWORD dwWrittenBytes)
{
	int OldProgressPercent = _GetCurProgressPercent();

	m_dwCurFileIOBytes += dwWrittenBytes;

	int NewProgressPercent = _GetCurProgressPercent();

	if (OldProgressPercent != NewProgressPercent)	// 너무 많은 notify가 날아가는 것 방지하기 위해서... 최대 100번만 notify 날아간다.
	{
		_PostMessagetoTargetService(ENM_FILEDATA_RECEIVED, m_dwID, m_dwCurFileIOBytes);
	}
	
	if (m_dwCurFileIOBytes >= m_pFile->GetFileSize())
	{
		return TRUE;
	}	

	return FALSE;
}

void CSession::SetReliableSession(bool onoff)
{
	m_bIsReliableSession = onoff;
}

bool CSession::AttachLinkedSession(CSession* linked_session)
{
	if (linked_session == nullptr)
	{
		PutLog(LOG_FATAL_FILE, L"AttachLinkedSession - session:%d", m_dwID);
		return false;
	}

	if (linked_session == this)
	{
		PutLog(LOG_FATAL_FILE, L"AttachLinkedSession - session:%d", m_dwID);
		return false;
	}

	if (linked_session_ != nullptr)
	{
		PutLog(LOG_FATAL_FILE, L"AttachLinkedSession - session:%d, linked:%d", m_dwID, linked_session_->GetID());
		return false;	
	}
	
	if (!linked_session->AttachLinkedSocket(m_pSocket))
	{
		PutLog(LOG_FATAL_FILE, L"AttachLinkedSession - session:%d, socket:%d", m_dwID, m_pSocket->GetHandle());
		return false;
	}

	linked_session_ = linked_session;
	PutLog(LOG_NOTIFY_FILE, L"AttachLinkedSession - session:%d, linked:%d, socket:%d", m_dwID, linked_session_->GetID(), m_pSocket->GetHandle());
	return true;
}

bool CSession::AttachLinkedSocket(CSockStream* linked_socket)
{
	if (linked_socket == nullptr)
	{
		PutLog(LOG_FATAL_FILE, L"AttachLinkedSocket - session:%d", m_dwID);
		return false;
	}

	if (!m_bIsReliableSession)
	{
		PutLog(LOG_FATAL_FILE, L"AttachLinkedSocket - session:%d", m_dwID);
		return false;
	}

	if (linked_socket == m_pSocket)
	{
		PutLog(LOG_FATAL_FILE, L"AttachLinkedSocket - session:%d(%d), socket:%d", m_dwID, linked_socket->GetSessionID(), linked_socket->GetHandle());
		return false;
	}

	if (linked_socket_)
	{
		PutLog(LOG_FATAL_FILE, L"AttachLinkedSocket - session:%d(%d)", m_dwID, linked_socket_->GetSessionID());
		return false;
	}

	if (m_pSocket)
	{
		PutLog(LOG_FATAL_FILE, L"AttachLinkedSocket - session:%d, socket:%d", m_dwID, m_pSocket->GetHandle());
		return false;
	}

	linked_socket_ = linked_socket;
	PutLog(LOG_NOTIFY_FILE, L"AttachLinkedSocket - session:%d(%d), socket:%d", m_dwID, linked_socket_->GetSessionID(), linked_socket_->GetHandle());
	return true;
};

void CSession::DetachLinkedSocket()
{
	if (linked_socket_ == nullptr)
	{
		PutLog(LOG_FATAL_FILE, L"DetachLinkedSocket - session:%d", m_dwID);
		return;
	}
	
	if (m_pSocket && linked_socket_ != m_pSocket)
	{
		PutLog(LOG_FATAL_FILE, L"DetachLinkedSocket - session:%d, socket:%d", m_dwID, m_pSocket->GetHandle());
		return;
	}
	
	PutLog(LOG_NOTIFY_FILE, L"DetachLinkedSocket - session:%d, socket:%d", m_dwID, linked_socket_->GetHandle());		
	linked_socket_ = nullptr;
	m_pSocket = nullptr;
}

void CSession::SkipMsgSecurity()
{
	if (m_pSocket)
		m_pSocket->SkipMsgSecurity();
}

CSockStream* CSession::GetSocket()
{
	return (linked_socket_) ? linked_socket_ : m_pSocket;
}

bool CSession::UpdateReliableSession()
{
	if (linked_session_)
	{
		return linked_session_->UpdateReliableSession();
	}
	else
	{
		CSockStream* socket = GetSocket();
		if (socket == nullptr)
		{
			PutLog(LOG_TRACE_FILE, L"UpdateReliableSession - session:%u", m_dwID);
			return false;
		}

		if (!m_bIsReliableSession)
		{
			PutLog(LOG_TRACE_FILE, L"UpdateReliableSession - session:%u, socket:%llu", m_dwID, socket->GetHandle());
			return false;
		}

		std::vector<CMsg*> msgs;

		post_msgs_cs_.Lock();
		if (!post_msgs_.empty())
		{
			msgs.reserve(post_msgs_.size());
			for each (CMsg* msg in post_msgs_)
			{
				msg->DecRef();
				msgs.push_back(msg);
			}

			post_msgs_.clear();
		}
		post_msgs_cs_.Unlock();

		long error_code = NERR_GOOD;
		for each (CMsg* msg in msgs)
		{
			if (error_code == NERR_GOOD)
			{
				error_code = socket->SendMsg(msg);
				if (error_code == NERR_GOOD)
					continue;

				PutLog(LOG_FATAL_FILE, L"SendMsg - error:%d, id:%d, seq:%llu, session:%d", error_code, msg->GetMsgID(), msg->GetSequence(), m_dwID);
			}

			PostMsg(msg);
		}

		if (error_code != NERR_GOOD)
		{
			PutLog(LOG_FATAL_FILE, L"UpdateReliableSession - error:%d, session:%d", error_code, m_dwID);
			return false;
		}
		
		m_pSocket = socket;
		PutLog(LOG_TRACE_FILE, L"UpdateReliableSession - session:%u, socket:%llu, msg:%d", m_dwID, m_pSocket->GetHandle(), msgs.size());
		return true;
	}
}

bool CSession::ResetReliableSession()
{
	if (!m_bIsReliableSession)
	{
		PutLog(LOG_FATAL_FILE, L"ResetReliableSession - session:%d", m_dwID);
		return false;
	}

	receive_msg_sequence_ = 0;
	return true;
}
	
/////////////////////////////////////////////////////////////////////////
// 화일 전송(수신/송신) 완료됬다
/////////////////////////////////////////////////////////////////////////
void CSession::FileTransferCompleted(int io_mode, BOOL bSuccess)
{
	if (bSuccess == FALSE)
	{
		CloseFile();

		_PostMessagetoTargetService(ENM_FILE_TRANSFER_FAILED, m_dwID, NULL);
	}
	else
	{
		// File Close 의 최종 책임은 APP 로 돌리기 위해 임의의 File 참조자를 추가한다.
		// RefCount > 0 상태로 만들어야 뭐 압축을 푼다거나 하는 동안 Session이 닫혀도 
		// CSession::CloseFile() 호출 시에도 File이 Valid한 상태로 유지 되니까...
		// 한마디로! RefCount++ 을 하기위해 아래와 같이 복잡한 수작이 들어갔다.
		/*
		DWORD dwFileHandle = NULL;
		std::tstring file_name = m_pFile->GetFileName();
		DWORD dwForWrite = (m_pFile->GetOpenMode() == IFile::OPEN_MODE_READ) ? 0 : m_pFile->GetFileSize();
		FILESYSTEM.OpenFile(m_pFile->GetFileType(), file_name, dwForWrite, FS_RESERVED_SYSTEM_SESSION_ID, dwFileHandle);

		_ASSERT(dwFileHandle == m_pFile->GetFileHandle());
		*/
		m_pFile->AddContext(FS_RESERVED_SYSTEM_SESSION_ID);
		IFile* pFile = m_pFile;

		CloseFile();

		_PostMessagetoTargetService(ENM_FILE_TRANSFER_COMPLETED, m_dwID, (ULONG_PTR)pFile);
	}
}

size_t CSession::CloseFile()
{
	// meaning of the -1 is 'unknown'
	size_t nRefCnt = SIZE_MAX;
	
	if (m_pFile != NULL)
	{
		nRefCnt = FILESYSTEM.CloseFile(m_pFile->GetFileHandle() ,m_dwID );
		
		m_pFile				= NULL;
		m_dwCurFileIOBytes	= 0;
	}

	return nRefCnt;
}

long CSession::SendMsg(CMsg* pMsg)
{
	long error_code = NERR_UNKNOWN;

	if (m_pSocket && m_pSocket->IsValidSock())
	{
		SetMsgSequence(pMsg);
		long rcount_prev = pMsg->GetRefCnt();
		error_code = m_pSocket->SendMsg(pMsg);
		switch (error_code)
		{
		case NERR_GOOD:
		case NERR_NET_ACCESSED_BEFORE_HANDSHAKE:
			break;
		default:
			{
				if (pMsg->GetRefCnt() > rcount_prev)
					pMsg->DecRef();

				if (PostMsg(pMsg) == NERR_GOOD)
					error_code = NERR_GOOD;
			}
			break;
		}
	}
	else
	{		
		error_code = PostMsg(pMsg);
	}

	return error_code;
}

long CSession::PostMsg(CMsg* pMsg)
{	
	if (!IsReliableSession())
		return NERR_INVALID_SESSION;

	pMsg->AddRef();
	SetMsgSequence(pMsg);
	post_msgs_cs_.Lock();
	post_msgs_.insert(pMsg);
	post_msgs_cs_.Unlock();
	return NERR_GOOD;
}

void CSession::SetMsgSequence(CMsg* msg)
{	
	if (IsReliableSession() && msg->GetSequence() == 0)
	{
		send_msg_sequence_ = static_cast<ULONGLONG>(InterlockedIncrement64(&s_send_msg_sequence_));
		msg->SetSequence(send_msg_sequence_);
	}
}
