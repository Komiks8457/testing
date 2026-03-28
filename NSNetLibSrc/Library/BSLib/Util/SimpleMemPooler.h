#pragma once

// CChunkAllocatorST의 malloc/free 버전

template <class T>
class CChunkAllocatorST_malloc
{
public:
	CChunkAllocatorST_malloc(long nChunkSize = DEFAULT_CHUNK_SIZE)
	{ 
		m_nChunkSize = nChunkSize;		
	}
	~CChunkAllocatorST_malloc() 
	{ 
		Release(); 
	}

protected:
	typedef std::list<T*>	ITEMBLOCK;

	long			m_nChunkSize;
	CQue<T*>		m_FreeItemQueue;
	ITEMBLOCK		m_ItemChunkArray;

protected:
	BOOL AllocItemBlock()
	{
		if (!m_FreeItemQueue.IsEmpty())
		{
			Put(_T("아직 할당할 Item이 남았는데 왜그려? FreeItemNum: %d"), m_FreeItemQueue.GetSize());
			return FALSE;
		}		
		
		T* pItemBlock = (T*)malloc(sizeof(T) * m_nChunkSize);
		if (pItemBlock)
		{
			m_ItemChunkArray.push_back(pItemBlock);
			
			for (int i = 0; i < m_nChunkSize; i++)
				m_FreeItemQueue.Enque(&pItemBlock[i]);
			
			return TRUE;
		}
		
		// 아..........................................
		// 인클루드 꼬여서.. 하드코딩 한다.......... =_=;;;;;;;; by novice.
		PutLog( (DWORD)(((2 & 0xFF) << 24) | ((0 & 0xFF) << 16) | (1 & 0xFFFF)), _T("CChunkAllocatorST::AllocItemBlock() Failed") );
		return FALSE;
	}

	T* ThereIsNoFreeItem()
	{
		AllocItemBlock();
		return PopFreeItem();
	}

	T* PopFreeItem()
	{
		T* pT = NULL;
		m_FreeItemQueue.Deque(pT);

		return pT;
	}

public:
	T* NewItem()
	{
		T* pT = PopFreeItem();
		if (pT == NULL)
			pT = ThereIsNoFreeItem();

		return pT;
	}

	void FreeItem(T* pT)
	{
		if (!pT)
		{
			Put(_T("FreeItem()에서 지우려는 Item이 NULL이다."));
			_ASSERT(0);
			return;
		}

		m_FreeItemQueue.Enque(pT);
	}

	void Release()
	{
		ITEMBLOCK::iterator it = m_ItemChunkArray.begin();
		ITEMBLOCK::iterator it_end = m_ItemChunkArray.end();

		for (; it != it_end; ++it)
		{
			T* pBlock = *it;
			if (pBlock)
				free((void*)pBlock);
		}
		m_ItemChunkArray.clear();
	}
};

template <class T>
class SimpleMemPooler
{
public:
#ifdef MEMORY_PROFILER
	void* operator new(size_t size, const char* file, int line)
	{
		T* item = pool_.NewItem();
		mem_profiler.Add(item, size, file, line);
		return item;
	}
	void operator delete (void* ptr)
	{
		mem_profiler.Add(ptr);
		pool_.FreeItem((T*)ptr);
	}
#else
	void* operator new (size_t size)
	{
		return pool_.NewItem();
	}	
	void operator delete (void* ptr)
	{
		pool_.FreeItem(static_cast<T*>(ptr));
	}
#endif

private:
	static CChunkAllocatorST_malloc<T> pool_;
};

template <class T>
class SimpleMemPoolerMT
{
public:	
#ifdef MEMORY_PROFILER
	void* operator new(size_t size, const char* file, int line)
	{
		SCOPED_LOCK_SINGLE(&critical_section_);

		T* item = pool_.NewItem();
		mem_profiler.Add(item, size, file, line);
		return item;
	}
	void operator delete (void* ptr)
	{
		critical_section_.Lock();
		mem_profiler.Add(ptr);
		pool_.FreeItem((T*)ptr);
		critical_section_.Unlock();
	}
#else
	void* operator new (size_t size)
	{
		SCOPED_LOCK_SINGLE(&cs_);

		void* ptr = pool_.NewItem();

		if (ptr == nullptr)
		{
			ASSERT(FALSE);
			return nullptr;
		}

		return ptr;
	}	
	void operator delete (void* ptr)
	{
		cs_.Lock();
		pool_.FreeItem(static_cast<T*>(ptr));
		cs_.Unlock();
	}
#endif

private:
	static CCriticalSectionBS cs_;
	static CChunkAllocatorST_malloc<T> pool_;
};

template <class T>
CChunkAllocatorST_malloc<T> SimpleMemPooler<T>::pool_;

template <class T>
CChunkAllocatorST_malloc<T> SimpleMemPoolerMT<T>::pool_;

template <class T>
CCriticalSectionBS SimpleMemPoolerMT<T>::cs_;