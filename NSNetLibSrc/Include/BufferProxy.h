#pragma once

class CStreamBuffer
{

public:
	CStreamBuffer()
	{
		m_nROffset = 0;
		m_nWOffset = 0;
		m_Capacity = 0;
		m_pBuffer  = NULL;
	}

	CStreamBuffer(long bufferSize)
	{
		m_nROffset = 0;
		m_nWOffset = 0;
		m_Capacity = 0;
		m_pBuffer  = NULL;

		Create(bufferSize);
	}

	virtual ~CStreamBuffer()
	{
		Cleanup();
	}

	void Cleanup()
	{
		if (m_pBuffer != NULL)
		{
			delete [] m_pBuffer;
			m_pBuffer = NULL;
		}

		m_nROffset = 0;
		m_nWOffset = 0;
		m_Capacity = 0;
		m_pBuffer  = NULL;
	}

protected:
	BYTE*	m_pBuffer;
	long	m_nROffset;
	long	m_nWOffset;
	long	m_Capacity;

public:
	void	Create(long Buffer_Size)
	{
		if (m_Capacity < Buffer_Size)
		{
			SAFE_DELVEC(m_pBuffer);

			m_pBuffer  = new BYTE[Buffer_Size];
			m_Capacity = Buffer_Size;
		}
		
		_ASSERT(m_pBuffer != NULL);
		
		Reset();
	}

	void	WriteBytes(void* pBuf, WORD nLen)
	{
		_ASSERT(m_pBuffer != NULL);

		if ((m_nWOffset + nLen) > m_Capacity)	
		{
			_ASSERT(FALSE);
			throw 0;
			return;
		}

		::memcpy(&m_pBuffer[m_nWOffset], pBuf, nLen);

		m_nWOffset += nLen;
	}

	void	ReadBytes(void* pBuf, WORD nLen)
	{
		if ((m_nROffset + nLen) > m_nWOffset)
		{
			_ASSERT(FALSE);
		}

		::memcpy(pBuf, &m_pBuffer[m_nROffset], nLen);

		m_nROffset += nLen;
	}

	int		GetCapacity() { return m_Capacity; }
	int		GetWrPos() { return m_nWOffset; }
	int		GetRdPos() { return m_nROffset; } 
	void	SetDataSizeAtHeader(WORD){}
	long	GetHeaderSize() { return 0; }

	void	Reset() { m_nROffset = m_nWOffset = 0; }
	long	GetDataSize() { return m_nWOffset; }
	void	SetWrPos(int WrPos)
	{
		_ASSERT(WrPos >= 0 && WrPos <= m_Capacity);
		m_nWOffset = WrPos;
	}

	void	SetRdPos(int RdPos)	{ m_nROffset = RdPos; }
	long	GetAvailableBufSize() { return (m_Capacity - m_nWOffset); }
	BYTE*	GetBuffer()	{ return m_pBuffer; }
	BYTE*	GetWrBuffer() { return &m_pBuffer[m_nWOffset]; }
	BYTE*	GetBufferAt(long nOffset)
	{
		_ASSERT(nOffset >= 0 && nOffset < m_Capacity);

		return (BYTE*)&m_pBuffer[nOffset];
	}

public:
	template <class T>
	CStreamBuffer& operator << (const T& arg)
	{
		WriteBytes((void*)&arg, sizeof(T));
		return *this;
	}

	template <class T>
	CStreamBuffer& operator >> (T& arg)
	{
		ReadBytes((void*)&arg, sizeof(T));
		return *this;
	}

	template <>
	CStreamBuffer& operator << (const std::string& str)
	{
		WORD wLen = (WORD)str.length();

		WriteBytes(&wLen, sizeof(WORD));

		if (wLen < 1) // ÀÌ°­Èñ ÇäÇäÇäÇä
			return *this;

		WriteBytes((void*)str.c_str(), wLen);
		return *this;
	}

	template <>
	CStreamBuffer& operator >> (std::string& strString)
	{
		WORD wLen = 0;
		ReadBytes(&wLen, sizeof(WORD));

		if (wLen > 0)
		{	
			strString.resize(wLen);
			ReadBytes((void*)strString.c_str(), wLen);
		}
		else
		{
			strString.clear();
		}
		return *this;
	}
};

///////////////////////////////////////////////////
// CBufferProxy - CMsg, CStreamBuffer µÎ °´Ã¼¸¦ Ãß»óÈ­ ½ÃÅ²´Ù
///////////////////////////////////////////////////
class CBufferProxy
{
public:
	CBufferProxy()
	{
		Reset();
	}

	template <class T>
	CBufferProxy(T* org)
	{
		Delegate(org);
	}

	~CBufferProxy()
	{
		// ÀÌ°Ç Restore()°¡ ºÒ¸®Áö ¾Ê¾Ò´Ù´Â ¾ê±â³×!
		_ASSERT(m_pBuffer == NULL);
	}
	
protected:
	int		m_nROffset;
	int		m_nWOffset;
	int		m_nPeekPos;
	int		m_Capacity;
	BYTE*	m_pBuffer;

