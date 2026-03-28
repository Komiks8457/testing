#pragma once

#include "IMsg.h"

#define _DUMP_ALLOCATED_MSG

#define MSG_MASK_SIZE_ONLY		static_cast<DWORD>(0x000FFFFF)
#define DEFAULT_MSGBUF_SIZE		4096

enum
{
	CONTEXT_TYPE_MSG = 0,
	CONTEXT_TYPE_CIRCULAR,
	CONTEXT_TYPE_NUM
};

class IIOContext
{
	typedef struct _BSWSABUF 
	{
		unsigned long	len;
		char FAR *		buf;

	} BSWSABUF, FAR * LPBSWSABUF;

public:
	IIOContext() : m_nOperation(0), m_dwOPTarget(0), m_nType(0), m_nBufNum(0)
	{
		::ZeroMemory(&m_Overlapped, sizeof(m_Overlapped));
		::ZeroMemory(m_wsaBuf, sizeof(m_wsaBuf));
	}

	OVERLAPPED	m_Overlapped;

protected:
	long		m_nOperation;	
	DWORD		m_dwOPTarget;
	long		m_nType;
		
public:
	long		m_nBufNum;
	BSWSABUF	m_wsaBuf[2];

	void		SetContextType(long nType){m_nType = nType;}
	long		GetContextType(){return m_nType;}
	long		GetOperationMode() const { return m_nOperation; }		
	DWORD		GetOperationTarget() { return m_dwOPTarget; }
	void		SetOperationTarget(DWORD dwNewTarget) { m_dwOPTarget = dwNewTarget; }
};

/////////////////////////////////////
// CMsg
/////////////////////////////////////
class CMsg : public IIOContext, public IMsg
{
public:
	explicit CMsg(WORD id = 0);
	~CMsg()
	{
		SAFE_DELVEC( m_pMassiveBuffer );
	}

public:
	void	SetBuffer(BYTE* pBuffer, DWORD cap);
	void	Resize(DWORD wSize);

	////////////////////////////////////////////////////
	// header relatives
	void	ClearHeader() { ::ZeroMemory(&m_pActiveBuffer[0], MSG_HEADER_SIZE);}

	void	SetMsgID(WORD wMsgID) 
	{	
		*m_pID = wMsgID; 
		build_id_ = wMsgID;
	}

	void	GetHeader(WORD& wMsgID, DWORD& wSize) { wMsgID = GetMsgID(); wSize = GetMsgSize(); }
	WORD	GetMsgID() { return (WORD)(*m_pID); }

	DWORD	GetDataSize();
	DWORD	GetMsgSize()  { return (GetDataSize() + MSG_HEADER_SIZE); }			// Header를 포함한 사이즈

	void	SetCheckSum(BYTE btCheckSum) { *m_pCheckSum = btCheckSum; }
	void	SetSequence(ULONGLONG Seq) { *m_pSeq = Seq; }

	BYTE	GetCheckSum() { return *m_pCheckSum; }
	ULONGLONG GetSequence() { return *m_pSeq; }

	DWORD	GetAvailableBufSize()	{ _ASSERT(m_nWrCur <= m_nBufCapacity); return m_nBufCapacity - m_nWrCur; }
	void	SetDataSizeAtHeader(DWORD wSize);

	////////////////////////////////////////////////////
	// buffer offset
	BOOL	SetRdPos(DWORD wPos, BOOL bIsOffsetFromHeader = FALSE);
	BOOL	SetWrPos(DWORD wPos, BOOL bIsOffsetFromHeader = FALSE);

	DWORD	GetRdPos() { return m_nRdCur; }
	DWORD	GetWrPos() { return m_nWrCur; }

	void	IncRdPos(DWORD Pos) { m_nRdCur += Pos; }
	void	IncWrPos(DWORD Pos) { m_nWrCur += Pos; }

	void	DecRdPos(DWORD Pos) { m_nRdCur -= Pos; }
	void	DecWrPos(DWORD Pos) { m_nWrCur -= Pos; }

	void	ForceStopRead() { m_nRdCur = m_nWrCur; }
	BOOL	AllDataRead() { return (m_nRdCur == m_nWrCur); }
	DWORD	GetRemainBytesToRead() { return (m_nWrCur - m_nRdCur); }

