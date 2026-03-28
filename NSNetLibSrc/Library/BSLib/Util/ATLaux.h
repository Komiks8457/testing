/*///////////////////////////////////////////////////////////////////////
Win32 Callback thunking, the main idea was taken from ATL.
CAuxThunk<> is provided for __thiscall methods, CAuxStdThunk<> is for __stdcall.

*** EXAMPLE_CODE *** 
	class CHook: public CAuxThunk<CHook>
	{
	public:
	  CHook(): m_hhook(NULL)
	  {
		InitThunk((TMFP)CBTHook, this);
	  }

	  LRESULT CBTHook(int nCode, WPARAM wParam, LPARAM lParam);
	  BOOL Hook() 
	  {
		m_hook = SetWindowsHookEx(WH_CBT, (HOOKPROC)GetThunk(), NULL, GetCurrentThreadId());
		return (BOOL)m_hook;
	  }
	  HHOOK m_hhook;
	
	};

	LRESULT CHook::CBTHook(int nCode, WPARAM wParam, LPARAM lParam)
	{
	  if ( nCode == HCBT_CREATEWND ) {
		UnhookWindowsHookEx(m_hook);
		HWND hwnd = (HWND)wParam;
		// do whatever we want with HWND
		...
	  }
	  return CallNextHookEx(m_hook, nCode, wParam, lParam);
	}
*** END ***

*** EXAMPLE_CODE *** 
	struct TIMEOUT: CAuxThunk<TIMEOUT> 
	{
    UINT m_timerID;
	CONTEXT m_contex;

	TIMEOUT(CONTEXT& contex): m_contex(contex)
	{
		InitThunk((TMFP)TimerProc, this);
	}
	void TimerProc(HWND, UINT, UINT idEvent, DWORD dwTime);	
	};

	void TIMEOUT::TimerProc(HWND, UINT, UINT idEvent, DWORD dwTime)
	{
	  KillTimer(NULL, m_timerID); 
	  delete this;
	}
	HRESULT CSimpleObj::Post(CONTEXT& contex) {
	  TIMEOUT* pTimeout = new TIMEOUT(context);
	  pTimeout->m_timerID = ::SetTimer(NULL, 0, timeout, (TIMERPROC)pTimeout->GetThunk());
	}
*** END ***
//////////////////////////////////////////////////////////////////////////////*/

#pragma once

#ifndef _M_IX86
  #pragma message("CAuxThunk/CAuxStdThunk is implemented for X86 only!")
#endif

#pragma pack(push, 1)
template <class T>
class CAuxThunk
{
	BYTE		m_mov;          
	ULONG_PTR   m_this;         
	BYTE		m_jmp;          
	ULONG_PTR   m_relproc;

public:
	typedef void (T::*TMFP)();
	void InitThunk(TMFP method, const T* pThis)
	{
		union 
		{
			ULONG_PTR	func;
			TMFP		method;
		}addr;

		addr.method	= method;
		m_mov		= 0xB9;
		m_this		= (ULONG_PTR)pThis;
		m_jmp		= 0xE9;
		m_relproc	= addr.func - (ULONG_PTR)(this+1);

		FlushInstructionCache(GetCurrentProcess(), this, sizeof(*this));
	}
	FARPROC GetThunk() const 
	{
		_ASSERT(m_mov == 0xB9);
		return (FARPROC)this; 
	}
};

template <class T>
class CAuxStdThunk
{
	BYTE		m_mov;
	ULONG_PTR   m_this;
	ULONG_PTR	m_xchg_push;
	BYTE		m_jmp;
	ULONG_PTR   m_relproc;
public:
	typedef void (__stdcall T::*TMFP)();
	void InitThunk(TMFP method, LPVOID pThis)
	{
		union 
		{
			ULONG_PTR	func;
			TMFP		method;
		} addr;

		addr.method = method;
		m_mov		= 0xB8;
		m_this		= (ULONG_PTR)pThis;
		m_xchg_push = 0x50240487;
		m_jmp		= 0xE9;
		m_relproc	= addr.func - (ULONG_PTR)(this+1);

		FlushInstructionCache(GetCurrentProcess(), this, sizeof(*this));
	}
	FARPROC GetThunk() const 
	{
		_ASSERT(m_mov == 0xB8);
		return (FARPROC)this; 
	}
};

#pragma pack(pop)
