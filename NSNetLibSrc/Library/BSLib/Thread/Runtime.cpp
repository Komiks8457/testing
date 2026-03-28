#include "stdafx.h"
#include "SynchObject.h"
#include "runtime.h"

/////////////////////////////////////////////////
// CRuntime
/////////////////////////////////////////////////
CBase* CRuntime::CreateObject()
{
	if (m_pfnCreateObject == NULL)
	{
		Put(_T("Error: Trying to create object which is not BS_DECLARE_DYNCREATE: %s"), m_lpszClassName);
		return NULL;
	}

	CBase* pBase = NULL;
	__try
	{
		pBase = (*m_pfnCreateObject)();
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		Put(_T("CRuntime::CreateObject() Something wrong!: %s"), m_lpszClassName);
		return NULL;
	}
	return pBase;
}

void CRuntime::DeleteObject(void* ptr)
{
	if (m_pfnDeleteObject == NULL)
	{
		Put(_T("Error: Trying to delete object which is not BS_DECLARE_DYNCREATE: %s"), m_lpszClassName);
		return;
	}

	__try
	{
		(*m_pfnDeleteObject)(ptr);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		Put(_T("CRuntime::DeleteObject() Something wrong!: %s"), m_lpszClassName);
	}
}

BOOL CRuntime::IsDerivedFrom(const CRuntime* pBaseClass) const
{
	_ASSERT(this != NULL);
	_ASSERT(pBaseClass != NULL);
	
	const CRuntime* pClassThis = this;
	while (pClassThis != NULL)
	{
		if (pClassThis == pBaseClass)
			return TRUE;
		
		pClassThis = pClassThis->m_pBaseClass;
	}
	return FALSE;
}


/////////////////////////////////////////////////
// CBase
/////////////////////////////////////////////////
const struct CRuntime CBase::classCBase = { _T("CBase"), sizeof(CBase), NULL, NULL, NULL };

CRuntime* CBase::GetRuntimeClass() const
{
	return BS_RUNTIME_CLASS(CBase);
}

BOOL CBase::IsKindOf(const CRuntime* pClass) const
{
	_ASSERT(this != NULL);
	_ASSERT(IsValidAddress(this, sizeof(CBase)));

	CRuntime* pClassThis = GetRuntimeClass();
	return pClassThis->IsDerivedFrom(pClass);
}

/////////////////////////////////////////////////
// CServiceObject
/////////////////////////////////////////////////
const struct CRuntime CServiceObject::classCServiceObject = { _T("CServiceObject"), sizeof(CServiceObject), NULL, NULL, NULL };

CRuntime* CServiceObject::GetRuntimeClass() const
{
	return BS_RUNTIME_CLASS(CServiceObject);
}

/////////////////////////////////////////////////
// CServiceObjectSock
/////////////////////////////////////////////////
const struct CRuntime CServiceObjectSock::classCServiceObjectSock = { _T("CServiceObjectSock"), sizeof(CServiceObjectSock), NULL, NULL, NULL };

CRuntime* CServiceObjectSock::GetRuntimeClass() const
{
	return BS_RUNTIME_CLASS(CServiceObjectSock);
}


