#pragma once

////////////////////////////////////////////////////////////////////////////////
// getting conversion ACP
typedef UINT (*BSGETTHREADACP)();
extern BSGETTHREADACP g_pfnBSGetThreadACP;

inline UINT BSGetThreadACPFake()
{
    UINT nACP = 0;
    LCID lcidThread = ::GetThreadLocale();

    // GetLocaleInfoA will fail for a Unicode-only LCID,
    // but those are only supported on Windows 2000.
    // Since Windows 2000 supports CP_THREAD_ACP, this code path is never
    // executed on Windows 2000.
    char szACP[7];
    if (::GetLocaleInfoA(lcidThread, LOCALE_IDEFAULTANSICODEPAGE, szACP, 7) != 0)
    {
        char* pch = szACP;
        while (*pch != '\0')
        {
            nACP *= 10;
            nACP += *pch++ - '0';
        }
    }
	
    // use the default ANSI code page 
    // if we were unable to get the thread ACP or if one does not exist.
    if (nACP == 0) 
		nACP = ::GetACP();

    return nACP;
}

inline UINT BSGetThreadACPReal()
{
    return CP_THREAD_ACP;
}

inline UINT BSGetThreadACPThunk()
{
    OSVERSIONINFO ver;
    BSGETTHREADACP pfnBSGetThreadACP;

    ver.dwOSVersionInfoSize = sizeof(ver);
    ::GetVersionEx(&ver);
    if ((ver.dwPlatformId == VER_PLATFORM_WIN32_NT) && (ver.dwMajorVersion >= 5))
        pfnBSGetThreadACP = BSGetThreadACPReal;
    else
        pfnBSGetThreadACP = BSGetThreadACPFake;
    
    ::InterlockedExchange(
        reinterpret_cast<LONG*>(reinterpret_cast<void**>(&g_pfnBSGetThreadACP)),
        static_cast<LONG>(reinterpret_cast<LONG_PTR>(pfnBSGetThreadACP)));

    return g_pfnBSGetThreadACP();
}

__declspec(selectany) BSGETTHREADACP g_pfnBSGetThreadACP = BSGetThreadACPThunk;

////////////////////////////////////////////////////////////////////////////////
// conversion stack buffer
inline UINT BSGetConversionACP()
{
    return g_pfnBSGetThreadACP();
}

#if defined(_BS_DEBUG_)
    #define BS_USE_CONV \
            int _BS_convert = 0; _BS_convert; \
            unsigned int _BS_acp = BSGetConversionACP(); _BS_acp; \
            LPCWSTR _BS_lpw = NULL; _BS_lpw; \
            LPCTSTR _BS_lpa = NULL; _BS_lpa
#else
    #define BS_USE_CONV \
            int _BS_convert; _BS_convert; \
            unsigned int _BS_acp = BSGetConversionACP(); _BS_acp; \
            LPCWSTR _BS_lpw; _BS_lpw; \
            LPCTSTR _BS_lpa; _BS_lpa
#endif

////////////////////////////////////////////////////////////////////////////////
// global UNICODE, ANSI translation helpers
/*
inline LPWSTR BSA2WHelper(LPWSTR lpw, LPCTSTR lpa, int nChars, UINT acp)
{
    _ASSERT(lpa != NULL);
    _ASSERT(lpw != NULL);
    // verify that no illegal character present
    // since lpw was allocated based on the size of lpa
    // don't worry about the number of chars
    lpw[0] = '\0';
    MultiByteToWideChar(acp, 0, lpa, -1, lpw, nChars);
    return lpw;
}

inline LPSTR BSW2AHelper(LPSTR lpa, LPCWSTR lpw, int nChars, UINT acp)
{
    _ASSERT(lpw != NULL);
    _ASSERT(lpa != NULL);
    // verify that no illegal character present
    // since lpa was allocated based on the size of lpw
    // don't worry about the number of chars
    lpa[0] = '\0';
    WideCharToMultiByte(acp, 0, lpw, -1, lpa, nChars, NULL, NULL);
    return lpa;
}
*/
////////////////////////////////////////////////////////////////////////////////
// conversion macros
/*
#define BS_A2W(lpa) (\
        ((_BS_lpa = lpa) == NULL) ? NULL : (\
        _BS_convert = (lstrlenA(_BS_lpa)+1),\
		BSA2WHelper((LPWSTR) _alloca(_BS_convert*2), _BS_lpa, _BS_convert, _BS_acp)))

#define BS_A2CW(lpa) ((LPCWSTR) BS_A2W(lpa))

#define BS_W2A(lpw) (\
        ((_BS_lpw = lpw) == NULL) ? NULL : (\
        _BS_convert = (lstrlenW(_BS_lpw)+1)*2,\
        BSW2AHelper((LPSTR) _alloca(_BS_convert), _BS_lpw, _BS_convert, _BS_acp)))

#define BS_W2CA(lpw) ((LPCSTR) BS_W2A(lpw))

#if defined(_UNICODE)
    #define BS_T2A BS_W2A
    #define BS_A2T BS_A2W
    inline LPWSTR BS_T2W(LPTSTR lp) { return lp; }
    inline LPTSTR BS_W2T(LPWSTR lp) { return lp; }
    #define BS_T2CA BS_W2CA
    #define BS_A2CT BS_A2CW
    inline LPCWSTR BS_T2CW(LPCTSTR lp) { return lp; }
    inline LPCTSTR BS_W2CT(LPCWSTR lp) { return lp; }
#else
    #define BS_T2W BS_A2W
    #define BS_W2T BS_W2A
    inline LPSTR BS_T2A(LPTSTR lp) { return lp; }
    inline LPTSTR BS_A2T(LPSTR lp) { return lp; }
    #define BS_T2CW BS_A2CW
    #define BS_W2CT BS_W2CA
    inline LPCSTR BS_T2CA(LPCTSTR lp) { return lp; }
    inline LPCTSTR BS_A2CT(LPCSTR lp) { return lp; }
#endif
*/

////////////////////////////////////////////////////////////////////////////////
// format string utility function

inline void FormatString(LPTSTR lpszBuf, size_t nMaxChars, LPCTSTR lpszFormat, ...)
{
    va_list args;
    va_start(args, lpszFormat);
    SF_vsprintf(lpszBuf, nMaxChars, lpszFormat, args);
    va_end(args);
}

