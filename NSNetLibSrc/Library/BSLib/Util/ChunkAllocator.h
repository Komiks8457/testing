////////////////////////////////////////////////
// Programmed by 강문철
// Email: overdrv72@hotmail.com
// Description: 메모리 fragmentation과
//				속도 저하를 막기위한
//				객체 Allocator + Containder
//				template이니깐 곳곳에 사용되겠당...
////////////////////////////////////////////////

#pragma once

#include "Que.h"

// BSLib가 아직 유니코드가 아니기땜시.. LPCTSTR을 하면 다른곳에 포함을 못시킨다 by novice
extern void PutLog(DWORD logtype, LPCTSTR foramt, ...);
#ifdef SERVER_BUILD
extern void PutLog(DWORD logtype, JSONValue& value);
#endif // #ifdef SERVER_BUILD

#ifdef _DEBUG
	#define DEFAULT_CHUNK_SIZE			50
#else
	#define DEFAULT_CHUNK_SIZE			500
#endif

///////////////////////////////////////
// for single thread
///////////////////////////////////////

// external 모듈을 위한 interface ㅡ,,ㅡ

/*
template <class T>
class __declspec( novtable) IChunkAllocator
{
public:
	virtual T* NewItem() = 0;
	virtual void FreeItem(T* pItem) = 0;
};
*/

#ifdef __TRACE_CHUNKALLOCATOR_USAGE__

struct __usage__
{
	__usage__()
	{
		pName = NULL;
		pAllocatedItemNum = NULL;
		pCapacity = NULL;
	}

	std::tstring* pName;
	DWORD*	pAllocatedItemNum;
	DWORD*	pCapacity;
};

typedef std::list<__usage__*>	POOL_USAGES;

class CChunkUsageMonitor
{
public:
	CChunkUsageMonitor()
	{

	}

	~CChunkUsageMonitor()
	{
		m_Usages.clear();
	}

protected:
	POOL_USAGES	m_Usages;
	
public:
	__usage__*	RegisterUsage(__usage__& u) 
	{ 
		__usage__* new_one = new __usage__;
		*new_one = u;

		if (::_stricmp(new_one->pName->c_str(), "Unknown") == 0)
			m_Usages.push_back(new_one);
		else
			m_Usages.push_front(new_one);

		return new_one;
	}

	void	RemoveUsage(__usage__* u)
	{
		POOL_USAGES::iterator it = std::find(m_Usages.begin(), m_Usages.end(), u);
		if (it != m_Usages.end())
		{
			delete (*it);
			m_Usages.erase(it);
		}
	}

	void	DumpUsage()
	{
		for (POOL_USAGES::iterator it = m_Usages.begin(); it != m_Usages.end(); ++it)
		{
			__usage__* u = (*it);

			if (u->pName == NULL)
				continue;

			PutLog((*(u->pAllocatedItemNum) > *(u->pCapacity)) ? LOG_FATAL : LOG_NOTIFY_FILE, _T("%S >> %d / %d"),
				u->pName->c_str(), *(u->pAllocatedItemNum), *(u->pCapacity));
		}
	}
};

extern CChunkUsageMonitor	g_ChunkUsageMonitor;

#endif

template<class Type>
class BlockTypeAllocator
{
public:
	static Type* Alloc(size_t count) { return new Type[count]; }
	static void Free(Type* ptr) { delete [] ptr; }
};

template<class Type>
class BlockSizeAllocator
{
public:
	static Type* Alloc(size_t count) { return static_cast<Type*>(malloc(sizeof(Type) * count)); }
	static void Free(Type* ptr) { free(ptr); }
};

template <class T, class BlockAllocator = BlockTypeAllocator<T>>
class ChunkAllocatorST
{
public:
	typedef void (T::*INIT_CALLBACK)(long);
	typedef void (*DUMP_CALLBACK)(DWORD Data1, DWORD Data2);

public:
	ChunkAllocatorST(long nChunkSize = DEFAULT_CHUNK_SIZE, INIT_CALLBACK lpInitializer = NULL, DWORD dwParam = 0, LPCTSTR lpszName = NULL)
	{ 
		m_nChunkSize = nChunkSize;		
		m_nAllocatedItemNum = 0;

		m_lpfInitializer = lpInitializer;
		m_dwParam = dwParam;

		m_lpfDumpCallback= NULL;
		
		m_nCapacity = 0;

#ifdef __TRACE_CHUNKALLOCATOR_USAGE__
		if (lpszName != NULL && lstrlenA(lpszName) > 0)
			m_strName.assign(lpszName);
		else
			m_strName = "Unknown";

		__usage__ u;
		u.pAllocatedItemNum = &m_nAllocatedItemNum;
		u.pCapacity = &m_nCapacity;
		u.pName = &m_strName;

		m_pUsage = g_ChunkUsageMonitor.RegisterUsage(u);
#endif

	}
	~ChunkAllocatorST() 
	{ 
#ifdef __TRACE_CHUNKALLOCATOR_USAGE__
		if (m_pUsage != NULL)
		{
			g_ChunkUsageMonitor.RemoveUsage(m_pUsage);
			m_pUsage = NULL;
		}
#endif
		Release(); 
	}

