////////////////////////////////////////////////
// Programmed by 강문철
// Email: overdrv72@hotmail.com
// Description: Mutex, Event, CriticalSection등과 같은
//				동기화관련 객체들의 Wrapper이다.
//				으음... 솔직히 이넘들은 
//				굳이 새로만들 필요도 없겠고 해서 MFC 소스를
//				홀랑 copy/paste했다... -_-;
////////////////////////////////////////////////
#ifndef __BS_SYNCHOBJECT_H__
#define __BS_SYNCHOBJECT_H__

namespace std { typedef basic_string<TCHAR> tstring; }

class CSynchObject
{
public:
	CSynchObject(LPCTSTR pstrName);
	virtual ~CSynchObject();

protected:
	HANDLE		m_hObject;
	std::tstring	m_strName;
	bool		trace_;
	
public:
	virtual BOOL Lock(DWORD dwTimeout = INFINITE);

	virtual BOOL Unlock() = 0;
	virtual BOOL Unlock(LONG, LPLONG);
	virtual BOOL IsCriticalSection();
	LPCTSTR		 GetName();

	bool trace() const { return trace_; }
	void set_trace(bool trace) { trace_ = trace; }
	void set_name(const std::tstring& name) { m_strName = name; }
		
	operator HANDLE() const;

	friend class CScopedLockSingle;
	friend class CScopedLockMulti;
};

///////////////////////////////////////////////////////
// CMutexBS
class CSemaBS : public CSynchObject
{
public:
	CSemaBS(LONG lInitialCount = 1, LONG lMaxCount = 1, LPCTSTR pstrName=NULL, LPSECURITY_ATTRIBUTES lpsaAttributes = NULL) : CSynchObject(pstrName)
	{
		_ASSERT(lMaxCount > 0);
		_ASSERT(lInitialCount <= lMaxCount);

		m_hObject = ::CreateSemaphore(lpsaAttributes, lInitialCount, lMaxCount, pstrName);
		_ASSERT(m_hObject != NULL);
	}
	virtual ~CSemaBS() {}

public:
	virtual BOOL Unlock();
	virtual BOOL Unlock(LONG lCount, LPLONG lprevCount = NULL);
};

///////////////////////////////////////////////////////
// CMutexBS

class CMutexBS : public CSynchObject
{
public:
	CMutexBS(LPSECURITY_ATTRIBUTES lpsaAttribute = NULL, BOOL bInitiallyState = FALSE, LPCTSTR lpszName = NULL) : CSynchObject(lpszName)
	{
		m_hObject = ::CreateMutex(lpsaAttribute, bInitiallyState, lpszName);
		_ASSERT(m_hObject != NULL);
	}

	virtual ~CMutexBS() {};

public:
	BOOL Unlock()
	{
		return ::ReleaseMutex(m_hObject);
	}

};

///////////////////////////////////////////////////////
// CEventBS

class CEventBS : public CSynchObject
{
public:
	CEventBS(LPSECURITY_ATTRIBUTES lpsaAttribute = NULL, BOOL bManualReset = TRUE, BOOL bInitiallyState = FALSE, LPCTSTR pstrName = NULL) : CSynchObject(pstrName)
	{
		m_hObject = ::CreateEvent(lpsaAttribute, bManualReset, bInitiallyState, pstrName);
		_ASSERT(m_hObject != NULL);
	}

	virtual ~CEventBS() {};

public:
	BOOL SetEvent()		{ _ASSERT(m_hObject != NULL); return ::SetEvent(m_hObject);   }
	BOOL PulseEvent()	{ _ASSERT(m_hObject != NULL); return ::PulseEvent(m_hObject); }
	BOOL ResetEvent()	{ _ASSERT(m_hObject != NULL); return ::ResetEvent(m_hObject); }
	BOOL Unlock()		{ return TRUE; }
};

///////////////////////////////////////////////////////
// CCriticalSectionBS
#define DEFAULT_SPIN_COUNT		4000

class CCriticalSectionBS : public CSynchObject
{
public:
	CCriticalSectionBS(BOOL bUseSpinCount = FALSE, DWORD dwSpinCount = DEFAULT_SPIN_COUNT);

	virtual ~CCriticalSectionBS()  
	{ 
		::DeleteCriticalSection(&m_CS); 	
	}

public:
	virtual BOOL IsCriticalSection()  { return TRUE; }

	operator CRITICAL_SECTION*() { return (CRITICAL_SECTION*) &m_CS; }
	CRITICAL_SECTION m_CS;

// Operations
public:
	BOOL Lock() 
	{ 
		::EnterCriticalSection(&m_CS);
		return TRUE; 
	}
	BOOL Lock(DWORD dwTimeout) { return Lock(); }
	BOOL Unlock() 
	{ 
		::LeaveCriticalSection(&m_CS); 		
		return TRUE; 
	}
};

