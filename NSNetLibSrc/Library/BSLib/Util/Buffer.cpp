#include "stdafx.h"
#include "Buffer.h"

#ifdef UNREAL_BUILD
#include "../../../../../Client/UnrealEngine3/Development/External/zlib/Inc/zlib.h"
#else
#include <zlib/zlib.h>
#endif

////////////////////////////////////////////////////////////////////////////////
// string trimmer
////////////////////////////////////////////////////////////////////////////////
BOOL IsSpace(TCHAR ch)
{
	switch (ch)
	{
	case (_T(' ')):
	case (_T('\t')):
	case (_T('\r')):
	case (_T('\n')):
		return TRUE;
	}
	return FALSE;
}

////////////////////////////////////////////////////////////////////////////////
// CBuffer implementation
CBuffer::CBuffer()
{
    m_pBufStart = m_abFixedBuf;
    m_pBufMax	= &m_abFixedBuf[128];
    m_pBufRdCur = m_pBufWrCur = m_pBufStart;
}

CBuffer::~CBuffer()
{
    if (m_pBufStart != m_abFixedBuf) 
		delete [] m_pBufStart;
}

void CBuffer::Reserve(const size_t nBytes)
{
    if (nBytes <= Capacity()) 
		return;

    LTrim();

    if (nBytes <= Capacity()) 
		return;

    size_t nNewBufSize = m_pBufMax - m_pBufStart;
    do 
    { 
        nNewBufSize += nNewBufSize / 2; 
    }
    while (nNewBufSize < m_pBufWrCur - m_pBufStart + nBytes);

    BYTE* pNewBufStart = new BYTE[nNewBufSize];
    if (pNewBufStart == NULL) 
		throw;

    ::memcpy(pNewBufStart, Data(), Size());
    
	if (m_pBufStart != m_abFixedBuf) 
		delete [] m_pBufStart;

    size_t nSize = Size();
    m_pBufStart = pNewBufStart;
    m_pBufMax = m_pBufStart + nNewBufSize;
    m_pBufRdCur = m_pBufStart;
    m_pBufWrCur = m_pBufStart + nSize;
}

void CBuffer::Clear()
{
    m_pBufWrCur = m_pBufRdCur = m_pBufStart;
}

void CBuffer::ReadBytes(void* pBuf, const size_t nBytes)
{
    if (nBytes > Size()) 
		throw

    ::memcpy(pBuf, Data(), nBytes);
    m_pBufRdCur += nBytes;
}

void CBuffer::WriteBytes(const void* pBuf, const size_t nBytes)
{
    Reserve(nBytes);
    ::memcpy(m_pBufWrCur, pBuf, nBytes);
    m_pBufWrCur += nBytes;
}

void CBuffer::LTrim()
{
    if (m_pBufStart != m_pBufRdCur)
    {
        ::memmove(m_pBufStart, m_pBufRdCur, m_pBufWrCur - m_pBufRdCur);
        m_pBufWrCur -= m_pBufRdCur - m_pBufStart;
        m_pBufRdCur = m_pBufStart;
    }
}

void CBuffer::RTrim()
{
    if (m_pBufStart == m_abFixedBuf || m_pBufWrCur == m_pBufMax) 
		return;

    size_t nNewBufSize = m_pBufWrCur - m_pBufStart;
    size_t nRdCur = GetRdCur();
    size_t nWrCur = GetWrCur();
    BYTE* pNewBufStart = NULL;
    
    if (nNewBufSize <= FIXED_BUFFER_SIZE)
    {
        pNewBufStart = m_abFixedBuf;
        nNewBufSize = FIXED_BUFFER_SIZE;
    }
    else 
        pNewBufStart = new BYTE[nNewBufSize];
    
    if (pNewBufStart == NULL) 
		return;

    ::memcpy(pNewBufStart, m_pBufStart, nNewBufSize);
    if (m_pBufStart != m_abFixedBuf) 
		delete [] m_pBufStart;

    m_pBufStart = pNewBufStart;
    m_pBufMax	= pNewBufStart + nNewBufSize;
    m_pBufRdCur = pNewBufStart + nRdCur;
    m_pBufWrCur = pNewBufStart + nWrCur;
}

void CBuffer::Trim()
{
    LTrim();
    RTrim();
}

#ifdef _DEBUG
#pragma comment(lib,"zlibd")
#else
#pragma comment(lib,"zlib")
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CBSCompressor::CBSCompressor()
{

}

CBSCompressor::~CBSCompressor()
{

}


///////////////////////////////////////
//Class Name	: CBSCompressor
//Function Name	: Encode
//Desc			: 메모리를 압축한다
//Parameta		:
//					const BYTE* pSrcBuffer		소스메모리
//					const DWORD dwValidBytes	사이즈
//					BYTE* pDestBuffer			압축 메모리 담길 메모리.
//					DWORD dwEncodedBytes		압축된 메모리의 사이즈
//return type	:  BOOL							성공 / 실패
///////////////////////////////////////////////////////
BOOL CBSCompressor::Encode(const BYTE* pSrcBuffer, const DWORD dwValidBytes, BYTE* pDestBuffer, DWORD& dwEncodedBytes)
{
	memcpy((void*)pDestBuffer,&dwValidBytes,4);

	int nResult = compress(pDestBuffer+4,&dwEncodedBytes,pSrcBuffer,dwValidBytes);
	if( nResult == Z_MEM_ERROR || nResult ==Z_BUF_ERROR )
	{
		dwEncodedBytes = 0;
		return FALSE;
	}

	return TRUE;
}

///////////////////////////////////////
//Class Name	: CBSCompressor
//Fucntion Name	: Decode
//Desc			: 압축을 해제한다
//Parameta		:
//					const BYTE* pSrcBuffer		소스메모리
//					const DWORD dwSrcSize		
//					BYTE* pDestBuffer			압축 메모리 담길 메모리.
//					DWORD dwBufSize				압축 풀 버퍼사이즈
//return values : BOOL							실패시 dwBufSize에 압축 해제되는 메모리의 사이즈를 리턴한다.
///////////////////////////////////////////////////////

BOOL CBSCompressor::Decode(const BYTE* pSrcBuffer,const DWORD dwSrcSize,BYTE* pDestBuffer, DWORD& dwBufSize)
{
	DWORD dwUnCompressedSize;
	memcpy(&dwUnCompressedSize,pSrcBuffer,4);

	if(dwBufSize < dwUnCompressedSize)
	{
		dwBufSize = dwUnCompressedSize;
		return FALSE;
	}

	int nResult = uncompress(pDestBuffer,&dwBufSize,pSrcBuffer+4,dwSrcSize);
	if(nResult == Z_MEM_ERROR || nResult == Z_BUF_ERROR || nResult == Z_DATA_ERROR)		
		return FALSE;

	return TRUE;
}
