////////////////////////////////////////////////
// Programmed by 강문철
// Email: overdrv72@hotmail.com
// Description: Dynamic Creation과 RTTI위한 클래스 몇개...
//				음... 당연히 MFC랑 충돌은 없으니 걱정 마시길~
////////////////////////////////////////////////
#pragma once

inline BOOL IsValidAddress(const void* lp, UINT nBytes, BOOL bReadWrite = TRUE)
{
#ifdef _DEBUG
	return (lp != NULL && !IsBadReadPtr(lp, nBytes) && (!bReadWrite || !IsBadWritePtr((LPVOID)lp, nBytes)));
#else
	return true;
#endif
}

// Helper Macros
#define BS_RUNTIME_CLASS(class_name) ((CRuntime*)(&class_name::class##class_name))

#define BS_ASSERT_KINDOF(class_name, object)						\
		_ASSERT((object)->IsKindOf(BS_RUNTIME_CLASS(class_name)))

#define BS_DECLARE_DYNAMIC(class_name)								\
public:																\
	static const CRuntime class##class_name;						\
	virtual CRuntime* GetRuntimeClass() const;

#define BS_DECLARE_DYNCREATE(class_name)							\
	BS_DECLARE_DYNAMIC(class_name)									\
	static CBase* PASCAL CreateObject();							\
	static void   PASCAL DeleteObject(void* ptr);

#define BS_IMPLEMENT_RUNTIMECLASS(class_name, base_class_name, pfnNew, pfnDel)	\
		const CRuntime class_name::class##class_name = {				\
		_T(#class_name),													\
		sizeof(class class_name),										\
		pfnNew,															\
		pfnDel,															\
		BS_RUNTIME_CLASS(base_class_name)};								\
																		\
	CRuntime* class_name::GetRuntimeClass() const						\
		{ return BS_RUNTIME_CLASS(class_name); }

#define BS_IMPLEMENT_DYNAMIC(class_name, base_class_name)				\
	BS_IMPLEMENT_RUNTIMECLASS(class_name, base_class_name, NULL, NULL)

#define BS_IMPLEMENT_DYNCREATE(class_name, base_class_name)			\
	CBase* PASCAL class_name::CreateObject()						\
		{ return new class_name; }									\
	void   PASCAL class_name::DeleteObject(void* ptr)				\
		{ delete (class_name*)ptr; }								\
	BS_IMPLEMENT_RUNTIMECLASS(class_name, base_class_name, class_name::CreateObject, class_name::DeleteObject)

/////////////////////////////////////////////
// CRuntime (for Dynamic Creation)
/////////////////////////////////////////////
class CBase;

struct CRuntime
{
	////////////
	LPCTSTR		m_lpszClassName;
	int			m_nObjectSize;

	CBase*		(PASCAL* m_pfnCreateObject)();			// NULL => abstract class
	void		(PASCAL* m_pfnDeleteObject)(void* ptr); // NULL => abstract class
	CRuntime*	m_pBaseClass;

	////////////
	CBase*		CreateObject();
	void		DeleteObject(void* ptr);
	BOOL		IsDerivedFrom(const CRuntime* pBaseClass) const;
};

/////////////////////////////////////////////
// CBase
/////////////////////////////////////////////
class CBase
{
public:
	virtual CRuntime* GetRuntimeClass() const;
	virtual ~CBase() {};
		
protected:
	CBase() {};

public:
	BOOL IsKindOf(const CRuntime* pClass) const;

public:
	static const CRuntime classCBase;
};


/////////////////////////////////////////////
// CServiceObject
/////////////////////////////////////////////
class CEventHandler;
class CTask;

class CServiceObject : public CBase
{
protected:
	CServiceObject() : m_nCurActiveThreadNum(0), m_pOwner(nullptr), m_Param(NULL) {}

public:	
	static const CRuntime classCServiceObject;
	virtual CRuntime* GetRuntimeClass() const;

protected:
	long		m_nCurActiveThreadNum;
	CTask*		m_pOwner;
	ULONG_PTR	m_Param;

	CCriticalSectionBS	m_CS;

public:
	virtual bool	Init(CTask* pTask, ULONG_PTR Param = 0) { m_pOwner = pTask; m_Param = Param; return true; }	
	virtual bool	Activate(long ThreadNumToSpawn) { return true; }
	virtual int		DoWork(void* pArg, DWORD dwThreaID) = 0;
	virtual BOOL	EndWork() = 0;
	virtual CTask*	GetTask(){return ((m_pOwner!=NULL) ? m_pOwner : NULL);}
};

class CMsg;
class CServiceObjectSock : public CServiceObject
{
public:	
	BS_DECLARE_DYNAMIC(CServiceObjectSock)

public:
	virtual BOOL	RegisterHandle(HANDLE hObject, ULONG_PTR CompletionKey) = 0;
	virtual BOOL	CircleOfTheLife(CEventHandler* pHandler, DWORD dwData = 0) = 0;

	virtual IQue<CMsg*>* GetSendingQueue(){ return NULL; }
};

//////////////////////////////////////////
// CEventHandler
//////////////////////////////////////////
class CEventHandler
{
public:
	CEventHandler(void) {}
	virtual ~CEventHandler(void) {}

public:
	virtual BOOL	RegisterHandleTo(CServiceObject* pServiceObj, HANDLE hHandle) = 0;
	virtual long	HandleEvent(DWORD dwData, void* pContext) = 0;
	virtual void	Reset() = 0;
};

//////////////////////////////////////////
// CTransHandler
//////////////////////////////////////////
class CTransHandler : public CEventHandler
{
public:
	CTransHandler(void) {}
	virtual ~CTransHandler(void) {}

public:
	virtual BOOL	LetsRock(DWORD dwServiceObject) = 0;
	virtual long	TransHandleEvent(BYTE btChecker, void* pOwner, void* pDeliver) = 0;
	virtual DWORD	TransResultHandleEvent(DWORD dwSession,BYTE btChecker, void* pOwner, void* pDeliver) = 0;
};
