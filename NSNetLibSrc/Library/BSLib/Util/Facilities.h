#pragma once

#define CTIME_ASSERT(expr, msg) \
	CompileTimeError<((expr) != 0)> BSERR__##msg;

template <class TO, class FROM>
TO safe_reinterpret_cast(FROM from)
{
	CTIME_ASSERT(sizeof(FROM) <= sizeof(TO), CANT_BE_CAST__DEST_TYPE_TOO_NARROW);
	return reinterpret_cast<TO>(from);
}

// #ifndef MAKEFOURCC
// #define MAKEFOURCC(ch0, ch1, ch2, ch3)                          \
//             ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |   \
//             ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24))
// #endif

#ifndef SAFE_DELETE
	#define SAFE_DELETE(x)	{ delete (x); (x) = NULL; }
#endif

#ifndef SAFE_DELVEC
	#define SAFE_DELVEC(x)	{ delete [] (x); (x) = NULL; }
#endif

#ifndef SAFE_RELEASE
	#define SAFE_RELEASE(x)	if ((x)) { (x)->Release(); (x) = NULL; }
#endif

#define MAKE_LONGLONG(a, b) ((LONGLONG)(((DWORD)((LONGLONG)(a) & 0x00000000ffffffff)) | ((LONGLONG)((DWORD)((LONGLONG)(b) & 0x00000000ffffffff))) << 32))

///////////////////////////////////////
// CScheduledCallbacker
///////////////////////////////////////
template <class T, class _T_TIME = float>
class CScheduledCallbacker
{
	typedef void (T::*LPCALLBACK)();

	struct CallbackEntry
	{
		_T_TIME			Elapsed;
		_T_TIME			Timeout;
		T*				pInstance;
		LPCALLBACK		func;
	};

	typedef std::vector<CallbackEntry>	CALLBACKS;

public:
	CScheduledCallbacker() {}
	virtual ~CScheduledCallbacker() {}

protected:
	CALLBACKS	m_Entries;

public:
	void	Finalize()
	{
		m_Entries.clear();
	}

	void	RegisterCallback(T* pInstance, _T_TIME Timeout, LPCALLBACK lpFunc)
	{
		_ASSERT(Timeout > 0);

		CallbackEntry entry;
		entry.Elapsed   = 0;
		entry.Timeout   = Timeout;
		entry.pInstance = pInstance;
		entry.func		= lpFunc;

		m_Entries.push_back(entry);
	}

	void	Update(_T_TIME delta)
	{
		if (delta == 0)
			return;

		for (size_t i = 0; i < m_Entries.size(); ++i)
		{
			CallbackEntry& entry = m_Entries[i];
			
			entry.Elapsed += delta;

			if (entry.Elapsed >= entry.Timeout)
			{
				(*entry.pInstance.*entry.func)();
				entry.Elapsed -= entry.Timeout;
			}
		}
	}
};

///////////////////////////////////////
// CScheduledCallbacker
///////////////////////////////////////
template <class T, class _T_TIME = float>
class CScheduledCallbackerEx
{
	typedef int (T::*LPCALLBACK)( DWORD dwRegisterID );

	struct CallbackEntry
	{
		_T_TIME			Elapsed;
		_T_TIME			Timeout;
		T*				pInstance;
		LPCALLBACK		func;
	};

	typedef std::map<DWORD, CallbackEntry>	CALLBACKS;

public:
	CScheduledCallbackerEx() {}
	virtual ~CScheduledCallbackerEx() {}

protected:
	CALLBACKS	m_Entries;

public:
	void	Finalize()
	{
		m_Entries.clear();
	}

	void	RegisterCallback(DWORD dwRegisterID, T* pInstance, _T_TIME Timeout, LPCALLBACK lpFunc)
	{
		_ASSERT(Timeout > 0);

		CallbackEntry entry;
		entry.Elapsed   = 0;
		entry.Timeout   = Timeout;
		entry.pInstance = pInstance;
		entry.func		= lpFunc;

		m_Entries.insert(CALLBACKS::value_type(dwRegisterID, entry));
	}

	void	Update(_T_TIME delta)
	{
		if (delta == 0)
			return;

		CALLBACKS::iterator it = m_Entries.begin();
		CALLBACKS::iterator it_end = m_Entries.end();
		for( ; it != it_end ; )
		{
			CallbackEntry& entry = it->second;

			entry.Elapsed += delta;

			if (entry.Elapsed >= entry.Timeout)
			{
				int nResult = (*entry.pInstance.*entry.func)( it->first );
				if( nResult < 0 )
				{
					CALLBACKS::iterator it_temp = it;
					++it_temp;

					m_Entries.erase( it );

					it = it_temp;
					continue;
				}

				if (entry.Elapsed != 0)
					entry.Elapsed -= entry.Timeout;
			}

			++it;
		}
	}

	void	SetTimeout(DWORD dwRegisterID, _T_TIME Timeout)
	{
		CALLBACKS::iterator it = m_Entries.find(dwRegisterID);
		if (it == m_Entries.end())
			return;
		
		CallbackEntry& entry	= it->second;
		entry.Elapsed			= 0;
		entry.Timeout			= Timeout;
	}

	bool	IsExist( DWORD dwRegisterID )
	{
		CALLBACKS::iterator it = m_Entries.find(dwRegisterID);
		if (it != m_Entries.end())
			return true;

		return false;
	}
};

