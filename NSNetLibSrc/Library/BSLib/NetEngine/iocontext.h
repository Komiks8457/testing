#pragma once

#include "Interface.h"

#define DEFAULT_BUFFER_SIZE	DEFAULT_MSGBUF_SIZE

class CBaseSocket;
class IFile;

class CIOContext : public IIOContext
{
public:
	CIOContext(long nOperation);
	~CIOContext();

private:
	BYTE*	m_pBuffer;
	long	m_nFront;
	long	m_nBack;
	int		buffer_size_;

	CBaseSocket*	m_pOwner;	

protected:
	BYTE* GetEmptyBuffer(OUT DWORD& size)
	{
		size = buffer_size_ - m_nFront;
		return (m_pBuffer + m_nFront);
	}

	BYTE* GetDataBuffer(OUT DWORD& size)
	{
		size = m_nFront;
		return m_pBuffer;
	}
	
public:
	void Reset(BOOL bDeleteBuffer);
	void CreateBuffer(int buffer_size = DEFAULT_BUFFER_SIZE);
	
	int GetRemainingBytes()
	{
		_ASSERT(m_nFront >= m_nBack);
		return (m_nFront - m_nBack);
	}

	void ClearOverlapped()
	{
		memset( &m_Overlapped, 0, sizeof( OVERLAPPED ) );
	}

	void IOCompleted(DWORD dwBytes)
	{
		if (dwBytes == 0)
			return;
		
		if (m_nOperation == IO_READ)
		{
			m_nFront += dwBytes;
			_ASSERT(m_nFront <= buffer_size_);
		}
		else if (m_nOperation == IO_WRITE)
		{
			int remain = m_nFront - dwBytes;
			if (remain > 0)
			{
				//PutLog(LOG_WARNING, _T("!!!! 오홋! 이런 경우가 있긴 있네?"));
				_ASSERT(FALSE);
				::memmove(m_pBuffer, m_pBuffer + dwBytes, remain);
			}
			m_nFront = remain;
		}
	}

	void PrepareOperation()
	{
		switch (m_nOperation)
		{
		case IO_WRITE:
			{
				m_wsaBuf[0].buf = (char*)GetDataBuffer(m_wsaBuf[0].len);
				m_nBufNum = 1;
			}
			break;
		case IO_READ:
			{
				m_wsaBuf[0].buf = (char*)GetEmptyBuffer(m_wsaBuf[0].len);
				m_nBufNum = 1;
			}				
			break;

		case IO_ACCEPT:
			m_wsaBuf[0].buf = (char*)m_pBuffer;
			m_wsaBuf[0].len = buffer_size_ - 1;
			m_nBufNum = 1;
			break;
		default:
			_ASSERT(0);
		}
	}

	HANDLE			GetEventHandle() { return m_Overlapped.hEvent; }
	void			SetOwner(CBaseSocket* pSock) { m_pOwner = pSock; }
	CBaseSocket*	GetOwner() { return m_pOwner; }
	BYTE*			GetBuffer() { return m_pBuffer; }
	BOOL			IsEmpty() 	{ return (m_nFront == m_nBack); }
	BOOL			IsFull()	{ return (m_nFront == buffer_size_); }
	int				buffer_size() const { return buffer_size_; }
		
	int				WriteBytes(BYTE* pSrc, int Len)
	{
		if (Len == 0)
			return 0;

		int overflow = (m_nFront + Len) - buffer_size_;
		if (overflow > 0)
			Len -= overflow;

		if (Len <= 0)
			return 0;
		
		::memcpy(m_pBuffer + m_nFront, pSrc, Len);
		m_nFront += Len;

		return Len;
	}

	int			ReadBytes(BYTE* pDst, int Len)
	{
		if (Len == 0)
			return 0;

		if (m_nFront == m_nBack)
			return 0;
		
		int data_size = m_nFront - m_nBack;
		_ASSERT(data_size > 0);
		
		Len = (Len > data_size) ? data_size : Len;
		::memcpy(pDst, m_pBuffer + m_nBack, Len);
		m_nBack += Len;

		return Len;
	}

	void		TrimBuffer()
	{
		if (m_nFront == m_nBack)
		{
			m_nFront = m_nBack = 0;
			return;
		}

		int remain = m_nFront - m_nBack;
		_ASSERT(remain > 0 && remain < MSG_HEADER_SIZE);
		::memmove(m_pBuffer, m_pBuffer + m_nBack, remain);

		m_nBack  = 0;
		m_nFront = remain;
	}
};
