#include "stdafx.h"
#include "Msg.h"

#include "netengine_def.h"

CMsg::CMsg(WORD id)
{
	////////////////////
	// CBSContext
	////////////////////
	m_nType			= CONTEXT_TYPE_MSG;
	m_nOperation	= IO_WRITE;
	m_dwOPTarget	= IO_TARGET_DATA;
	
	m_nBufNum		= 1;
	m_wsaBuf[0].buf = (LPSTR)m_pActiveBuffer;
		
	////////////////////
	// CMsg
	////////////////////
	m_nRdCur		= 0;
	m_nWrCur		= 0;
	m_nPeekPos		= 0;

	m_nRefCnt		= 0;
	
	m_bReadForward	= TRUE;
	m_pActiveBuffer = NULL;

	SetBuffer(m_Buffer, DEFAULT_MSGBUF_SIZE);
	m_pMassiveBuffer = NULL;

	//m_addrRelated.sin_addr.s_addr = 0;
	//m_addrRelated.sin_port = 0;

	m_bIsInUse = FALSE;
	m_dwThreadID = 0;

	*m_pID = id;
	build_id_ = id;
	profile_tick_ = 0;
}

void CMsg::SetBuffer(BYTE* pBuffer, DWORD cap)
{
	if (m_pActiveBuffer == pBuffer)
		return;

	BYTE* pOldBuffer = m_pActiveBuffer;
	m_pActiveBuffer = pBuffer;
	m_nBufCapacity  = cap;

	// header field pointer
	m_pDataSize		= (DWORD*)&m_pActiveBuffer[MSG_OFFSET_SIZE];
	m_pID			= (WORD*)&m_pActiveBuffer[MSG_OFFSET_ID];
	m_pSeq			= (ULONGLONG*)&m_pActiveBuffer[MSG_OFFSET_SEQ];	
	m_pCheckSum		= (BYTE*)&m_pActiveBuffer[MSG_OFFSET_CHECKSUM];

	if (pOldBuffer != NULL)
	{
		::memcpy(pBuffer, pOldBuffer, MSG_HEADER_SIZE);
	}
	
	m_wsaBuf[0].buf = (char*)pBuffer;
}

void CMsg::Resize(DWORD wSize)
{
	_ASSERT(wSize <= MSG_MASK_SIZE_ONLY);
	
	SAFE_DELVEC( m_pMassiveBuffer );

	m_pMassiveBuffer = new BYTE[wSize];	
	::memcpy(m_pMassiveBuffer, m_pActiveBuffer, MSG_HEADER_SIZE);

	SetBuffer(m_pMassiveBuffer, wSize);
}

DWORD CMsg::GetDataSize()
{ 
	return ((*m_pDataSize) & MSG_MASK_SIZE_ONLY); 
}

BOOL CMsg::SetRdPos(DWORD wPos, BOOL bIsOffsetFromHeader)
{ 
	if (bIsOffsetFromHeader == TRUE)	// header size 제외된 offset 값이 인자로 넘어왔다는 얘기다...
	{
		if (MSG_HEADER_SIZE + wPos > m_nBufCapacity)
			return FALSE;

		m_nRdCur = (MSG_HEADER_SIZE + wPos);
	}
	else
	{
		if (wPos < MSG_HEADER_SIZE || wPos > m_nBufCapacity)
			return FALSE;

		m_nRdCur = wPos;
	}

	return TRUE;
}

BOOL CMsg::SetWrPos(DWORD wPos, BOOL bIsOffsetFromHeader)
{		
	if (bIsOffsetFromHeader == TRUE)	// header size 제외된 offset 값이 인자로 넘어왔다는 얘기다...
	{
		if (MSG_HEADER_SIZE + wPos > m_nBufCapacity)
			return FALSE;

		m_nWrCur = (MSG_HEADER_SIZE + wPos);
	}
	else
	{
		if (wPos < MSG_HEADER_SIZE || wPos > m_nBufCapacity)
			return FALSE;

		m_nWrCur = wPos;
	}

	return TRUE;
}

BOOL CMsg::IsEncMsg()
{
	return ((*m_pDataSize) & MSG_FLAG_SIZEFIELD_ENCRYPTED) != 0;
}

void CMsg::SetAsEncrypted()
{ 
	(*m_pDataSize) |= MSG_FLAG_SIZEFIELD_ENCRYPTED; 
}

void CMsg::SetAsPlaneMsg()
{ 
	(*m_pDataSize) &= ~MSG_FLAG_SIZEFIELD_ENCRYPTED; 
}