///////////////////////////////////////
// CAvgTimeChecker
///////////////////////////////////////
template < int MAX_SLOT_COUNT = 10 >
class CAvgTimeChecker
{
	enum { AVG_TIMER_SLOT_COUNT = MAX_SLOT_COUNT };

public:
	CAvgTimeChecker(DWORD accum = 100)
	{
		_ASSERT(accum > 0);
		
		m_dwMaxAccum = accum;
		
		m_nSlot = 0;
		m_prev_tick = 0;
		m_dwAccumed = 0;
		m_dwAccumCounter = 0;
		m_dwAvgTime = 0;

		::ZeroMemory(m_Averages, sizeof(m_Averages));
	}

protected:
	int			m_nSlot;
	ULONGLONG	m_prev_tick;
	DWORD		m_dwAccumed;
	DWORD		m_dwAccumCounter;
	DWORD		m_dwAvgTime;
	DWORD		m_dwMaxAccum;
	DWORD		m_Averages[AVG_TIMER_SLOT_COUNT];

public:
	float GetAvgTimeF()
	{
		return (m_dwAvgTime / 1000.0f);
	}

	int GetAvgTimeN()
	{
		return m_dwAvgTime;
	}

	void Begin()
	{
		m_prev_tick = GetTickCount64();
	}

	void End()
	{
		DWORD elapsed_tick = static_cast<DWORD>( GetTickCount64() - m_prev_tick );
		
		m_dwAccumed += elapsed_tick;
		++m_dwAccumCounter;
		
		if (m_dwAccumCounter >= m_dwMaxAccum)
		{
			++m_nSlot;
			m_nSlot %= AVG_TIMER_SLOT_COUNT;
			
			m_Averages[m_nSlot] = (m_dwAccumed / m_dwMaxAccum);

			DWORD dwTotal = 0;
			for (int i = 0; i < AVG_TIMER_SLOT_COUNT; ++i)
				dwTotal += m_Averages[i];

			m_dwAvgTime = (dwTotal / AVG_TIMER_SLOT_COUNT);

			// reset;
			m_dwAccumed = 0;
			m_dwAccumCounter = 0;
		}
	}
};

///////////////////////////////////////
// CAvgValue
///////////////////////////////////////
template < int MAX_SLOT_COUNT = 10 >
class CAvgValue
{
	enum { AVG_VALUE_SLOT_COUNT = MAX_SLOT_COUNT };

public:
	CAvgValue(int accum = 100) : m_nMaxAccum(accum), m_nAvgValue(0)
	{
		_ASSERT(accum > 0);

		Reset();
	}

protected:
	int	m_nSlot;
	int	m_nAccumed;
	int	m_nAccumCounter;
	int	m_nAvgValue;
	int	m_nMaxAccum;
	int	m_Averages[AVG_VALUE_SLOT_COUNT];

public:
	void Reset()
	{
		m_nSlot = 0;
		m_nAccumed = 0;
		m_nAccumCounter = 0;
		m_nAvgTime = 0;

		::ZeroMemory(m_Averages, sizeof(m_Averages));
	}

	int GetValue()
	{
		return m_nAvgValue;
	}

	void SetValue(int Value)
	{
		m_nAccumed += dwValue;
		++m_nAccumCounter;
		
		if (m_nAccumCounter >= m_nMaxAccum)
		{
			++m_nSlot;
			m_nSlot %= AVG_VALUE_SLOT_COUNT;
			
			m_Averages[m_nSlot] = (m_nAccumed / m_nMaxAccum);

			int nTotal = 0;
			for (int i = 0; i < AVG_VALUE_SLOT_COUNT; ++i)
				nTotal += m_Averages[i];

			m_nAvgValue = (nTotal / AVG_VALUE_SLOT_COUNT);

			// reset;
			m_nAccumed = 0;
			m_nAccumCounter = 0;
		}
	}
};

////////////////////////////////////////////////////////////
// std::tstring 을 Key로 하는 map 에서 사용하렴...
// hash_map 에서는 재미를 못볼 것이다...
// hash_map 같은 경우는 차라리 무조건 소문자로 넣고 소문자 비교로 가는 것이...
////////////////////////////////////////////////////////////
struct str_comp_no_case
{
	bool operator () (const std::string& a, const std::string& b) const
	{
		return (::_stricmp(a.c_str(), b.c_str()) != 0);
	}
};

struct wstr_comp_no_case
{
	bool operator () (const std::wstring& a, const std::wstring& b) const
	{
		return (::_wcsicmp(a.c_str(), b.c_str()) != 0);
	}
};


inline void StrLwr(std::string& out, LPCSTR pInput)
{
	out = pInput;
	std::transform(out.begin(), out.end(), out.begin(), tolower ); 
}

inline void StrLwr(std::wstring& out, LPCWSTR pInput)
{
	out = pInput;
	std::transform(out.begin(), out.end(), out.begin(), tolower ); 
}

/*
inline BOOL FindWord(std::tstring& strStringToCheck, vecString& FilterWords)
{
	std::tstring strTemp = strStringToCheck;
	::StrLwr((char*)strTemp.c_str());

	for (vecString::iterator it = FilterWords.begin(); it != FilterWords.end(); ++it)
	{
		std::tstring& strFilter = *it;
		
		if (strTemp.find(strFilter) != std::tstring::npos)
			return TRUE;
	}

	return FALSE;
}


*/