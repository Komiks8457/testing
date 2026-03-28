#pragma once

#include "IQue.h"

#include "../thread/synchobject.h"
#include "../thread/scopedlock.h"

#define QUEUE_DEFAULT_SIZE			512
#define QUEUE_NO_LIMIT				-1

//////////////////////////////////////////
template <class T> 
class CQue  
{
public:
	typedef std::deque<T> QUE;

public:
	CQue(size_t nLimit = QUEUE_NO_LIMIT)	{ m_nLimit = nLimit; }
	virtual ~CQue() { m_nLimit = 0; }

protected:
	size_t	m_nLimit;
	QUE		m_Que;
	
public:
	size_t	GetSize() { return m_Que.size(); }
	BOOL	IsEmpty() { return m_Que.empty(); }
	
	BOOL IsFull()
	{
		if (m_nLimit == QUEUE_NO_LIMIT)
			return FALSE;

		return (m_Que.size() >= m_nLimit);
	}

	BOOL Enque(T Data)
	{
		if (m_nLimit != QUEUE_NO_LIMIT)
		{
			size_t nCurSize = m_Que.size();
			if (nCurSize >= (size_t)m_nLimit)
				return FALSE;
		}
		
		m_Que.push_back(Data);
		
		return TRUE;
	}

	bool Deque(T& Data)
	{
		if (m_Que.empty() == true)
		{
			Data = NULL;
			return false;
		}

		Data = m_Que.front();
		m_Que.pop_front();
			
		return true;
	}
};

//////////////////////////////////////////////////////////////////////
//	Use RWLock

#define RW_WAIT_TIME	1
template <class T>
class CQueRW : public IQue<T>
{
	typedef std::deque<T> QUE;	

	public:
		CQueRW(long nMaxThreads = 1)
		{ 
			m_pLock				= NULL;
			m_bActivated		= TRUE;
			m_nLimit			= QUEUE_NO_LIMIT;
			m_nQueuedItemCount	= 0L;
			m_hWait				= NULL;

			BOOL	bResult		= Open(nMaxThreads);
			_ASSERT(bResult == TRUE);
		}
		virtual				~CQueRW(){ Close(); }

	protected:
		long			m_bActivated;
		long			m_nLimit;
		long			m_nQueuedItemCount;
		QUE				m_Que;	
		HANDLE			m_hWait;

		CRWMonitor*		m_pLock;		
		
	private:
		virtual BOOL		Open(long nMaxConcurrentThreads)
		{
			_ASSERT( m_pLock == NULL );			
			
			m_pLock	= new CRWMonitor(nMaxConcurrentThreads);
			m_hWait = ::CreateEvent(NULL, FALSE, FALSE, NULL);
			return TRUE;
		}

	public:
		virtual BOOL		Close()
		{
			::InterlockedExchange(&m_bActivated, FALSE);
			
			Deactivate();

			if (m_hWait)
			{
				CloseHandle(m_hWait);
				m_hWait = NULL;
			}
			if (m_pLock)	
			{
				delete m_pLock;
				m_pLock	= NULL;
			}
			return TRUE;
		}


		virtual size_t		GetSize()
		{
			long lCount = 0L;
			::InterlockedExchange(&lCount, m_nQueuedItemCount);
			return	(size_t)lCount;			
		}

		virtual BOOL		IsEmpty()
		{
			long lCount = 0L;
			::InterlockedExchange(&lCount, m_nQueuedItemCount);			
			return (lCount <= 0);
		}

		virtual BOOL		IsFull() 
		{			
			if (m_nLimit == QUEUE_NO_LIMIT)	
				return FALSE;

			long lCount = 0L;
			::InterlockedExchange(&lCount, m_nQueuedItemCount);
			return (lCount >= (size_t)m_nLimit);
		}


		virtual BOOL		Enque( T NewItem )
		{			
			if (!m_bActivated)	
				return TRUE;

			SCOPED_RW_LOCK(m_pLock, RW_WRITE);	
			
			m_Que.push_back(NewItem);
			
			::InterlockedIncrement(&m_nQueuedItemCount);			
			return TRUE;
		}


