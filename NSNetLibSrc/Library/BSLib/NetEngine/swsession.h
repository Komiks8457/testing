#pragma once

#include "SocketTCP.h"
#include "FileSystem.h"
#include "../util\classlink.h"

class CSession
{	
public:
	CSession(void);
	virtual ~CSession(void);

protected:
	bool			m_bIsReliableSession;	
	DWORD			m_dwID;
	DWORD			m_dwTargetTask;
	DWORD			m_dwCurFileIOBytes;
	
	IFile*			m_pFile;
	CSockStream*	m_pSocket;
	
	CCriticalSectionBS	m_CS;
	CCriticalSectionBS	post_msgs_cs_;

	DWORD			keep_alive_count_;
	DWORD			keep_alive_warning_;
	DWORD			keep_alive_recovery_;
	ULONGLONG		serial_number_;	
	CSession*		linked_session_;
	CSockStream*	linked_socket_;
	
	CNetEngine*		GetEngine() const
	{		
		if (m_pSocket)
			return m_pSocket->engine();

		return nullptr;
	}

	struct MsgKeyCompare { bool operator()(CMsg* lhs, CMsg* rhs) const { return lhs->GetSequence() < rhs->GetSequence(); } };
	typedef std::set<CMsg*, MsgKeyCompare> PostMsgs;
	PostMsgs post_msgs_;

	ULONGLONG receive_msg_sequence_;
	ULONGLONG send_msg_sequence_;
	static volatile __int64 s_send_msg_sequence_;

public:
	void	Close(DWORD dwReason);
	void	Reset();
	void	Invalidate();
	bool	Create(CSockStream* pSock, DWORD dwID, DWORD dwTargetTask);

	BOOL	SetEncodeContext(EncodeCtx& ec);
	long	SendMsg(CMsg* pMsg);
	long	PostMsg(CMsg* pMsg);
	void	SetMsgSequence(CMsg* msg);
	void	SessionHandshakingCompleted();

	/******************************************************************
	// file transfering Invokers
	******************************************************************/
	BOOL	RecvFile(FILE_TYPE file_type, std::wstring strFileFullPath, DWORD dwFileSize, long nOffset = 0);
	BOOL	SendFile(FILE_TYPE file_type, std::wstring strFileFullPath, long nOffset = 0);
	size_t	CloseFile();

	/******************************************************************
	// file transfering event handlers
	******************************************************************/
	BOOL	OnFileDataSent(DWORD dwSendBytes);
	BOOL	OnFileDataReceived(DWORD dwWrittenBytes);

	/******************************************************************
	// misc.
	******************************************************************/
	bool	AttachLinkedSession(CSession* linked_session);
	bool	AttachLinkedSocket(CSockStream* linked_socket);
	void	DetachLinkedSocket();
	void	SetReliableSession(bool onoff);
	bool	UpdateReliableSession();
	bool	ResetReliableSession();
	bool	IsReliableSession() const { return m_bIsReliableSession; }
	bool	IsValidSession(ULONGLONG current_tick, DWORD interval, DWORD dwTimeout);
	bool	ReceiveMsg(CMsg* msg);
	void	SkipMsgSecurity();

	long			GetCountOfOutstandingSend();
	DWORD			GetID() { return m_dwID; }
	CSockStream*	GetSocket();

	IFile*			GetWorkingFile() { return m_pFile; }
	SESSION_TYPE	GetSessionType() { if (m_pSocket == NULL) return SESSION_TYPE_INVALID; return m_pSocket->GetSessionType(); }

	void	FileTransferCompleted(int io_mode, BOOL bSuccess);
	
//	long	GetSessionCountOfClassLink() { return GET_CLASSLINK_CNT(CSession); }

	void set_serial_number(ULONGLONG serial_number) { serial_number_ = serial_number; }
	ULONGLONG serial_number() const { return serial_number_; }

protected:
	CTask*	_GetTargetTask();
	CTask*	_GetTask(DWORD dwTaskID);
	BOOL	_SetTask(DWORD dwTaskID);
	void	_PostMessagetoTargetService(WORD wNotifyMsg, DWORD dwData1, ULONG_PTR Data2);
	BOOL	_PrepareFileTransfer(FILE_TYPE file_type_to_open, std::wstring& strFileFullPath, DWORD dwForWrite, long nOffset);
	BOOL	_GeneratePathDir(std::wstring& strFullPath);
	int		_GetCurProgressPercent();
	
	friend class CSessionMgr;
};





