class CSpinCriticalSectionBS : public CCriticalSectionBS
{
public:
	CSpinCriticalSectionBS(DWORD dwSpinCount = DEFAULT_SPIN_COUNT) : CCriticalSectionBS(TRUE, dwSpinCount)
	{
	}
};


///////////////////////////////////////////////////////////////////////////
//	RW-Lock( use mutext & semaphore )
class _condition;

class _monitor
{
protected:
	HANDLE				m_hMutex;

	virtual void		lock()    { WaitForSingleObject( m_hMutex, INFINITE ); }
	virtual void		release() { ReleaseMutex( m_hMutex ); }
	friend class		_condition;

public:
	_monitor() 
	{ 
		m_hMutex = CreateMutex( NULL, FALSE, NULL); 
	}
	virtual ~_monitor()
	{ 
		CloseHandle( m_hMutex ); 
	}
};




class _semaphore
{
protected:
	HANDLE				m_hSemaphore;   

public:
	_semaphore( int nSema = 1 ) 
	{ 
		m_hSemaphore = CreateSemaphore( NULL, 0, nSema, NULL ); 
	}
	~_semaphore() 
	{ 
		CloseHandle( m_hSemaphore ); 
	}

	void				P() { WaitForSingleObject( m_hSemaphore, INFINITE ); }
	BOOL				V() { return ReleaseSemaphore( m_hSemaphore, 1, NULL ); }
};



class _condition
{
protected:
	_semaphore		m_Semaphore;  
	long			m_nSemaCount;    
	_monitor*		m_Monitor;

public:
	_condition( _monitor* mon, int nSema = 1 ) : m_Semaphore( nSema )
	{ 
		m_Monitor		= mon;
		m_nSemaCount	= 0; 
	}
	~_condition() {};

	void wait() 
	{
		::InterlockedIncrement( &m_nSemaCount );			
		m_Monitor->release();
		m_Semaphore.P();
		m_Monitor->lock();
		::InterlockedDecrement( &m_nSemaCount );
	};

	void signal() 
	{		  
		if( m_nSemaCount > 0 )   
		{
			m_Semaphore.V();
		}
	};
};



class CRWMonitor : public _monitor
{
	protected:
		long			m_nReaderCount;  
		long			m_nWriterCount;  
		long			m_nWaitingReaders; 
		long			m_nWaitingWriters; 

		_condition*		m_ReadWaitQueue; 
		_condition*		m_WriteWaitQueue;  

	public:
		CRWMonitor(int nSema = 1)
		{
			m_ReadWaitQueue		= new _condition(this, nSema);
			m_WriteWaitQueue	= new _condition(this, nSema);

  
			m_nReaderCount		= m_nWriterCount 
								= 0;
			m_nWaitingReaders	= m_nWaitingWriters 
								= 0; 
		}

		~CRWMonitor()
		{
			delete m_ReadWaitQueue;
			delete m_WriteWaitQueue;
		}

		void			StartReading();
		void			StopReading();    
		void			StartWriting();
		void			StopWriting();    
};




inline void 
CRWMonitor::StartReading()
{
	lock(); 

	if( m_nReaderCount || m_nWaitingWriters )
	{
		::InterlockedIncrement( &m_nWaitingReaders );		
		m_ReadWaitQueue->wait();
		::InterlockedDecrement( &m_nWaitingReaders );		
	}
    ::InterlockedIncrement( &m_nReaderCount );	

	m_ReadWaitQueue->signal();  
	release();
}




inline void 
CRWMonitor::StopReading()
{
	lock();   
	::InterlockedDecrement( &m_nReaderCount );
	
	if( ( m_nReaderCount == 0 ) && m_nWaitingWriters )
		m_WriteWaitQueue->signal();
	else
		m_ReadWaitQueue->signal();  

	release();
}



inline void 
CRWMonitor::StartWriting()
{
	lock();

	if( m_nWriterCount || m_nReaderCount )
	{
		::InterlockedIncrement( &m_nWaitingWriters );		
		m_WriteWaitQueue->wait();
		::InterlockedDecrement( &m_nWaitingWriters );		
	}
	::InterlockedIncrement( &m_nWriterCount );	
  
	release();
}




inline void 
CRWMonitor::StopWriting()
{
	lock();

	::InterlockedDecrement( &m_nWriterCount );  
	if( m_nWaitingReaders )
		m_ReadWaitQueue->signal();
	else
		m_WriteWaitQueue->signal();
  
	release();
}

#endif __BS_SYNCHOBJECT_H__