		virtual BOOL		Deque( T* QueuedItem, DWORD dwTimeout = INFINITE, BOOL bForCleanup = FALSE )
		{
			if (dwTimeout == INFINITE) 
				dwTimeout = 1;

			::WaitForSingleObject(m_hWait, dwTimeout);

			SCOPED_RW_LOCK(m_pLock, RW_READ);

			*QueuedItem = NULL;
			if (!m_bActivated)
			{
				if (bForCleanup == FALSE)						
					return FALSE;
				
				if (!m_Que.empty())
				{	
					*QueuedItem = m_Que.front();
					m_Que.pop_front();
						
					::InterlockedDecrement(&m_nQueuedItemCount);				
					return	TRUE;
				}
				else				
					return FALSE;				
			}
			else
			{					
				if (!m_Que.empty())
				{
					*QueuedItem = m_Que.front();
					m_Que.pop_front();				
										
					::InterlockedDecrement(&m_nQueuedItemCount);					
					return TRUE;
				}
				else				
					return TRUE;				
			}			
		}

		virtual BOOL		Deactivate()
		{
			::InterlockedExchange(&m_bActivated, FALSE);			
			return	TRUE;
		}


		virtual void		Activate()
		{			
			::InterlockedExchange(&m_bActivated, TRUE);			
		}

		virtual void		Dump() const
		{
		#ifdef _DEBUG
		//	PUT("CQueMT::dump");
		//	PUT("Deactivated: %d\nMax Concurrent ThreadNum: %d\nCur Waiting Threads: %d\nCur Queued Item Count: %d\n", 
		//		m_bActivated, 
		//		m_nMaxConcurrentThreads,
		//		m_nCurWaitingThreads,
		//		m_nQueuedItemCount);
		#endif
		}
};

template <class T> 
class CQueMT : public IQue<T>
{
public:
	class Items
	{
	public:
		typedef std::function<void(T entry)> Handler;

		Items(unsigned long capacity, Handler handler = nullptr) : capacity_(capacity), count_(0), entries_(new OVERLAPPED_ENTRY[capacity]), handler_(handler)
		{
			ZeroMemory(entries_, sizeof(OVERLAPPED_ENTRY) * capacity);
		}

		~Items()
		{
			delete [] entries_;
		}

		void Process()
		{
			_ASSERT(handler_ != nullptr);
			for (BYTE offset = 0; offset < count_; ++offset)
				handler_(reinterpret_cast<T>(entries_[offset].lpOverlapped));
		}

		T GetEntry(unsigned long offset)
		{
			if (offset >= count_)
				return nullptr;

			return reinterpret_cast<T*>(entries_[offset].lpOverlapped);
		}

		unsigned long count() const { return count_; }

	private:
		friend class CQueMT<T>;

		unsigned long capacity_;
		unsigned long count_;
		OVERLAPPED_ENTRY* entries_;
		Handler handler_;
	};

	CQueMT(BOOL bUseSpinLock = TRUE, DWORD dwSpinCount = DEFAULT_SPIN_COUNT) : m_CS(bUseSpinLock, dwSpinCount), m_nMaxConcurrentThreads(0)
	{
		m_nCurWaitingThreads	= 0;
		m_nQueuedItemCount		= 0;
		m_hIOCP					= INVALID_HANDLE_VALUE;
		m_bActivated			= TRUE;
	}
	virtual ~CQueMT() { Close(); }

protected:
	CCriticalSectionBS	m_CS;
	long	m_nMaxConcurrentThreads;
	long	m_nCurWaitingThreads;
	long	m_nQueuedItemCount;	
	long	m_bActivated;
	HANDLE	m_hIOCP;

public:
	bool Open(long nMaxConcurrentThreads = 0)
	{
		if (m_hIOCP == INVALID_HANDLE_VALUE)
		{
			m_hIOCP = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 1, nMaxConcurrentThreads);
			if (m_hIOCP == INVALID_HANDLE_VALUE)
				return false;

			m_nMaxConcurrentThreads = nMaxConcurrentThreads;
		}