	void SetDumpCallbacks(DUMP_CALLBACK lpDumpCallback)
	{
		m_lpfDumpCallback = lpDumpCallback;
	}

protected:
	typedef std::list<T*>	ITEMBLOCK;
	typedef std::set<T*>	ITEMSET;
	
	INIT_CALLBACK 	m_lpfInitializer;
	DUMP_CALLBACK	m_lpfDumpCallback;
	
	long			m_nChunkSize;
	CQue<T*>		m_FreeItemQueue;
	ITEMBLOCK		m_ItemChunkArray;

	DWORD			m_dwParam;
	
	size_t			m_nAllocatedItemNum;
	size_t			m_nCapacity;

#ifdef __TRACE_CHUNKALLOCATOR_USAGE__
	std::tstring		m_strName;
	__usage__*		m_pUsage;
#endif

protected:
	BOOL AllocItemBlock()
	{
		if (!m_FreeItemQueue.IsEmpty())
		{
			return FALSE;
		}		
		
		T* pItemBlock = BlockAllocator::Alloc(m_nChunkSize);
		if (pItemBlock)
		{
			m_ItemChunkArray.push_back(pItemBlock);
			
			for (int i = 0; i < m_nChunkSize; i++)
				m_FreeItemQueue.Enque(&pItemBlock[i]);
			
			m_nCapacity = m_ItemChunkArray.size() * m_nChunkSize;

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

		if (pT != NULL && m_lpfInitializer != NULL)
			(pT->*m_lpfInitializer)(m_dwParam);

		return pT;
	}

public:
	T* NewItem()
	{
		T* pT = PopFreeItem();
		if (pT == NULL)
			pT = ThereIsNoFreeItem();

		++m_nAllocatedItemNum;

		return pT;
	}

	void FreeItem(T* pT)
	{
		if (pT == nullptr)
		{
			ASSERT(FALSE);
			return;
		}

		if (m_nAllocatedItemNum == 0)
		{
			std::string str(typeid(*pT).name());
			std::wstring wstr;
			wstr.assign(str.begin(), str.end());
			PutLog(LOG_FATAL_FILE, L"invalid free item - %s", wstr.c_str());
			return;
		}

		m_FreeItemQueue.Enque(pT);

		--m_nAllocatedItemNum;
	}

	int	 GetChunkSize()	{ return m_nChunkSize; }
	void SetChunkSize(long nChunkSize) { ::InterlockedExchange(&m_nChunkSize, nChunkSize); }
	int	 GetItemBlockNum() { return m_ItemChunkArray.size(); }
	int  GetFreeItemNum()  { return m_FreeItemQueue.GetSize(); }
	int	 GetAllocatedItemNum() { return m_nAllocatedItemNum; }

	void  GetUsage(size_t& nTotal,size_t& nUsed,size_t& nFree)
	{
		nTotal = m_nCapacity;
		nUsed = m_nAllocatedItemNum;
		_ASSERT( nUsed <= nTotal);
		nFree = nTotal - nUsed;
	}

	void Release()
	{
		ITEMBLOCK::iterator it = m_ItemChunkArray.begin();
		ITEMBLOCK::iterator it_end = m_ItemChunkArray.end();

		for (; it != it_end; ++it)
		{
			T* pBlock = *it;
			if (pBlock)
				BlockAllocator::Free(pBlock);
		}
		m_ItemChunkArray.clear();

//		ClearDataForDump();
	}

	//virtual void Dump(int count = -1) {}
};

///////////////////////////////////////
// multi-thread safe
///////////////////////////////////////
template <class T, class BlockAllocator = BlockTypeAllocator<T>>
class ChunkAllocatorMT : public ChunkAllocatorST<T, BlockAllocator>
{
public:
	ChunkAllocatorMT(long nChunkSize = DEFAULT_CHUNK_SIZE, INIT_CALLBACK lpInitializer = NULL, DWORD dwParam = 0, LPCTSTR lpszName = NULL)
		 : m_CS(TRUE), ChunkAllocatorST<T, BlockAllocator>(nChunkSize, lpInitializer, dwParam, lpszName) 
	{
	}
	~ChunkAllocatorMT(void) { Release(); }

protected:
	CCriticalSectionBS	m_CS;	

public:
	T* NewItem()
	{
		SCOPED_LOCK_SINGLE(&m_CS);
		return ChunkAllocatorST<T, BlockAllocator>::NewItem();
	}

