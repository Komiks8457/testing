#pragma once

// base member initialize 할때 this넘긴다고 궁시렁 대는 거.... 
// IMPLEMENT_CLASS_LINK() <-- 아래에 보면 요거에서 쓰거덩...
   
#pragma warning(disable: 4355)	


#define CL_FLAG_ERASED	0x00000001


template <class _T>
struct __SCLStaticData;

template <class _T>
struct	_CL
{
	typedef _CL<_T>	iterator;

	__SCLStaticData<_T>* pStatic;

	_CL()
	{
		pStatic = NULL;
	}
	_CL(__SCLStaticData<_T>* p)
	{
		pStatic = p;
		pStatic->m_lpCLCur = pStatic->m_lpCLBegin;
		pStatic->m_dwFlag &= ~CL_FLAG_ERASED;
	}

	inline iterator& operator++()
	{
		if(pStatic->m_dwFlag & CL_FLAG_ERASED)
		{
			pStatic->m_dwFlag &= ~CL_FLAG_ERASED;
			return *this;
		}
		pStatic->m_lpCLCur = pStatic->m_lpCLCur->GetNext();
		return *this;
	}
	iterator operator ++(int)
	{
		if(pStatic->m_dwFlag & CL_FLAG_ERASED)
		{
			pStatic->m_dwFlag &= ~CL_FLAG_ERASED;
			return *this;
		}
		pStatic->m_lpCLCur = pStatic->m_lpCLCur->GetNext();
		return *this;
	}
	inline _T* operator*() 
	{ 
		return pStatic->m_lpCLCur->GetObj();
	}
	inline BOOL	IsData() { return pStatic->m_lpCLCur ? TRUE : FALSE; }
	inline BOOL	IsEnd() { return pStatic->m_lpCLCur ? FALSE : TRUE; }
};


template <class _T>
class CClassLink;

template <class _T>
struct __SCLStaticData
{
	BOOL			m_blAutoLink;
	LONG			m_lnCnt;
	CClassLink<_T>*	m_lpCLBegin;
	CClassLink<_T>*	m_lpCLEnd;
	CClassLink<_T>*	m_lpCLCur;
	DWORD			m_dwFlag;

	void			Init()
	{
		m_lnCnt = 0;
		m_lpCLBegin = m_lpCLEnd = m_lpCLCur = NULL;
		m_dwFlag = 0;
	}
};


#define CLASSLINK_BEGIN(CN)		\
typedef CN typeClassLink; \
static	__SCLStaticData<typeClassLink> m_sCLBegin;

#define DECLARE_CLASS_LINK(CN)		\
public:					\
	CLASSLINK_BEGIN(CN)	\
	CClassLink<CN>	m_ClassLink;

#define IMPLEMENT_CLASSLINK(CN, AutoLink)	\
	__SCLStaticData<CN>  CN::m_sCLBegin = { AutoLink, 0, NULL, NULL, NULL, 0 };


#define GET_CLASSLINK_BEGIN(CN)	_CL<CN>(&CN::m_sCLBegin)
#define GET_CLASSLINK_CNT(CN)	CN::m_sCLBegin.m_lnCnt



template <class _T>
class	CClassLink
{
	BOOL		m_blLinked;
	CClassLink*	m_lpPre;
	CClassLink*	m_lpNext;

public:
	CClassLink()
	{
		m_blLinked = false;
		if(_T::m_sCLBegin.m_blAutoLink)
			Link();
	}
	virtual ~CClassLink()
	{
		Unlink();
	}
	inline BOOL	IsLinked() { return m_blLinked; }
	inline __SCLStaticData<_T>* GetBeginPtr() { return m_lpBeginPtr; }
	inline void SetBeginPtr(__SCLStaticData<_T>* p) { m_lpBeginPtr = p; }

	inline CClassLink*	GetPre() { return m_lpPre; }
	inline CClassLink*	GetNext() { return m_lpNext; }
	inline _T*			GetObj() { return (_T*)((ULONG_PTR)(this) - offsetof(_T,m_ClassLink)); }

	void	Link(BOOL blPushBack = TRUE,BOOL blForce = FALSE)
	{
		if(blForce == FALSE)
		{
			if(m_blLinked)
				return;
		}
		m_blLinked = TRUE;
		++_T::m_sCLBegin.m_lnCnt;
		if( _T::m_sCLBegin.m_lpCLBegin == NULL ) // Begin이 널이 아니면 당연히 End도 널이 아니다.
		{
			_T::m_sCLBegin.m_lpCLBegin = this;
			_T::m_sCLBegin.m_lpCLEnd = this;

			m_lpPre = NULL;
			m_lpNext = NULL;
		}
		else
		{	
			if(blPushBack)	// 맨뒤에다 넣는다.
			{
				CClassLink* pOldEnd = _T::m_sCLBegin.m_lpCLEnd;
				pOldEnd->m_lpNext = this;	// 이전의 End에 새로 추가된 오브젝트를 링크 시킨다.
				_T::m_sCLBegin.m_lpCLEnd = this;
				
				m_lpPre = pOldEnd;
				m_lpNext = NULL;
			}
			else			// 맨앞에다 넣는다.
			{
				CClassLink* pOldBegin = _T::m_sCLBegin.m_lpCLBegin;
				pOldBegin->m_lpPre = this;	// 이전의 End에 새로 추가된 오브젝트를 링크 시킨다.
				_T::m_sCLBegin.m_lpCLBegin = this;

				m_lpPre = NULL;
				m_lpNext = pOldBegin;
			}
		}
	}
	void	Unlink()
	{
		if(m_blLinked == FALSE)
			return;
		m_blLinked = FALSE;
		if(this == _T::m_sCLBegin.m_lpCLCur)
		{
			_T::m_sCLBegin.m_lpCLCur = m_lpNext;
			_T::m_sCLBegin.m_dwFlag |= CL_FLAG_ERASED;
		}
		ASSERT(_T::m_sCLBegin.m_lpCLBegin);
		--_T::m_sCLBegin.m_lnCnt;

		if( m_lpPre )
			m_lpPre->m_lpNext = m_lpNext;
		else
			_T::m_sCLBegin.m_lpCLBegin = m_lpNext;

		if( m_lpNext )
			m_lpNext->m_lpPre = m_lpPre;
		else
			_T::m_sCLBegin.m_lpCLEnd = m_lpPre;

		m_lpPre = m_lpNext = NULL;
	}
};