void CMsg::Reset(long param)
{
	m_bReadForward	= TRUE;

	m_nRefCnt		= 0;
	m_dwSession		= 0; 
	m_dwThreadID	= 0;

	m_nRdCur	= m_nWrCur = MSG_HEADER_SIZE;
	m_nPeekPos	= 0;

	//m_bIsInUse	= FALSE;
	
	if (m_bIsInUse == TRUE)
	{
	//	_asm int 3
	//	PutLog(LOG_FATAL_FILE, _T("someone about to reset msg in use !!! [0x%x]"), GetMsgID());
	}

	SetBuffer(m_Buffer, DEFAULT_MSGBUF_SIZE);

	ClearHeader();

	SAFE_DELVEC(m_pMassiveBuffer);

	//m_addrRelated.sin_addr.s_addr = 0;
	//m_addrRelated.sin_port = 0;
	profile_tick_ = 0;
	profile_handler_ = nullptr;
}

void CMsg::SetDataSizeAtHeader(DWORD wSize)
{
	//ASSERTA0(wSize <= MSG_MASK_SIZE_ONLY, _T("Reset() 함수가 호출되지 않고 Msg가 사용된 듯 하다...") );
	*m_pDataSize = ((*m_pDataSize & MSG_MASK_SIZEFIELD_FLAGS) | wSize);
}

void CMsg::ReadBytesReverse(void* pBuf, DWORD nLen)
{
	if (nLen > m_nWrCur)
		throw MsgReverseReadException;

	m_nWrCur = m_nWrCur - nLen;

	BYTE* pCur = GetWrBuffer();
	::memcpy(pBuf,pCur,nLen);

	SetDataSizeAtHeader(m_nWrCur - MSG_HEADER_SIZE);
}

void CMsg::ReadBytes(void* pBuf, DWORD nLen)
{
	if ((m_nRdCur + nLen) > m_nWrCur)
		throw MsgReadException;

	BYTE* pCur = GetRdBuffer();
	::memcpy(pBuf, pCur, nLen);
	m_nRdCur = m_nRdCur + nLen;
}

void CMsg::WriteBytes(void* pBuf, DWORD nLen)
{
	if ((m_nWrCur + nLen) > MSG_MASK_SIZE_ONLY)	
		throw MsgWriteException;

	DWORD available_buf_size = GetAvailableBufSize();

	if (available_buf_size < nLen)
	{
		DWORD buffer_count = nLen / DEFAULT_MSGBUF_SIZE + 1;
		m_nBufCapacity += buffer_count * DEFAULT_MSGBUF_SIZE;
		BYTE* massive_buffer = new BYTE[m_nBufCapacity];
		
		DWORD data_size = GetDataSize();		
		if (data_size > 0)
			memcpy(massive_buffer + MSG_HEADER_SIZE, m_pActiveBuffer + MSG_HEADER_SIZE, data_size);

		SetBuffer(massive_buffer, m_nBufCapacity);
		delete [] m_pMassiveBuffer;
		m_pMassiveBuffer = massive_buffer;
	}

	BYTE* pCur = GetWrBuffer();
	::memcpy(pCur, pBuf, nLen);
	m_nWrCur = m_nWrCur + nLen;

	SetDataSizeAtHeader(m_nWrCur - MSG_HEADER_SIZE);
}

DWORD CMsg::CopyMsg(IMsg* pSrc, BOOL bUnreadOnly)
{
	BYTE* pSourceBuf = NULL;
	DWORD wDataSizeToCopy = 0;

	if (bUnreadOnly == TRUE)
	{
		wDataSizeToCopy = pSrc->GetRemainBytesToRead();
		pSourceBuf = pSrc->GetRdBuffer();
	}
	else
	{
		wDataSizeToCopy = pSrc->GetDataSize();
		pSourceBuf = static_cast<CMsg*>(pSrc)->GetBufferAt(MSG_HEADER_SIZE);
	}

	static_cast<CMsg*>(pSrc)->ForceStopRead();
	
	WriteBytes(pSourceBuf, wDataSizeToCopy);

	return wDataSizeToCopy;
}

void CMsg::StartProfile(const ProfileHandler& handler)
{
	if (handler)
		profile_handler_ = handler;

	profile_tick_ = GetTickCount64();
}

DWORD CMsg::StopProfile()
{
	DWORD elapsed_tick = 0;

	if (profile_tick_)
	{
		elapsed_tick = static_cast<DWORD>(GetTickCount64() - profile_tick_);
		if (profile_handler_)
			profile_handler_(elapsed_tick);

		profile_tick_ = 0;
	}

	return elapsed_tick;
}