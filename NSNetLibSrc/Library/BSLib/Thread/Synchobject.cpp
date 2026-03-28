#include "stdafx.h"
#include "SynchObject.h"
/*
extern "C" {

WINBASEAPI
BOOL
WINAPI
InitializeCriticalSectionAndSpinCount(
    IN OUT LPCRITICAL_SECTION lpCriticalSection,
    IN DWORD dwSpinCount
    );
}
*/
BOOL CSemaBS::Unlock()
{
	return Unlock(1, NULL); 
}

BOOL CSemaBS::Unlock(LONG lCount, LPLONG lpPrevCount /* =NULL */)
{
	return ::ReleaseSemaphore(m_hObject, lCount, lpPrevCount);
}

CSynchObject::CSynchObject(LPCTSTR pstrName)
{
	m_hObject = NULL;
	trace_ = false;
	
	if (pstrName)
		m_strName = pstrName;
}

CSynchObject::~CSynchObject()
{
	if (m_hObject != NULL)
	{
		::CloseHandle(m_hObject);
		m_hObject = NULL;
	}
}

BOOL CSynchObject::Lock(DWORD dwTimeout)
{
	if (::WaitForSingleObject(m_hObject, dwTimeout) == WAIT_OBJECT_0)
		return TRUE;
	else
		return FALSE;
}

BOOL CSynchObject::Unlock(LONG, LPLONG) 
{ 
	return TRUE; 
}

BOOL CSynchObject::IsCriticalSection()  
{ 
	return FALSE; 
}
LPCTSTR		 CSynchObject::GetName() 
{ 
	return (LPCTSTR)(m_strName.c_str()); 
}
	
CSynchObject::operator HANDLE() const { return m_hObject;}

CCriticalSectionBS::CCriticalSectionBS(BOOL bUseSpinCount, DWORD dwSpinCount) : CSynchObject(NULL)
{
	if (bUseSpinCount == TRUE)
		InitializeCriticalSectionAndSpinCount(&m_CS, dwSpinCount);
	else
		InitializeCriticalSection(&m_CS);
}
