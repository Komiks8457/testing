////////////////////////////////////////////////
// Programmed by 강문철
// Email: overdrv72@hotmail.com
// Description: CSynchObject에서 상속받은 넘들을
//				이용해서 C++의 Scoped rule에 따라
//				Unlock이 자동으루 이뤄지라고 만든넘...
//				물론 Win32의 동기화 객체는 동일 
//				Thread의 중복 Lock에 의한 Deadlock이
//				발생하지는 않지만... 역시 Unlock은
//				정확히 호출해줘야 하지... 
//				그런점에서 아래의 넘들 사용하면
//				적어두 Unlock을 까먹을 염려는 없겠지...
////////////////////////////////////////////////

//#pragma once
#ifndef __SCOPEDLOCK_H__
#define __SCOPEDLOCK_H__

/////////////////////////////////////////////////////////////////////////////
// CScopedLockSingle
/////////////////////////////////////////////////////////////////////////////

#define SCOPED_LOCK_SINGLE(LOCK)											\
	CScopedLockSingle TempScopedLockS(LOCK, TRUE, __FUNCTIONW__, __LINE__);

class CScopedLockSingle
{
public:
	CScopedLockSingle(CSynchObject* pObject, BOOL bInitialLock, const wchar_t* file, int line);
	~CScopedLockSingle() { Unlock(); }

public:
	inline BOOL IsLocked() { return m_bAcquired; }
	inline BOOL Lock(DWORD dwTimeOut = INFINITE);
	inline BOOL Unlock();
	inline BOOL Unlock(LONG lCount, LPLONG lPrevCount = NULL);

protected:
	CSynchObject*	m_pObject;
	HANDLE			m_hObject;
	BOOL			m_bAcquired;
	const wchar_t* file_;
	int line_;
};

/////////////////////////////////////////////////////////////////////////////
// CScopedLockMulti
/////////////////////////////////////////////////////////////////////////////
#define SCOPED_LOCK_MULTI(LOCKs, COUNT, INITIAL_LOCK) CScopedLockMulti TempScopedLockS(LOCKs, COUNT, INITIAL_LOCK);		

#ifndef _countof
#define _countof(array) (sizeof(array)/sizeof(array[0]))
#endif

class CScopedLockMulti
{
public:
	inline CScopedLockMulti(CSynchObject* ppObjects[], DWORD dwCount, BOOL bInitialLock = FALSE);
	inline ~CScopedLockMulti();

public:
	inline DWORD	Lock(DWORD dwTimeOut = INFINITE, BOOL bWaitForAll = TRUE, DWORD dwWakeMask = 0);
	inline BOOL		Unlock();
	inline BOOL		Unlock(LONG lCount, LPLONG lPrevCount = NULL);
	inline BOOL		IsLocked(DWORD dwItem);

protected:
	HANDLE  m_hPreallocated[8];
	BOOL    m_bPreallocated[8];

	HANDLE* m_pHandleArray;
	BOOL*   m_bLockedArray;
	DWORD   m_dwCount;

	CSynchObject* const * m_ppObjectArray;
};

inline BOOL CScopedLockMulti::IsLocked(DWORD dwObject)
{
	_ASSERT((int)dwObject >= 0 && (int)dwObject < m_dwCount);
	return m_bLockedArray[dwObject];
}

#ifdef SERVER_BUILD
extern void PutLog(DWORD logtype, LPCTSTR foramt, ...);
#endif
/////////////////////////////////////////////////////////////////////////////
// CScopedLockSingle

inline CScopedLockSingle::CScopedLockSingle(CSynchObject* pObject, BOOL bInitialLock, const wchar_t* file, int line)
{
	_ASSERT(pObject != NULL);
	
	m_pObject	= pObject;
	m_hObject	= pObject->m_hObject;
	m_bAcquired = FALSE;
	file_ = file;
	line_ = line;

	if (bInitialLock)
		Lock();	
}

inline BOOL CScopedLockSingle::Lock(DWORD dwTimeOut)  // = INFINITE
{
	_ASSERT(m_pObject != NULL || m_hObject != NULL);
	_ASSERT(!m_bAcquired);

	m_bAcquired = m_pObject->Lock(dwTimeOut);

#ifdef SERVER_BUILD
	if (m_pObject->trace())
		PutLog(LOG_TRACE_FILE, L"Lock - name:%s, file:%s, line:%d", m_pObject->GetName(), file_, line_);
#endif

	return m_bAcquired;
}

inline BOOL CScopedLockSingle::Unlock()
{
#ifdef SERVER_BUILD
	if (m_pObject->trace())
		PutLog(LOG_TRACE_FILE, L"Unlock - name:%s, file:%s, line:%d", m_pObject->GetName(), file_, line_);
#endif

	_ASSERT(m_pObject != NULL);
	if (m_bAcquired)
		m_bAcquired = !m_pObject->Unlock();

	return !m_bAcquired;
}

