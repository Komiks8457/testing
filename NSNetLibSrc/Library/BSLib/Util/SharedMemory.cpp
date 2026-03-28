#include "stdafx.h"
#include "SharedMemory.h"

CSharedMemory::CSharedMemory()
{
	ResetMemory();

	m_hMemoryMap = NULL;
	m_pMemoryMap = NULL;
}

CSharedMemory::~CSharedMemory()
{

}


BOOL CSharedMemory::OpenSharedMemory(std::tstring strShareName)
{
	m_hMemoryMap = NULL;
	m_pMemoryMap = NULL;

	m_hMemoryMap = ::CreateFileMapping(
		(HANDLE)0xffffffff, // 파일 맵의 핸들, 초기에 0xffffffff를 설정한다.
		NULL,        // 보안 속성
		PAGE_READWRITE,    // 읽고/쓰기 속성
		0,        // 64비트 어드레스를 사용한다. 상위 32비트 - 메모리의 크기
		MAX_MEM_BUFFER,// 하위 32비트 - 여기선LPBYTE 타입.
		strShareName.c_str());// 공유 파일맵의 이름 - Uique 해야한다.

	if(!m_hMemoryMap)
	{
		return FALSE;
	}

	m_pMemoryMap = (BYTE*)::MapViewOfFile(
		m_hMemoryMap,    // 파일맵의 핸들
		FILE_MAP_ALL_ACCESS,    // 액세스 모드 - 현재는 쓰기
		0,        // 메모리 시작번지부터의 이격된 상위 32비트 
		0,        // 메모리 시작번지부터의 이격된 하위 32비트
		0);        // 사용할 메모리 블록의 크기 - 0이면 설정한 전체 메모리

	if(!m_pMemoryMap) 
	{
		CloseHandle(m_hMemoryMap);
		return FALSE;
	}
	return TRUE;
}


void CSharedMemory::WriteBytes(void* pBuf, WORD nLen)
{
	memcpy(&m_szBuffer[m_dwWrCur],pBuf,nLen);
	m_dwWrCur += nLen;
}

void CSharedMemory::ResetMemory()
{
	m_dwWrCur = 0;
	ZeroMemory(m_szBuffer,MAX_MEM_BUFFER);
}

void CSharedMemory::FlushMemory()
{
	memcpy(m_pMemoryMap,m_szBuffer,m_dwWrCur);
}
