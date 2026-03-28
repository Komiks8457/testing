#pragma once

template <class _T, BOOL _Cleanup = TRUE, class _Container = std::map<DWORD, _T*>, class itr = _Container::iterator >
class CObjMap
{
public:
	typedef	_Container		CONTYPE;
	typedef	_T				value_type;
	typedef itr				iterator;
	////////////////////////////////////////////////////////////////////
	// 2005는.. 이게 안된다..
	// 그래서 위에, 잘 보면.. _Container::iterator역할을 할 iterator 인자를 만들어버렸다
	//typedef	_Container::iterator	iterator;
	////////////////////////////////////////////////////////////////////

public:
	CObjMap() {}
	~CObjMap() 
	{ 
		if (_Cleanup) 
			Cleanup(); 
	}

protected:
	CONTYPE	m_Map;

public:
	DWORD Count() 
	{ 
		return m_Map.size(); 
	}

	BOOL DelObj(DWORD dwID)
	{
		CONTYPE::iterator it = m_Map.find(dwID);
		if (it != m_Map.end())
		{
			delete (_T*)((*it).second);
			m_Map.erase(it);
			return TRUE;
		}
		
		return FALSE;
	}

	CONTYPE& GetContainer()
	{
		return m_Map;
	}

	iterator LBound(DWORD dwKey) { return m_Map.lower_bound(dwKey); }
	iterator UBound(DWORD dwKey) { return m_Map.upper_bound(dwKey); }

	_T* GetObj(DWORD dwID)
	{
		CONTYPE::iterator it = m_Map.find(dwID);
		if (it != m_Map.end())
			return (*it).second;
		return NULL;
	}

	BOOL AddObj(DWORD dwID, _T* pObj)
	{
		if (GetObj(dwID) != NULL)
			return FALSE;

		m_Map.insert(CONTYPE::value_type(dwID, pObj));
		return TRUE;
	}

	_T* PopObj(DWORD dwID)
	{
		_T* pObj = NULL;
		CONTYPE::iterator it = m_Map.find(dwID);
		if (it != m_Map.end())
		{
			pObj =  (*it).second;
			m_Map.erase(it);
		}

		return pObj;
	}

	_T*	PopFront()
	{
		_T* pObj = NULL;
		CONTYPE::iterator it = m_Map.begin();
		if (it != m_Map.end())
		{
			pObj = (*it).second;
			m_Map.erase(it);
		}
		return pObj;
	}

	void Cleanup()
	{
		CONTYPE::iterator it = m_Map.begin();
		CONTYPE::iterator it_end = m_Map.end();

		for (; it != it_end; ++it)
			delete (*it).second;
		m_Map.clear();
	}

	iterator LockIterator(size_t& dwCount)
	{
		dwCount = m_Map.size();
		return m_Map.begin();
	}

	iterator Begin() { return m_Map.begin(); }
	iterator End() { return m_Map.end(); }
};