	////////////////////////////////////////////////////
	// reference count
	int		GetRefCnt()	{ return m_nRefCnt; }
	long	AddRef()	{ return ::InterlockedIncrement(&m_nRefCnt); }
	long	DecRef()	
	{ 
		if (m_nRefCnt > 0)	// 음수로 가는 일은 없도록...
		{
			::InterlockedDecrement(&m_nRefCnt);
			return m_nRefCnt;
		}

		return 0;
	}

	////////////////////////////////////////////////////
	// diagnostics
	BOOL	IsEncMsg();
	void	SetAsEncrypted();
	void	SetAsPlaneMsg();

	////////////////////////////////////////////////////
	// buffer pointer
	void	ReadBytes(void* pBuf, DWORD nLen);
	void	WriteBytes(void* pBuf, DWORD nLen);
	void	ReadBytesReverse(void* pBuf, DWORD nLen);

	DWORD	GetCapacity() { return m_nBufCapacity; }
	BYTE*	GetRdBuffer() { return (m_pActiveBuffer + m_nRdCur); }
	BYTE*	GetWrBuffer() { return (m_pActiveBuffer + m_nWrCur); }

	BYTE*	GetBufferAt(DWORD nOffset)
	{ 
		_ASSERT(nOffset < m_nBufCapacity);
		return &m_pActiveBuffer[nOffset]; 
	}

	void	PrepareOperation() 
	{ 
		// 여기서 GetDataSize() 를 안쓰고 굳이 m_nWrCur를 사용한 이유는 
		// Msg를 암호화 할 경우 Header에 들어있는 Plane Text상태의 메시지 사이즈는
		// 변하면 안되거든? 그런 관계로 m_nWrCur를 encrypt된 상태의 데이터 사이즈로 외부에서
		// 강제 세팅하고 사용하기 때문이다. 그렇지 않으면 실제 WSASend 호출될 때, 
		// 암호화 되기 전의 사이즈 만큼만 날아가게 되거덩...
		m_wsaBuf[0].len = m_nWrCur; 
	}

	////////////////////////////////////////////////////
	// associated objects
	DWORD	GetSessionID()					{ return m_dwSession; }
	void	SetSessionID(DWORD dwSession)	{ m_dwSession = dwSession; }

	void	SetThreadID(DWORD dwThreadID)	{ m_dwThreadID = dwThreadID; }
	DWORD	GetThreadID()					{ return m_dwThreadID; }

	WORD build_id() const { return build_id_; }
	void set_build_id(WORD build_id) { build_id_ = build_id; }

	typedef std::function<void(DWORD elapsed_tick)> ProfileHandler;
	void StartProfile(const ProfileHandler& handler = NULL );
	DWORD StopProfile();

protected:
	BYTE	m_Buffer[DEFAULT_MSGBUF_SIZE];
	BYTE*	m_pActiveBuffer;
	BYTE*	m_pMassiveBuffer;
	DWORD	m_nRdCur;		// Header사이즈를 포함한 Offset
	DWORD	m_nWrCur;		// Header사이즈를 포함한 Offset	
	DWORD	m_nPeekPos;


	long	m_nRefCnt;

	BOOL	m_bReadForward;
	DWORD	m_nBufCapacity;

	// for msg_header direct accessing
	WORD*	m_pID;				// Msg ID
	DWORD*	m_pDataSize;		// Msg Size ( Not Header )	
	ULONGLONG*	m_pSeq;			// Msg Sequence
	BYTE*	m_pCheckSum;		// Msg CheckSum	

	// temporary associated object
	DWORD	m_dwSession;		// Msg SessionID
	DWORD	m_dwThreadID;

	long	m_bIsInUse;

	WORD build_id_;
	LONGLONG profile_tick_;
	ProfileHandler profile_handler_;

public:
	//SOCKADDR_IN m_addrRelated;

public:
	void	SetMsgInUse(long bInUse) { ::InterlockedExchange(&m_bIsInUse, bInUse); }
	BOOL	IsInUse() { return m_bIsInUse; }

	void	Reset(long);
	void	SetReadDirection(BOOL bForward)	{ m_bReadForward = bForward; }

	void	BeginPeek() { _ASSERT(m_nPeekPos == 0); m_nPeekPos = m_nRdCur; }
	void	EndPeek()	{ m_nRdCur = m_nPeekPos; m_nPeekPos = 0; }

	DWORD	CopyMsg(IMsg* pSrc, BOOL bUnreadOnly = TRUE);

	BOOL	IsReadForward() const { return m_bReadForward; }
};