		return true;
	}

	virtual BOOL Close()
	{
		SCOPED_LOCK_SINGLE(&m_CS);
		Deactivate();
		if (m_hIOCP)
		{
			::CloseHandle(m_hIOCP);
			m_hIOCP = NULL;
		}
		
		return TRUE;
	}

	virtual size_t GetSize()
	{
		// lock 걸지 않으려고 이런 수작을... 이게 더 좋긴 좋은걸까? ^^;
		long nItemCount = 0;
		::InterlockedExchange(&nItemCount, m_nQueuedItemCount);
		return (size_t)nItemCount;
	}

	virtual BOOL IsEmpty()
	{
		long nItemCount = 0;
		::InterlockedExchange(&nItemCount, m_nQueuedItemCount);
		return (nItemCount == 0);
	}

	virtual BOOL IsFull() 
	{
		// 이놈은 만땅이 없기땜에...
		return FALSE; 
	}

	virtual BOOL Enque(T NewItem)
	{
		SCOPED_LOCK_SINGLE(&m_CS);		
		if (m_bActivated)
		{
			if (::PostQueuedCompletionStatus (m_hIOCP, sizeof(NewItem), m_bActivated,
			                                  reinterpret_cast<LPOVERLAPPED>(NewItem)))
			{				
				m_nQueuedItemCount++;
				return TRUE;
			}
		}
		
		return FALSE;
	}

	BOOL Deque(Items& QueuedItem, DWORD dwTimeout = INFINITE, BOOL bForCleanup = FALSE)
	{
		/*
		{
			SCOPED_LOCK_SINGLE(&m_CS);
			if (!m_bActivated)	 
			{
				////////////////////////////////////////////////////////
				// socket죽일때 사용하던 sending que청소를 위해서... 
				// 그래야 들어있던 큐랑 메시지랑 또 쓰지...
				///////////////////////////////////////////////////////
				if (bForCleanup == FALSE) 
					return FALSE;				
			}
			else
				m_nCurWaitingThreads++;
		}

		BOOL ret = ::GetQueuedCompletionStatusEx (m_hIOCP,
			QueuedItem.entries_
			,QueuedItem.capacity_
			,&QueuedItem.count_
			,dwTimeout
			,FALSE);
		{
			SCOPED_LOCK_SINGLE(&m_CS);
			if (m_nCurWaitingThreads > 0)
				m_nCurWaitingThreads--;

			if (ret)
			{
				m_nQueuedItemCount -= QueuedItem.count_;

				if (m_bActivated)
					return TRUE;
				else
					return FALSE;
			}
		}
		*/
		return TRUE;
	}

	virtual BOOL Deque(T* QueuedItem, DWORD dwTimeout = INFINITE, BOOL bForCleanup = FALSE)
	{		
		{
			SCOPED_LOCK_SINGLE(&m_CS);
			if (!m_bActivated)	 
			{
				////////////////////////////////////////////////////////
				// socket죽일때 사용하던 sending que청소를 위해서... 
				// 그래야 들어있던 큐랑 메시지랑 또 쓰지...
				///////////////////////////////////////////////////////
				if (bForCleanup == FALSE) 
					return FALSE;				
			}
		    else
				m_nCurWaitingThreads++;
		}

		ULONG_PTR CompletionKey = 0;
		DWORD dwSize;
		
		BOOL ret = ::GetQueuedCompletionStatus (m_hIOCP,
												&dwSize,
												&CompletionKey,
												reinterpret_cast<LPOVERLAPPED*>(QueuedItem),
												dwTimeout);
		{
			SCOPED_LOCK_SINGLE(&m_CS);
			if (m_nCurWaitingThreads > 0)
				m_nCurWaitingThreads--;
			
			if (ret)
			{
				if( CompletionKey != 0 )
				{					
					m_nQueuedItemCount--;
					
					if (m_bActivated)
						return TRUE;
					else
						return FALSE;
				}
				else						// Deactivate() 땜에 빠져나왔다!
				{
					return FALSE;
				}
			}
		}
		return TRUE;
	}

	virtual BOOL Deactivate()
	{
		SCOPED_LOCK_SINGLE(&m_CS);

		if (!m_bActivated)
			return TRUE;

		m_bActivated = FALSE;

		for (long i = 0; i < m_nMaxConcurrentThreads; i++) 
			::PostQueuedCompletionStatus(m_hIOCP,
										 0,
										 NULL,
										 NULL);
		
		return TRUE;
	}

	virtual void Activate()
	{
		::InterlockedExchange(&m_bActivated, TRUE);
	}

	void SetMaxConcurrentThreads(long nMaxConcurrentThreads)
	{
		m_nMaxConcurrentThreads = nMaxConcurrentThreads;
	}

	virtual void Dump() const
	{
	#ifdef _DEBUG
	//	PUT("CQueMT::dump");
	//	PUT("Deactivated: %d\nMax Concurrent ThreadNum: %d\nCur Waiting Threads: %d\nCur Queued Item Count: %d\n", 
	//		m_bActivated, 
	//		m_nMaxConcurrentThreads,
	//		m_nCurWaitingThreads,
	//		m_nQueuedItemCount);
	#endif
	}
};

