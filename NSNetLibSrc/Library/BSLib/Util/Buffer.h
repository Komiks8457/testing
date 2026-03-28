#ifndef __CSF_BUFFER_H__
#define __CSF_BUFFER_H__

////////////////////////////////////////////////////////////////////////////////
// CBuffer
class CBuffer
{
public:
    CBuffer();
	virtual ~CBuffer();

    inline const void* Data() const;
    inline size_t Size() const;
    inline bool IsEmpty() const;
    inline size_t Capacity() const;
    
    void Reserve(const size_t nBytes);
    void Clear();

    void ReadBytes(void* pBuf, const size_t nBytes);
    void WriteBytes(const void* pBuf, const size_t nBytes);

    void LTrim();
    void RTrim();
    void Trim();

protected:
    enum SeekPos { begin, current, end };

    inline size_t GetRdCur() const;
    inline void SeekRdCur(const size_t nPos);
    inline void SeekRdCur(const SeekPos pos, const long lOffset);

    inline size_t GetWrCur() const;
    inline void SeekWrCur(const size_t nPos);
    inline void SeekWrCur(const SeekPos pos, const long lOffset);

private:
    CBuffer(const CBuffer&);
    void operator =(const CBuffer&);

    BYTE* m_pBufStart;
    BYTE* m_pBufMax;
    BYTE* m_pBufRdCur;
    BYTE* m_pBufWrCur;

    enum { FIXED_BUFFER_SIZE = 128 };
    BYTE m_abFixedBuf[FIXED_BUFFER_SIZE];
};

inline const void* CBuffer::Data() const 
{
    return m_pBufRdCur;
}

inline size_t CBuffer::Size() const 
{
    return m_pBufWrCur - m_pBufRdCur;
}

inline size_t CBuffer::Capacity() const 
{
    return m_pBufMax - m_pBufWrCur;
}

inline bool CBuffer::IsEmpty() const 
{
    return m_pBufRdCur == m_pBufWrCur;
}

inline size_t CBuffer::GetRdCur() const 
{
    return m_pBufRdCur - m_pBufStart;
}

inline void CBuffer::SeekRdCur(const size_t nPos)
{
    if (m_pBufRdCur + nPos > m_pBufWrCur) 
		throw;

	m_pBufRdCur += nPos;
}

inline void CBuffer::SeekRdCur(const SeekPos pos, const long lOffset) 
{
    BYTE* pPosition;

    if (pos == CBuffer::begin) pPosition = m_pBufStart + lOffset;
    else if (pos == CBuffer::end) pPosition = m_pBufWrCur + lOffset;
    else pPosition = m_pBufRdCur + lOffset;

    if (pPosition < m_pBufStart || pPosition > m_pBufWrCur) 
		throw;

    m_pBufRdCur = pPosition;
}

inline size_t CBuffer::GetWrCur() const 
{
    return m_pBufWrCur - m_pBufStart;
}

inline void CBuffer::SeekWrCur(const size_t nPos)
{
    if (m_pBufWrCur + nPos > m_pBufMax) 
		throw;

    m_pBufWrCur += nPos;
}

inline void CBuffer::SeekWrCur(const SeekPos pos, const long lOffset)
{
    BYTE* pPosition;

    if (pos == CBuffer::begin) 
		pPosition = m_pBufRdCur + lOffset;
    else if (pos == CBuffer::end) 
		pPosition = m_pBufMax + lOffset;
    else 
		pPosition = m_pBufWrCur + lOffset;

    if (pPosition < m_pBufRdCur || pPosition > m_pBufMax) 
		throw;

    m_pBufWrCur = pPosition;
}


class CBSCompressor	  
{
public:
	CBSCompressor();
	~CBSCompressor();

public:
	BOOL Encode(const BYTE* pSrcBuffer,const DWORD dwValidBytes, BYTE* pDestBuffer, DWORD& dwEncodedBytes);
	BOOL Decode(const BYTE* pSrcBuffer,const DWORD dwSrcSize,BYTE* pDestBuffer, DWORD& dwBufSize);
};


#endif // __CSF_BUFFER_H__