inline BOOL CScopedLockSingle::Unlock(LONG lCount, LPLONG lpPrevCount)
{
	_ASSERT(m_pObject != NULL);
	if (m_bAcquired)
		m_bAcquired = !m_pObject->Unlock(lCount, lpPrevCount);

	return !m_bAcquired;
}

/////////////////////////////////////////////////////////////////////////////
// CScopedLockMulti

inline CScopedLockMulti::CScopedLockMulti(CSynchObject* pObjects[], DWORD dwCount, BOOL bInitialLock)
{
	_ASSERT(dwCount > 0 && dwCount <= MAXIMUM_WAIT_OBJECTS);
	_ASSERT(pObjects != NULL);

	m_ppObjectArray = pObjects;
	m_dwCount		= dwCount;

	// 미리 준비해놓은 배열로 수작이 되면 그넘들 쓴다. 안되면 new 해야지 머...
	if (m_dwCount > _countof(m_hPreallocated))
	{
		m_pHandleArray = new HANDLE[m_dwCount];
		m_bLockedArray = new BOOL[m_dwCount];
	}
	else
	{
		m_pHandleArray = m_hPreallocated;
		m_bLockedArray = m_bPreallocated;
	}

	for (DWORD i = 0; i < m_dwCount; i++)
	{
		_ASSERT(pObjects[i]);
		
		// CriticalSections 은 wait이 안된다... 알지?
		_ASSERT(pObjects[i]->IsCriticalSection() == FALSE);

		m_pHandleArray[i] = pObjects[i]->m_hObject;
		m_bLockedArray[i] = FALSE;
	}

	if (bInitialLock)
		Lock();
}

inline CScopedLockMulti::~CScopedLockMulti()
{
	Unlock();
	if (m_pHandleArray != m_hPreallocated)
	{
		delete [] m_bLockedArray;
		delete [] m_pHandleArray;
	}
}

inline DWORD CScopedLockMulti::Lock(DWORD dwTimeOut, BOOL bWaitForAll, DWORD dwWakeMask)
{
	DWORD dwResult = 0;
	if (dwWakeMask == 0)
		dwResult = ::WaitForMultipleObjects(m_dwCount, m_pHandleArray, bWaitForAll, dwTimeOut);
	else
		dwResult = ::MsgWaitForMultipleObjects(m_dwCount, m_pHandleArray, bWaitForAll, dwTimeOut, dwWakeMask);

	if (dwResult < (WAIT_OBJECT_0 + m_dwCount))
	{
		if (bWaitForAll)
		{
			for (DWORD i = 0; i < m_dwCount; i++)
				m_bLockedArray[i] = TRUE;
		}
		else
		{
			_ASSERT((int)(dwResult - WAIT_OBJECT_0) >= 0);
			m_bLockedArray[dwResult - WAIT_OBJECT_0] = TRUE;
		}
	}
	return dwResult;
}

inline BOOL CScopedLockMulti::Unlock()
{
	for (DWORD i=0; i < m_dwCount; i++)
	{
		if (m_bLockedArray[i])
			m_bLockedArray[i] = !m_ppObjectArray[i]->Unlock();
	}
	return TRUE;
}

inline BOOL CScopedLockMulti::Unlock(LONG lCount, LPLONG lpPrevCount)
{
	for (DWORD i=0; i < m_dwCount; i++)
	{
		if (m_bLockedArray[i])
			m_bLockedArray[i] = !m_ppObjectArray[i]->Unlock(lCount, lpPrevCount);
	}

	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CRWMonitor
/////////////////////////////////////////////////////////////////////////////
enum{ RW_READ, RW_WRITE };
#define SCOPED_RW_LOCK(LOCK,TYPE)				\
	CScopedRWLock TempRWLockS(LOCK, TYPE);\

class CScopedRWLock
{
public:
	CScopedRWLock(CRWMonitor* pObject,int nType = RW_READ);
	~CScopedRWLock() { Unlock(m_nType); }

public:	
	inline BOOL Lock(int nType);
	inline BOOL Unlock(int nType);	

protected:
	CRWMonitor*	m_pObject;	
	int			m_nType;
};

inline CScopedRWLock::CScopedRWLock(CRWMonitor* pObject, int nType)
{
	_ASSERT(pObject != NULL);	
	m_pObject	= pObject;	
	m_nType		= nType;
	Lock(m_nType);
}

inline BOOL CScopedRWLock::Lock(int nType)
{
	_ASSERT(m_pObject != NULL);	

	switch(nType)
	{
	case RW_READ:
		m_pObject->StartReading();
		return TRUE;
	case RW_WRITE:
		m_pObject->StartWriting();
		return TRUE;
	}
	return FALSE;
}

inline BOOL CScopedRWLock::Unlock(int nType)
{
	_ASSERT(m_pObject != NULL);
	switch(nType)
	{
	case RW_READ:
		m_pObject->StopReading();
		return TRUE;
	case RW_WRITE:
		m_pObject->StopWriting();
		return TRUE;
	}
	return FALSE;
}

#endif