	void*	m_pOrg;

public:
	void Reset()
	{
		m_pOrg		= NULL;
		m_nWOffset	= 0;
		m_nROffset	= 0;
		m_nPeekPos	= 0;
		m_Capacity	= 0;
		m_pBuffer	= 0;
	}

	////////////////////////////////////////// delegation
	template <class T>
	void Delegate(T* org)
	{
		m_pOrg		= org;
		m_nWOffset	= org->GetWrPos();
		m_nROffset	= org->GetRdPos();
		m_nPeekPos	= 0;
		
		// ¿Ö ÀÌ·¸°Ô ÇßÀ»±î? ¹ÌÃÆ¾ú³ª? -_-a
		//m_Capacity	= org->GetAvailableBufSize();
		m_Capacity  = org->GetCapacity();
		m_pBuffer	= org->GetBufferAt(0);
	}

	template <class T>
	void Restore(T* org)
	{
		if (m_nROffset != org->GetRdPos())
			org->SetRdPos(m_nROffset);
		
		if (m_nWOffset != org->GetWrPos())
		{
			org->SetWrPos(m_nWOffset);
			org->SetDataSizeAtHeader(m_nWOffset - MSG_HEADER_SIZE);
		}
		Reset();
	}

	////////////////////////////////////////// streaming operators
	void	WriteBytes(void* pBuf, WORD nLen)
	{
		_ASSERT(m_pBuffer != NULL);

		if ((m_nWOffset + nLen) > m_Capacity)	
		{
			_ASSERT(FALSE);
			throw 0;
			return;
		}

		::memcpy(&m_pBuffer[m_nWOffset], pBuf, nLen);

		m_nWOffset += nLen;
	}

	void	ReadBytes(void* pBuf, WORD nLen)
	{
		if ((m_nROffset + nLen) > m_nWOffset)
		{
			_ASSERT(FALSE);
		}
		
		::memcpy(pBuf, &m_pBuffer[m_nROffset], nLen);

		m_nROffset += nLen;
	}

	WORD GetWrPos() { return m_nWOffset; }
	WORD GetRdPos() { return m_nROffset; }
	int	 GetCapacity() { return m_Capacity; }

	template <class T>
	void OverWrite(T arg, long nPos)
	{
#ifdef _DEBUG
		if ((nPos + sizeof(T)) > (size_t)m_nWOffset)
			throw;
#endif	
		*((T*)(m_pBuffer + nPos)) = arg;
	}
	
	template <class T>
	CBufferProxy& operator << (T arg)
	{
		_ASSERT(m_pBuffer != NULL);

		if (int(m_nWOffset + sizeof(T)) > m_Capacity)	
		{
			_ASSERT(FALSE);
			throw 0;
			return *this;
		}

		*((T*)(&m_pBuffer[m_nWOffset])) = arg;

		m_nWOffset += sizeof(T);

		return *this;
	}

	CBufferProxy& operator << (LPCTSTR lpszString)
	{
		WORD wLen = lstrlen(lpszString);

		//_ASSERT((m_nWOffset - m_nROffset) > sizeof(wLen));

		*this << wLen;

		if (wLen < 1)
			return *this;

		WriteBytes((void*)lpszString, wLen);
		return *this;
	}

	CBufferProxy& operator << (const std::string& strString)
	{
		return *this << strString.c_str();
	}

	template <class T>
	CBufferProxy& operator >> (T& arg)
	{
		long nLen = sizeof(T);

		_ASSERT((m_nROffset + nLen) <= m_nWOffset);
		
		arg = *((T*)&m_pBuffer[m_nROffset]);
		m_nROffset += nLen;

		return *this;
	}

	CBufferProxy& operator >> (std::string& strString)
	{
		WORD wLen = 0;
		ReadBytes(&wLen, sizeof(WORD));
		if (wLen > 0)
		{
			_ASSERT((m_nROffset + wLen) <= m_nWOffset);

			strString.resize( wLen);
			ReadBytes( (void*)strString.c_str(), wLen);
		}
		return *this;
	}

	template <class T>
	void FromProxy(T** pMsg)
	{
		((T*)m_pOrg)->SetWrPos(m_nWOffset);
		*((T**)pMsg) = (T*)m_pOrg;
	}

	friend CBufferProxy& operator << (CBufferProxy& Buf, std::string& strString)
	{
		return (Buf << strString.c_str());
	}

	void PeekMsg(BOOL bPeek)
	{
		if (bPeek)
		{
			if (m_nPeekPos > 0)
			{
				_ASSERT(FALSE);
			}

			m_nPeekPos = m_nROffset;	
		}
		else
		{
			if (m_nPeekPos == 0)
				return;

			m_nROffset = m_nPeekPos;
			m_nPeekPos = 0;
		}
	}
};

