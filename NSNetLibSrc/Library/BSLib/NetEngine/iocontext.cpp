#include "StdAfx.h"
#include "IOContext.h"
#include "SockSystem.h"

CIOContext::CIOContext(long nOperation) : m_pBuffer(NULL), m_nFront(0), m_nBack(0), buffer_size_(0)
{
	m_nType		 = CONTEXT_TYPE_CIRCULAR;
	m_pOwner	 = NULL;
	m_nOperation = nOperation;
	
	::ZeroMemory(&m_Overlapped, sizeof(m_Overlapped));
	m_Overlapped.hEvent = WSA_INVALID_EVENT;

	m_dwOPTarget = IO_TARGET_DATA;

	m_nBufNum = 1;
}

CIOContext::~CIOContext()
{
	if (m_Overlapped.hEvent != WSA_INVALID_EVENT)
	{
		::WSACloseEvent(m_Overlapped.hEvent);
		m_Overlapped.hEvent = WSA_INVALID_EVENT;
	}
	
	SAFE_DELVEC(m_pBuffer);
}

void CIOContext::CreateBuffer(int buffer_size)
{
	if (buffer_size > buffer_size_)
	{
		if (m_pBuffer != NULL)
			SAFE_DELVEC(m_pBuffer);
	
		buffer_size_ = buffer_size;
		m_pBuffer = new BYTE[buffer_size_];
		ZeroMemory(m_pBuffer, buffer_size_);
	}
}

void CIOContext::Reset(BOOL bDeleteBuffer)
{	
	m_dwOPTarget = IO_TARGET_DATA;
	
	if (m_Overlapped.hEvent != WSA_INVALID_EVENT)
		::WSAResetEvent(m_Overlapped.hEvent);

	if (bDeleteBuffer == TRUE)
	{
		SAFE_DELVEC(m_pBuffer);
	}

	m_nFront = m_nBack = 0;
}

