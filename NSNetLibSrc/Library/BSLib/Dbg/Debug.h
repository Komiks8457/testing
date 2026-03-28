#pragma once

#include "StackWalker.h"
#include "APIHook.h"

extern void PutLine(const char *filename,int linenumber,const TCHAR *format,...);
extern void Put(TCHAR* format,...);
extern void WriteToFile( const wchar_t* lpszFileName, const wchar_t* lpszString );
extern bool ReportLogToFile( LPCTSTR lpszFileName, LPCTSTR lpszFormat, ... );
//extern void BSMessageBox(TCHAR* lpszStr, ... );

class NSStackWalker : public StackWalker
{
public:
	NSStackWalker(const wchar_t* stack_path, const wchar_t* log_path);
	~NSStackWalker();

private:
	virtual void OnOutput(LPCSTR szText) override;
	virtual void OnDbgHelpErr(LPCSTR szFuncName, DWORD gle, DWORD64 addr) override;

	std::wstring stack_path_;
	std::wstring log_path_;
	bool is_error_;
	HANDLE file_;
};

class Trace
{
public:
	Trace(const std::wstring& func, unsigned long line, const std::wstring& code);

	virtual void Reset();
	virtual void BuildLog(std::wstring& log);

private:	
	std::wstring func_;
	unsigned long line_;
	std::wstring code_;
};

template<typename Var1>
class Trace1 : public Trace
{
public:
	Trace1(const std::wstring& func, unsigned long line, const std::wstring& code, Var1 var1) : Trace(func, line, code), var1_(var1) {}

	void BuildLog(std::wstring& log)
	{
		Trace::BuildLog(log);
		std::wstringstream stream;
		stream << L"[" << var1_ << L"]";
		log.append(stream.str());
	}

	Var1 var1() const { return var1_; }

private:
	Var1 var1_;
};

template<typename Var1, typename Var2>
class Trace2 : public Trace1<Var1>
{
public:
	Trace2(const std::wstring& func, unsigned long line, const std::wstring& code, Var1 var1, Var2 var2) : Trace1<Var1>(func, line, code, var1), var2_(var2) {}

	void BuildLog(std::wstring& log)
	{
		Trace1::BuildLog(log);
		std::wstringstream stream;
		stream << L"[" << var2_ << L"]";
		log.append(stream.str());
	}

	Var2 var2() const { return var2_; }

private:
	Var2 var2_;
};

extern Trace* GetLastTrace();
extern Trace* GetLastTrace(DWORD thread_id);
extern void GetLastTraceLog(DWORD thread_id, std::wstring& log);
extern void SetLastTrace(Trace& trace);
extern void SetLastTrace(DWORD thread_id, Trace& trace);
extern void ClearLastTrace();
extern void ClearLastTrace(DWORD thread_id);

#define TRACE_NAME(line) trace##line
#define TRACE_VAR(line) TRACE_NAME(line)