	void FreeItem(T* pT)
	{
		SCOPED_LOCK_SINGLE(&m_CS);
		ChunkAllocatorST<T, BlockAllocator>::FreeItem(pT);
	}

	int	 GetItemBlockNum() 
	{ 
		SCOPED_LOCK_SINGLE(&m_CS); 
		return ChunkAllocatorST<T, BlockAllocator>::GetItemBlockNum(); 
	}
	
	int  GetFreeItemNum()  
	{ 
		SCOPED_LOCK_SINGLE(&m_CS); 
		return ChunkAllocatorST<T, BlockAllocator>::GetFreeItemNum(); 
	}

	void Release()
	{
		SCOPED_LOCK_SINGLE(&m_CS);
		ChunkAllocatorST<T, BlockAllocator>::Release();
	}

	void  GetUsage(size_t& nTotal,size_t& nUsed,size_t& nFree)
	{
		SCOPED_LOCK_SINGLE(&m_CS);
		ChunkAllocatorST<T, BlockAllocator>::GetUsage(nTotal, nUsed, nFree);
	}
};

#define DECLARE_OBJECT_POOL(Type, ChunkAllocator)												\
public:																							\
	void* operator new(size_t size)	{ return chunk_allocator_.NewItem(); }						\
	void operator delete (void* ptr) { chunk_allocator_.FreeItem(static_cast<Type*>(ptr)); }	\
	static void Release() { chunk_allocator_.Release(); }										\
private:																						\
	static ChunkAllocator<Type, BlockSizeAllocator<Type>> chunk_allocator_;

#define IMPLEMENT_OBJECT_POOL(Type, ChunkAllocator)												\
	ChunkAllocator<Type, BlockSizeAllocator<Type>> Type::chunk_allocator_;

#define IMPLEMENT_OBJECT_POOL_EX(Type, ChunkAllocator, ChunkSize)								\
	ChunkAllocator<Type, BlockSizeAllocator<Type>> Type::chunk_allocator_(ChunkSize);

#define DECLARE_POOL_ITEM(name)					\
	bool m_bValid;								\
	static name * New();						\
	static void Delete(name*& );				\
	virtual void DeleteMyself();				\
	void ReleaseInstanceData(){}

#define DECLARE_POOL_ITEM_BEGIN(name)			\
	bool m_bValid;								\
	static name * New();						\
	static void Delete(name*& );				\
	virtual void DeleteMyself();				\
	void ReleaseInstanceData(){						

#define DECLARE_POOL_ITEM_END(name)			}

#define IMPLEMENT_POOL_ITEM( name)											\
	ChunkAllocatorST< name >		g_Pool_##name(DEFAULT_CHUNK_SIZE, NULL, 0, _T(#name));		\
	name * name :: New() {													\
		name* pNewItem = g_Pool_##name.NewItem();							\
		pNewItem->m_bValid = true;											\
		return pNewItem;													\
	}																		\
	void name :: Delete( name *& pItem)										\
	{																		\
		if( pItem == NULL)													\
			return;															\
		if( pItem->m_bValid)												\
		{																	\
			pItem->m_bValid = false;										\
			pItem->ReleaseInstanceData();									\
			g_Pool_##name .FreeItem( pItem);								\
		}																	\
		pItem = NULL;														\
	}																		\
	void name::DeleteMyself()												\
	{																		\
		if(m_bValid)														\
		{																	\
			m_bValid = false;												\
			ReleaseInstanceData();											\
			g_Pool_##name.FreeItem((name*)this);							\
		}																	\
		else																\
		{																	\
			_ASSERT(FALSE);													\
		}																	\
	}

#define IMPLEMENT_POOL_ITEM_BEGIN( name)									\
	ChunkAllocatorST< name >		g_Pool_##name(DEFAULT_CHUNK_SIZE, NULL, 0, _T(#name));		\
	name * name :: New() {													\
			name * pNewItem = g_Pool_##name .NewItem();

#define IMPLEMENT_POOL_ITEM_END( name)										\
		pNewItem->m_bValid = true;											\
		return pNewItem;													\
	}																		

#define IMPLEMENT_POOL_ITEM_FUNC( name)										\
	void name::Delete( name *& pItem)										\
	{																		\
		if( pItem == NULL)													\
			return;															\
		if( pItem->m_bValid)												\
		{																	\
			pItem->m_bValid = false;										\
			pItem->ReleaseInstanceData();									\
			g_Pool_##name .FreeItem( pItem);								\
		}																	\
		pItem = NULL;														\
	}																		\
	void name::DeleteMyself()												\
	{																		\
		if(m_bValid)														\
		{																	\
			m_bValid = false;												\
			ReleaseInstanceData();											\
			g_Pool_##name.FreeItem((name*)this);							\
		}																	\
		else																\
		{																	\
			_ASSERT(FALSE);													\
		}																	\
	}


