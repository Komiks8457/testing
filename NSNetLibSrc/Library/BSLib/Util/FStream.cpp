#include "stdafx.h"
#include "FStream.h"

CFStream::CFStream()
{
	m_dwStartPos = 0;
	m_dwEndPos	 = 0;
	m_dwFileSize = 0;
	m_pStream	 = NULL;

	m_dwWrCur = 0;
	m_dwRdCur = 0;

	::ZeroMemory(m_strFileName, sizeof(m_strFileName));
}

CFStream::~CFStream()
{
	Close();
}

void CFStream::Close()
{
	if (m_pStream)
	{
		::fclose(m_pStream);
		m_pStream = NULL;
	}

	m_dwStartPos = 0;
	m_dwEndPos	 = 0;
	m_dwFileSize = 0;
	m_dwWrCur = 0;
	m_dwRdCur = 0;

	::ZeroMemory(m_strFileName, sizeof(m_strFileName));
}

BOOL CFStream::Rewind()
{
	if (!m_pStream)
		return FALSE;

	::fseek(m_pStream, 0, SEEK_SET);
	return TRUE;
}

BOOL CFStream::Open(LPCTSTR lpszFile, BOOL bForWrite)
{
	Close();
	
	TCHAR* pFlag;
	if (bForWrite)
		pFlag = _T("wb");
	else
		pFlag = _T("rb");

	if ( 0 != ::_tfopen_s( &m_pStream, lpszFile, pFlag) )
		return FALSE;

	if (SF_lstrcpy(m_strFileName, 256, lpszFile) != S_OK)
	{
		_ASSERT(FALSE);
		return FALSE;
	}
	
	m_dwStartPos  = ::ftell(m_pStream);

	if (!bForWrite)
	{
		::fseek(m_pStream, 0, SEEK_END);
		m_dwEndPos = ::ftell(m_pStream);
		m_dwFileSize = m_dwEndPos - m_dwStartPos;

		if (m_dwFileSize == 0)
		{
			::MessageBox(NULL, _T("오잉? 화일 사이즈가 0인데 열어서 뭐할라고... 뭐 그래도 읽을라고?"), _T("MathLib"), MB_OK);
			Close();
			return FALSE;
		}
	}

	::fseek(m_pStream, 0, SEEK_SET);
	return TRUE;
}

long CFStream::GetCurrentPos()
{
	if (m_pStream == NULL)
		return -1;

	 long nTemp = ::ftell(m_pStream);
	 return (nTemp - m_dwStartPos);
}

BOOL CFStream::Goto(int nOffset)
{
	if (m_pStream == NULL)
		return -1;

	return (::fseek(m_pStream, (m_dwStartPos + nOffset), SEEK_SET) == 0);
}

void CFStream::ReadBytes(void* pBuf, int nLen)
{
	if ((m_dwRdCur + nLen) > m_dwFileSize)
	{
		::MessageBox(NULL, _T("File 사이즈 초과해서 데이터 읽으실라구?"), _T("BSO"), MB_OK);
		return;
	}
	
	::fread(pBuf, nLen, 1, m_pStream);
	m_dwRdCur += nLen;
}

void CFStream::WriteBytes(void* pBuf, int nLen)
{
	::fwrite(pBuf, nLen, 1, m_pStream);
	m_dwWrCur += nLen;
	m_dwFileSize = m_dwWrCur;
}