#define TRACE(code)																\
	static Trace TRACE_VAR(__LINE__)(__FUNCTIONW__, __LINE__, L#code);			\
	SetLastTrace(TRACE_VAR(__LINE__));											\
	code

#define TRACE1(class, code, var1)												\
	static class TRACE_VAR(__LINE__)(__FUNCTIONW__, __LINE__, L#code, var1);	\
	SetLastTrace(TRACE_VAR(__LINE__));											\
	code

#define TRACE2(class, code, var1, var2)												\
	static class TRACE_VAR(__LINE__)(__FUNCTIONW__, __LINE__, L#code, var1, var2);	\
	SetLastTrace(TRACE_VAR(__LINE__));												\
	code

#define BEGIN_TRACE ClearLastTrace();

typedef void (*PUTDUMP_CALLBACK)(TCHAR* str);
extern void SetPutDumpCallback(PUTDUMP_CALLBACK callback);
extern void PutDump(TCHAR *format,...);
extern void __stdcall SetLastErrorCode(long error_code);
extern long __stdcall GetLastErrorCode();
extern int GetContextInfo_MiniDump(unsigned int code, struct _EXCEPTION_POINTERS *pExceptionInfo);

#ifdef _DEBUG
#define DBG_Assert(expression) \
	if( !(expression))\
	{\
		PutLine( __FILE__, __LINE__, _T("Assertion Failed !!! : %s"), #expression);\
		DebugBreak();\
	}
#else
#define DBG_Assert( expression)
#endif


#ifdef _DEBUG
#define DBG_MEMRYBLOCK(pointer,size)	\
	if( !_CrtIsMemoryBlock((const void*)pointer, size, NULL, NULL, NULL ) )\
	{\
		PutLine( __FILE__, __LINE__, _T("Invalid memory block!!!"));\
		DebugBreak();\
	}		
#else
#define DBG_MEMRYBLOCK(pointer,size)		
#endif


#ifdef _DEBUG
#define DBG_VALIDPOINTER(pointer,size)	\
	if( !_CrtIsValidPointer((const void*)pointer, size, TRUE) )\
	{\
		PutLine( __FILE__, __LINE__, _T("Invalid pointer!!!"));\
		DebugBreak();\
	}		
#else
#define DBG_VALIDPOINTER(pointer,size)		
#endif

// message box여부
#define DEBUG_OPTION_ASSERT_DONOT_SHOW_MESSAGEBOX		(DWORD)0x0001	// advance(진행) 않함 무조건 cancel상태
#define DEBUG_OPTION_ASSERT_SHOW_MESSAGEBOX_OKCANCEL	(DWORD)0x0002
#define DEBUG_OPTION_ASSERT_SHOW_MESSAGEBOX_OK			(DWORD)0x0004

// 진행 또는 종료 방법
#define DEBUG_OPTION_ASSERT_ADVANCE_BREAK				(DWORD)0x0010	// debugger가 있을때만 가능
#define DEBUG_OPTION_ASSERT_ADVANCE_NORMAL				(DWORD)0x0020

#define DEBUG_OPTION_ASSERT_WRITE_MINIDUMP				(DWORD)0x0100
#define DEBUG_OPTION_ASSERT_CALL_CALLBACK				(DWORD)0x0200	// assert직전에 callback함수를 호출함
#define DEBUG_OPTION_ASSERT_CANCEL_EXIT					(DWORD)0x0400	// exit process호출

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define DEBUG_OPTION_DEFAULT_CLIENT_DEBUGGER_PRESENT		(DEBUG_OPTION_ASSERT_SHOW_MESSAGEBOX_OKCANCEL | DEBUG_OPTION_ASSERT_ADVANCE_BREAK | DEBUG_OPTION_ASSERT_CANCEL_EXIT)
#define DEBUG_OPTION_DEFAULT_CLIENT_STAND_ALONE				(DEBUG_OPTION_ASSERT_SHOW_MESSAGEBOX_OKCANCEL | DEBUG_OPTION_ASSERT_ADVANCE_NORMAL | DEBUG_OPTION_ASSERT_CANCEL_EXIT)

#define DEBUG_OPTION_DEFAULT_SERVER_DEBUGGER_PRESENT		(DEBUG_OPTION_ASSERT_SHOW_MESSAGEBOX_OKCANCEL | DEBUG_OPTION_ASSERT_ADVANCE_BREAK | DEBUG_OPTION_ASSERT_CANCEL_EXIT)
#define DEBUG_OPTION_DEFAULT_SERVER_STAND_ALONE				(DEBUG_OPTION_ASSERT_DONOT_SHOW_MESSAGEBOX | DEBUG_OPTION_ASSERT_WRITE_MINIDUMP)

typedef void (*ASSERT_CALLBACK)(int iLine,TCHAR* filename,TCHAR* expression);

extern void SetDebugOption(DWORD option_debugger_present,DWORD option_stand_alone);
extern void SetAssertCallback(ASSERT_CALLBACK callback);

extern bool ProcessAssertF(int iLine,TCHAR* filename,TCHAR* expression,...);
extern bool ProcessAssertLog(int line,TCHAR* filename,TCHAR* expression,...);

extern void RegisterUnhandledExceptionFilter();
extern void WriteMiniDump();
extern void SetDebugHelperHandlers();
extern void SetDumpPath(LPCWSTR path);
extern void TestDump(BYTE option);

/////////////////////////////////////////////////////////
// from jroot and 좀 바꿈
/////////////////////////////////////////////////////////
#undef ASSERT
#undef _ASSERT
#undef _ASSERTE
#undef ASSERTMSG
#define _ASSERT(cond)								if( !(cond) ) { if( !ProcessAssertLog(__LINE__, _T(__FILE__), _T(#cond))) __debugbreak();} 
#define _ASSERTE(cond)								if( !(cond) ) { if( !ProcessAssertLog(__LINE__, _T(__FILE__), _T(#cond))) __debugbreak();}
#define ASSERT(cond)								if( !(cond) ) { if( !ProcessAssertLog(__LINE__, _T(__FILE__), _T(#cond))) __debugbreak();}
#define _FORCEASSERT()								if( !ProcessAssertLog(__LINE__, __BS_FILE__, _T("ForceAssert"))) __asm { int 3 };
#define FORCEASSERT()								if( !ProcessAssertLog(__LINE__, __BS_FILE__, _T("ForceAssert"))) __asm { int 3 };
#define ASSERTA0(cond,msg)							if( !(cond) ) { if( !ProcessAssertLog(__LINE__, _T(__FILE__), msg)) __debugbreak(); }
#define ASSERTA1(cond,msg, a1)						if( !(cond) ) { if( !ProcessAssertLog(__LINE__, _T(__FILE__), msg, a1)) __debugbreak(); }
#define ASSERTA2(cond,msg, a1, a2)					if( !(cond) ) { if( !ProcessAssertLog(__LINE__, _T(__FILE__), msg, a1, a2)) __debugbreak(); }
#define ASSERTA3(cond,msg, a1, a2, a3)				if( !(cond) ) { if( !ProcessAssertLog(__LINE__, _T(__FILE__), msg, a1, a2, a3)) __debugbreak(); }
#define ASSERTA4(cond,msg, a1, a2, a3, a4)			if( !(cond) ) { if( !ProcessAssertLog(__LINE__, _T(__FILE__), msg, a1, a2, a3, a4)) __debugbreak(); }
#define ASSERTA5(cond,msg, a1, a2, a3, a4, a5)		if( !(cond) ) { if( !ProcessAssertLog(__LINE__, _T(__FILE__), msg, a1, a2, a3, a4, a5)) __debugbreak(); }
#define ASSERTA6(cond,msg, a1, a2, a3, a4, a5, a6)	if( !(cond) ) { if( !ProcessAssertLog(__LINE__, _T(__FILE__), msg, a1, a2, a3, a4, a5, a6)) __debugbreak(); }

#undef VERIFY
#undef VERIFYMSG
#define VERIFY(cond)										if( !(cond) ) {	if( !ProcessAssertLog(__LINE__, _T(__FILE__), _T(#cond))) __debugbreak();}
#define VERIFYA0(cond,msg)									if( !(cond) ) { if( !ProcessAssertLog(__LINE__, _T(__FILE__), msg)) __debugbreak(); }
#define VERIFYA1(cond,msg, a1)								if( !(cond) ) { if( !ProcessAssertLog(__LINE__, _T(__FILE__), msg, a1)) __debugbreak(); }
#define VERIFYA2(cond,msg, a1, a2)							if( !(cond) ) { if( !ProcessAssertLog(__LINE__, _T(__FILE__), msg, a1, a2)) __debugbreak(); }
#define VERIFYA3(cond,msg, a1, a2, a3)						if( !(cond) ) { if( !ProcessAssertLog(__LINE__, _T(__FILE__), msg, a1, a2, a3)) __debugbreak(); }
#define VERIFYA4(cond,msg, a1, a2, a3, a4)					if( !(cond) ) { if( !ProcessAssertLog(__LINE__, _T(__FILE__), msg, a1, a2, a3, a4)) __debugbreak(); }
#define VERIFYA5(cond,msg, a1, a2, a3, a4, a5)				if( !(cond) ) { if( !ProcessAssertLog(__LINE__, _T(__FILE__), msg, a1, a2, a3, a4, a5)) __debugbreak(); }
#define VERIFYA6(cond,msg, a1, a2, a3, a4, a5, a6)			if( !(cond) ) { if( !ProcessAssertLog(__LINE__, _T(__FILE__), msg, a1, a2, a3, a4, a5, a6)) __debugbreak(); }