template <class T> 
class CQueMT9x : public IQue<T>
{
public:
	CQueMT9x(long nMaxThreads = 4, BOOL bUseSpinLock = TRUE, DWORD dwSpinCount = DEFAULT_SPIN_COUNT) : m_CS(bUseSpinLock, dwSpinCount)
	{
		m_bActivated = TRUE;
		m_hWait = NULL;
		m_nLimit = QUEUE_NO_LIMIT;

		BOOL resultValue = Open(0);
		if( resultValue == FALSE )
		{
			_ASSERT( false );
		}
	}
	virtual ~CQueMT9x() 
	{
		Close();
	}

protected:
	typedef std::queue<T>	QUE;
	CCriticalSectionBS	m_CS;
	HANDLE	m_hWait;
	QUE		m_Que;
	long	m_nLimit;
	long	m_bActivated;
			
private:
	BOOL Open(long nMaxConcurrentThreads) 
	{
		Close();
		
		m_hWait = ::CreateEvent(NULL, TRUE, FALSE, NULL);
		if (m_hWait)
			return TRUE;
		
		return FALSE;
	}
	
public:
	virtual BOOL Close() 
	{ 
		if (m_hWait)
		{
			::CloseHandle(m_hWait);
			m_hWait = NULL;
		}
		return TRUE; 
	}

	virtual size_t GetSize() 
	{ 
		SCOPED_LOCK_SINGLE(&m_CS);
		return m_Que.size(); 
	}

	virtual BOOL IsEmpty()
	{
		SCOPED_LOCK_SINGLE(&m_CS);
		return (m_Que.size() <= 0);
	}

	virtual BOOL IsFull() 
	{
		SCOPED_LOCK_SINGLE(&m_CS);
		if (m_nLimit == QUEUE_NO_LIMIT)
			return FALSE;

		return (m_Que.size() >= (size_t)m_nLimit);
	}

	virtual BOOL Enque(T NewItem)
	{
		SCOPED_LOCK_SINGLE(&m_CS);
		
		if (m_nLimit != QUEUE_NO_LIMIT)
		{
			size_t nCurSize = m_Que.size();
			if (nCurSize >= (size_t)m_nLimit)
				return FALSE;
		}
		
		m_Que.push(NewItem);
		
		if (m_Que.size() == 1)
			SetEvent(m_hWait);
		
		return TRUE;
	}

	virtual BOOL Deque(T* QueuedItem, DWORD dwTimeout = INFINITE, BOOL bForCleanup = FALSE)
	{
		*QueuedItem = NULL;

		if (::WaitForSingleObject(m_hWait, dwTimeout) == WAIT_OBJECT_0)
		{
			SCOPED_LOCK_SINGLE(&m_CS);
			
			size_t nItemNum = m_Que.size();

			if (!m_bActivated)
			{
				if (bForCleanup == FALSE)						
					return FALSE;
				else
				{
					if (nItemNum > 0)
					{
						*QueuedItem = m_Que.front();
						m_Que.pop();
					}
					
					return FALSE;
				}
			}
			else
			{
				if (nItemNum == 0)
					ResetEvent(m_hWait);
				else
				{
					if (nItemNum == 1)
						ResetEvent(m_hWait);

					*QueuedItem = m_Que.front();
					m_Que.pop();
				}

				return TRUE;
			}
		}

		return TRUE;
	}

	virtual BOOL Deactivate()
	{
		SCOPED_LOCK_SINGLE(&m_CS);
		m_bActivated = FALSE; 
		if (m_hWait)
			::SetEvent(m_hWait);

		return TRUE;
	}

	virtual void Activate()
	{
		SCOPED_LOCK_SINGLE(&m_CS);
		m_bActivated = TRUE;
	}

	virtual void Dump() const
	{
	#ifdef _DEBUG
	//	PUT("CQueMT::dump");
	//	PUT("Deactivated: %d\nMax Concurrent ThreadNum: %d\nCur Waiting Threads: %d\nCur Queued Item Count: %d\n", 
	//		m_bActivated, 
	//		m_nMaxConcurrentThreads,
	//		m_nCurWaitingThreads,
	//		m_nQueuedItemCount);
	#endif
	}
};
