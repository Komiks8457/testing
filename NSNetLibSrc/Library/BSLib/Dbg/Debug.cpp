#include "stdafx.h"
#include "debug.h"
#include "DbgHelp.h"
#include <signal.h>

#define MAX_DEBUG_MESSAGE_LEN		4096

typedef HASH_MAP<DWORD, Trace*&> TraceEntries;
TraceEntries g_trace_entries;

__declspec(thread) Trace* g_last_trace = nullptr;

Trace::Trace(const std::wstring& func, unsigned long line, const std::wstring& code) : func_(func), line_(line), code_(code)
{
	g_trace_entries.insert(TraceEntries::value_type(GetCurrentThreadId(), g_last_trace));
}

void Trace::Reset()
{
	func_.clear();
	line_ = 0;
	code_.clear();
}

void Trace::BuildLog(std::wstring& log)
{
	std::wstringstream stream;

	stream << "[" << func_ << "]";
	stream << "[" << line_ << "]";
	stream << "[" << code_ << "]";

	log = stream.str();
}

Trace* GetLastTrace()
{
	return GetLastTrace(GetCurrentThreadId());
}

Trace* GetLastTrace(DWORD thread_id)
{
	if (GetCurrentThreadId() == thread_id)
		return g_last_trace;

	TraceEntries::iterator it = g_trace_entries.find(thread_id);
	if (it == g_trace_entries.end())
		return nullptr;

	return it->second;
}

void GetLastTraceLog(DWORD thread_id, std::wstring& log)
{
	Trace* last_trace = GetLastTrace(thread_id);

	if (last_trace) 
		last_trace->BuildLog(log);
	else
		log.clear();
}

void SetLastTrace(Trace& trace)
{
	g_last_trace = &trace;
}

void SetLastTrace(DWORD thread_id, Trace& trace)
{
	if (GetCurrentThreadId() == thread_id)
		g_last_trace = &trace;
	else
	{
		TraceEntries::iterator it = g_trace_entries.find(thread_id);
		if (it != g_trace_entries.end())
			it->second = &trace;
	}
}

void ClearLastTrace()
{
	g_last_trace = nullptr;
}

void ClearLastTrace(DWORD thread_id)
{
	if (GetCurrentThreadId() == thread_id)
		g_last_trace = nullptr;
	else
	{
		TraceEntries::iterator it = g_trace_entries.find(thread_id);
		if (it != g_trace_entries.end())
			it->second = nullptr;
	}
}

static LPCWSTR dump_path = nullptr;

bool ReportLogToFile( LPCTSTR lpszFileName, LPCTSTR lpszFormat, ... )
{
	if ( NULL == lpszFileName )
	{
		return false;
	}

	if ( NULL == lpszFormat )
	{
		return false;
	}

	// 일단 로그 스트링을 만들고
	wchar_t szLogBuff[4096];
	wchar_t szLogToWrite[8192];

	va_list va;
	va_start( va, lpszFormat);
	if (SF_vsprintf( szLogBuff, _countof(szLogBuff), lpszFormat, va) != S_OK)
	{
		ASSERT(FALSE);
	}
	va_end( va);

	wchar_t szBuff[ MAX_PATH] = {0,};

	// 파일을 여는데.. 추후에 로그파일명에 시간을 넣는다거나 하는 작업을
	// 여기에서 변경해주도록 한다
	if (SF_sprintf( szBuff, _countof(szBuff), L"%s", lpszFileName ) != S_OK)
	{
		return false;
	}

	// 로그에 시간을 함께 기록한다
	SYSTEMTIME log_time;
	GetLocalTime( &log_time);

	if ( S_OK != SF_sprintf( szLogToWrite, _countof(szLogToWrite), L"%04d-%02d-%02d\t%02d:%02d:%02d\t%s\r\n", 
		log_time.wYear, log_time.wMonth, log_time.wDay, 
		log_time.wHour, log_time.wMinute, log_time.wSecond, 
		szLogBuff) )
	{
		return false;
	}

	WriteToFile( szBuff, szLogToWrite );

	return true;
}

NSStackWalker::NSStackWalker(const wchar_t* stack_path, const wchar_t* log_path) : StackWalker(), stack_path_(stack_path), log_path_(log_path), is_error_(false), file_(INVALID_HANDLE_VALUE)
{
}

NSStackWalker::~NSStackWalker()
{
	if (file_ != INVALID_HANDLE_VALUE)
		CloseHandle(file_);
}

void NSStackWalker::OnOutput(LPCSTR szText)
{
	if (is_error_)
	{
		if (file_ == INVALID_HANDLE_VALUE)
			return;
	}
	else if (file_ == INVALID_HANDLE_VALUE)
	{
		file_ = ::CreateFile(stack_path_.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (file_ == INVALID_HANDLE_VALUE)
		{
			ReportLogToFile(log_path_.c_str(), L"callstack create failed");
			return;
		}
	}

	DWORD tempByte = 0;
	WriteFile(file_, szText, static_cast<DWORD>(strlen(szText)), &tempByte, 0);
}

void NSStackWalker::OnDbgHelpErr(LPCSTR szFuncName, DWORD gle, DWORD64 addr)
{
	USES_CONVERSION;
	ReportLogToFile(log_path_.c_str(), L"[ERROR] %s - %d,%d", A2CW(szFuncName), gle, addr);
	is_error_ = true;
}

void PutLine(const TCHAR *filename,int linenumber,const TCHAR *format,...)
{
#ifdef _DEBUG
	TCHAR buff_format[ 128];
	TCHAR buff[MAX_DEBUG_MESSAGE_LEN];

	va_list ap;

	va_start( ap, format);
	if (SF_vsprintf( buff, MAX_DEBUG_MESSAGE_LEN, format, ap) != S_OK)
	{
		_ASSERT(FALSE);
	}
	va_end( ap);

	if (SF_sprintf( buff, _countof(buff), _T("DebugOut %d Line In %S : %s\n"), linenumber, filename, buff_format) != S_OK)
	{
		_ASSERT(FALSE);
	}

	::OutputDebugString( buff);
#endif
}

void Put(TCHAR *format,...)
{
#ifdef _DEBUG
	TCHAR buff[MAX_DEBUG_MESSAGE_LEN];

	va_list ap;
	
	va_start( ap, format);
	if (SF_vsprintf( buff, MAX_DEBUG_MESSAGE_LEN, format, ap) != S_OK)
	{
		_ASSERT(FALSE);
	}
	va_end( ap);

	::OutputDebugString( buff);
	::OutputDebugString( _T("\n"));
#endif
}

void WriteToFile( const wchar_t* lpszFileName, const wchar_t* lpszString )
{
	FILE* pFile = NULL;

	// 주어진 이름으로 파일을 열고
	if( 0 != _wfopen_s( &pFile, lpszFileName, L"a") )
	{
		return;
	}

	if ( NULL != pFile )
	{
		fwprintf_s( pFile, L"%s", lpszString );
		fclose( pFile );
	}
}

// ==>> by kjg
static PUTDUMP_CALLBACK s_pfnPutDumpCallback = NULL;

void SetPutDumpCallback(PUTDUMP_CALLBACK callback)
{
	s_pfnPutDumpCallback = callback;
}

void PutDump(TCHAR *format,...)
{
	TCHAR buff[MAX_DEBUG_MESSAGE_LEN];

	va_list ap;

	va_start( ap, format);
	if (SF_vsprintf( buff, MAX_DEBUG_MESSAGE_LEN, format, ap) != S_OK)
	{
		_ASSERT(FALSE);
	}
	va_end( ap);
	if (SF_strcat(buff, MAX_DEBUG_MESSAGE_LEN, _T("\n")) != S_OK)
	{
		_ASSERT(FALSE);
	}

	if (s_pfnPutDumpCallback)
		s_pfnPutDumpCallback(buff);

#ifdef _DEBUG
		::OutputDebugString(buff);
#endif
}

__declspec(thread) long g_last_error_code = 0;

void __stdcall SetLastErrorCode(long error_code)
{
	g_last_error_code = error_code;
}

long __stdcall GetLastErrorCode()
{
	return g_last_error_code;
}

// 
// void BSMessageBox(TCHAR* lpszStr, ... )
// {
// 	if (lstrlen(lpszStr) >= MAX_DEBUG_MESSAGE_LEN)
// 		return;
// 	
// 	TCHAR buf[MAX_DEBUG_MESSAGE_LEN] = {0,};
// 	
// 	va_list ap;
// 	va_start(ap, lpszStr);	
// 	if (SF_vsprintf(buf, MAX_DEBUG_MESSAGE_LEN, lpszStr, ap) != S_OK)
// 	{
// 		_ASSERT(FALSE);
// 	}
// 	va_end(ap);	
// 		
// 	::MessageBox(NULL, buf, _T("BSObj Plugin"), MB_OK);
// }

//-----------------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------
typedef BOOL	(WINAPI *DF_MiniDumpWriteDump)(HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType,CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam);
typedef BOOL	(WINAPI *DF_StackWalk)(DWORD MachineType,HANDLE hProcess,HANDLE hThread,LPSTACKFRAME StackFrame,PVOID ContextRecord,PREAD_PROCESS_MEMORY_ROUTINE ReadMemoryRoutine,PFUNCTION_TABLE_ACCESS_ROUTINE  FunctionTableAccessRoutine,PGET_MODULE_BASE_ROUTINE GetModuleBaseRoutine,PTRANSLATE_ADDRESS_ROUTINE TranslateAddress);
typedef PVOID	(WINAPI *DF_SymFunctionTableAccess)(HANDLE  hProcess,DWORD AddrBase);
typedef DWORD	(WINAPI *DF_SymGetModuleBase)(HANDLE hProcess,DWORD qwAddr);
typedef BOOL	(WINAPI *DF_SymInitialize)(HANDLE hProcess,PTSTR UserSearchPath,BOOL fInvadeProcess);
typedef DWORD	(WINAPI *DF_SymSetOptions)(DWORD   SymOptions);
typedef DWORD	(WINAPI *DF_SymGetOptions)(VOID);
typedef DWORD	(WINAPI *DF_UnDecorateSymbolName)(PCTSTR DecoratedName,PTSTR UnDecoratedName,DWORD UndecoratedLength,DWORD Flags);
typedef BOOL	(WINAPI *DF_SymGetLineFromAddr)(HANDLE hProcess,DWORD qwAddr,PDWORD pdwDisplacement,PIMAGEHLP_LINE64 Line);
typedef BOOL	(WINAPI *DF_SymGetSymFromAddr)(HANDLE hProcess,DWORD qwAddr,PDWORD pdwDisplacement,PIMAGEHLP_SYMBOL64 Symbol);
typedef BOOL	(WINAPI *DF_SymFromAddr)(HANDLE hProcess,DWORD qwAddr,PDWORD pdwDisplacement,PIMAGEHLP_SYMBOL64 Symbol);
typedef BOOL	(WINAPI *DF_SymGetModuleInfo)(HANDLE hProcess,DWORD qwAddr,PIMAGEHLP_MODULE64 ModuleInfo);

DF_MiniDumpWriteDump			g_DF_MiniDumpWriteDump			= NULL;
DF_StackWalk					g_DF_StackWalk					= NULL;	
DF_SymFunctionTableAccess		g_DF_SymFunctionTableAccess		= NULL;	
DF_SymGetModuleBase				g_DF_SymGetModuleBase			= NULL;
DF_SymInitialize				g_DF_SymInitialize				= NULL;
DF_SymSetOptions				g_DF_SymSetOptions				= NULL;
DF_SymGetOptions				g_DF_SymGetOptions				= NULL;
DF_UnDecorateSymbolName			g_DF_UnDecorateSymbolName		= NULL;	
DF_SymGetLineFromAddr			g_DF_SymGetLineFromAddr			= NULL;	
DF_SymGetSymFromAddr			g_DF_SymGetSymFromAddr			= NULL;	
DF_SymGetModuleInfo				g_DF_SymGetModuleInfo			= NULL;

HMODULE				g_hDBGHelpDll = NULL;
bool				g_SymInitialized = false;
DWORD				g_DebugOpton[ 2] = { DEBUG_OPTION_DEFAULT_CLIENT_DEBUGGER_PRESENT, DEBUG_OPTION_DEFAULT_CLIENT_STAND_ALONE};
ASSERT_CALLBACK		g_AssertCallback = NULL;

TCHAR				g_szOutputBuffer[ 4096] = {0};	// 왜 메모리가 4k가 증가하냐고 여기저기 찾겠지? 후후후후후

TCHAR*				g_pCallStack;
bool				g_bTerminateProcess = false;
CCriticalSectionBS	g_minidump_write_critical_section;

//-------------------------------------------------------------------------------------------------------------------------
#define GET_PROC( name)	\
	g_DF_##name = (DF_##name) ::GetProcAddress( g_hDBGHelpDll, #name);\
	if( g_DF_##name == NULL)\
		goto error_and_unload;

void InitSymbolPath(TCHAR *lpszIniPath)
{
//	CHAR *lpszPath;//[BUFFERSIZE];
	TCHAR*	lpszPath;
	size_t	nLen;

	// Creating the default path
	// ".;%_NT_SYMBOL_PATH%;%_NT_ALTERNATE_SYMBOL_PATH%;%SYSTEMROOT%;%SYSTEMROOT%\System32;"
	if (SF_strcat( lpszIniPath, MAX_PATH, _T("\\.") ) != S_OK)
	{
		_ASSERT(FALSE);
	}

	#define SYMBOL_PATH				_T("_NT_SYMBOL_PATH")
	#define ALTERNATE_SYMBOL_PATH	_T("_NT_ALT_SYMBOL_PATH")
	#define SYSTEMROOT_PATH			_T("SYSTEMROOT")

	// environment variable _NT_SYMBOL_PATH
	//if ( GetEnvironmentVariableA( _T("_NT_SYMBOL_PATH"), lpszPath, BUFFERSIZE ) )
//	if( lpszPath = getenv(SYMBOL_PATH) )
	if( 0 != _tdupenv_s( &lpszPath, &nLen, SYMBOL_PATH) )
	{
		if (SF_strcat( lpszIniPath, MAX_PATH, _T(";") ) != S_OK)
		{
			_ASSERT(FALSE);
		}
		if (SF_strcat( lpszIniPath, MAX_PATH, lpszPath ) != S_OK)
		{
			_ASSERT(FALSE);
		}
	}
	else
	{
		free(lpszPath);
	}

	// environment variable _NT_ALTERNATE_SYMBOL_PATH
	//if ( GetEnvironmentVariableA( "_NT_ALTERNATE_SYMBOL_PATH", lpszPath, BUFFERSIZE ) )
//	if( lpszPath = getenv(ALTERNATE_SYMBOL_PATH) )
	if( 0 != _tdupenv_s( &lpszPath, &nLen, ALTERNATE_SYMBOL_PATH) )
	{
		if (SF_strcat( lpszIniPath, MAX_PATH, _T(";") ) != S_OK)
		{
			_ASSERT(FALSE);
		}
		if (SF_strcat( lpszIniPath, MAX_PATH, lpszPath ) != S_OK)
		{
			_ASSERT(FALSE);
		}
	}
	else
	{
		free(lpszPath);
	}

	// environment variable SYSTEMROOT
	//if ( GetEnvironmentVariableA( "SYSTEMROOT", lpszPath, BUFFERSIZE ) )
//	if( lpszPath = getenv(SYSTEMROOT_PATH) )
	if( 0 != _tdupenv_s( &lpszPath, &nLen, SYSTEMROOT_PATH) )
	{
		if (SF_strcat( lpszIniPath, MAX_PATH, _T(";") ) != S_OK)
		{
			_ASSERT(FALSE);
		}
		if (SF_strcat( lpszIniPath, MAX_PATH, lpszPath ) != S_OK)
		{
			_ASSERT(FALSE);
		}
		if (SF_strcat( lpszIniPath, MAX_PATH, _T(";") ) != S_OK)
		{
			_ASSERT(FALSE);
		}

		// SYSTEMROOT\System32
		if (SF_strcat( lpszIniPath, MAX_PATH, lpszPath ) != S_OK)
		{
			_ASSERT(FALSE);
		}
		if (SF_strcat( lpszIniPath, MAX_PATH, _T("\\System32") ) != S_OK)
		{
			_ASSERT(FALSE);
		}
	}
	else
	{
		free(lpszPath);

		TCHAR lpszPath2[MAX_PATH];
		if( GetWindowsDirectory(lpszPath2, MAX_PATH-1) != 0 )
		{
			if (SF_strcat( lpszIniPath, MAX_PATH, _T(";") ) != S_OK)
			{
				_ASSERT(FALSE);
			}
			if (SF_strcat( lpszIniPath, MAX_PATH, lpszPath2 ) != S_OK)
			{
				_ASSERT(FALSE);
			}
			if (SF_strcat( lpszIniPath, MAX_PATH, _T(";") ) != S_OK)
			{
				_ASSERT(FALSE);
			}

			// SYSTEMROOT\System32
			if (SF_strcat( lpszIniPath, MAX_PATH, lpszPath2 ) != S_OK)
			{
				_ASSERT(FALSE);
			}
			if (SF_strcat( lpszIniPath, MAX_PATH, _T("\\System") ) != S_OK)
			{
				_ASSERT(FALSE);
			}
		}
	}
}

bool PrepareDBGHelpLibrary()
{
	DWORD dwOpts;
	TCHAR szSymbolPath[ MAX_PATH]={0};

	if( g_SymInitialized)
		return true;

	g_hDBGHelpDll = ::LoadLibrary( _T("dbghelp.dll"));
//	g_hDBGHelpDll = ::LoadLibrary( "imagehlp.dll");
	if( g_hDBGHelpDll == NULL)
	{
		if (SF_strcpy( g_szOutputBuffer, 4096, _T("cannot load dbghelp.dll")) != S_OK)
		{
			_ASSERT(FALSE);
		}
		return false;
	}

	GET_PROC( MiniDumpWriteDump			);
	GET_PROC( StackWalk					);
	GET_PROC( SymFunctionTableAccess	);
	GET_PROC( SymGetModuleBase			);
	GET_PROC( SymInitialize				);
	GET_PROC( SymSetOptions				);
	GET_PROC( SymGetOptions				);
	GET_PROC( UnDecorateSymbolName		);
	GET_PROC( SymGetLineFromAddr		);
	GET_PROC( SymGetSymFromAddr			);
	GET_PROC( SymGetModuleInfo			);

	dwOpts = g_DF_SymGetOptions();
	dwOpts |= SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES | SYMOPT_EXACT_SYMBOLS;
	dwOpts &= ~SYMOPT_UNDNAME;
	g_DF_SymSetOptions( dwOpts);

	InitSymbolPath( szSymbolPath);

	if( !g_DF_SymInitialize( GetCurrentProcess(), NULL, TRUE))
		goto error_and_unload;

	g_SymInitialized = true;

	return true;

error_and_unload:
	if (SF_strcpy( g_szOutputBuffer, 4096, _T("cannot load dbghelp.dll : wrong version or SymInitialize fail")) != S_OK)
	{
		_ASSERT(FALSE);
	}

	::FreeLibrary( g_hDBGHelpDll);

	return false;
}

#undef GET_PROC

int GetContextInfo_MiniDump(unsigned int code, struct _EXCEPTION_POINTERS *pExceptionInfo) 
{
	SCOPED_LOCK_SINGLE(&g_minidump_write_critical_section);

	TCHAR szTemp[ MAX_PATH] = {0,};
	TCHAR szModuleDrive[ MAX_PATH] = {0,};
	TCHAR szModulePath[ MAX_PATH] = {0,};
	TCHAR szModuleFilename[ MAX_PATH] = {0,};
	TCHAR szDateTime[ _MAX_PATH] = {0,};
	TCHAR szDumpPath[_MAX_PATH] = {0,};
	//TCHAR szStackPath[_MAX_PATH] = {0,};
	TCHAR szLogPath[_MAX_PATH] = {0,};

	GetModuleFileName( NULL, szTemp, _MAX_PATH);

	//_splitpath( szTemp, szModuleDrive, szModulePath, szModuleFilename, NULL);
	_tsplitpath_s( szTemp,	szModuleDrive, MAX_PATH,
		szModulePath, MAX_PATH,
		szModuleFilename, MAX_PATH,
		NULL, 0 );

	SYSTEMTIME time;
	GetLocalTime( &time);

	SF_sprintf( szDateTime, _countof(szDateTime), _T("_[%04d-%02d-%02d %02d-%02d-%02d-%d]"), time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);
	SF_strcat( szModuleFilename, _MAX_PATH, szDateTime);

	if (dump_path != nullptr)
	{
		::swprintf_s(szDumpPath, _MAX_PATH, L"%s\\%s.dmp", dump_path, szModuleFilename);
		//::swprintf_s(szStackPath, _MAX_PATH, L"%s\\%s.stk", dump_path, szModuleFilename);
		::swprintf_s(szLogPath, _MAX_PATH, L"%s\\%s.log", dump_path, szModuleFilename);
	}
	else
	{
		_tmakepath_s( szDumpPath, _MAX_PATH, szModuleDrive, szModulePath, szModuleFilename, _T(".dmp"));
		//_tmakepath_s( szStackPath, _MAX_PATH, szModuleDrive, szModulePath, szModuleFilename, _T(".stk"));
		_tmakepath_s( szLogPath, _MAX_PATH, szModuleDrive, szModulePath, szModuleFilename, _T(".log"));
	}

	ReportLogToFile(szLogPath, L"get context info called - code:0x%X", code);

	if( !PrepareDBGHelpLibrary())
	{
		ReportLogToFile(szLogPath, L"dbghelp prepare failed");
		return EXCEPTION_EXECUTE_HANDLER;
	}

	/*
	NSStackWalker sw(szStackPath, szLogPath);
	sw.ShowCallstack(GetCurrentThread(), pExceptionInfo->ContextRecord);
	*/
	
	HANDLE hFile = ::CreateFile( szDumpPath, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
										FILE_ATTRIBUTE_NORMAL, NULL );

	if (hFile == INVALID_HANDLE_VALUE)
	{
		ReportLogToFile(szLogPath, L"file create failed");
		return EXCEPTION_EXECUTE_HANDLER;
	}

	_MINIDUMP_EXCEPTION_INFORMATION ExInfo;
	ExInfo.ThreadId = ::GetCurrentThreadId();
	ExInfo.ExceptionPointers = pExceptionInfo;
	ExInfo.ClientPointers = FALSE;

	ReportLogToFile(szLogPath, L"thread_id:%d", ExInfo.ThreadId);

	MEMORY_BASIC_INFORMATION mbi;
	VirtualQueryEx(GetCurrentProcess(), pExceptionInfo->ExceptionRecord->ExceptionAddress, &mbi,sizeof(mbi) );

	ReportLogToFile(szLogPath, L"exception_addrdss:0x%X", pExceptionInfo->ExceptionRecord->ExceptionAddress);

	if (g_DF_MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &ExInfo, nullptr, nullptr) == FALSE)
		ReportLogToFile(szLogPath, L"minidump write failed");
	else
		ReportLogToFile(szLogPath, L"minidump write complete");

	CloseHandle( hFile);

	ReportLogToFile(szLogPath, L"get context info finished");
	
	return EXCEPTION_EXECUTE_HANDLER;
}

/////////////////////////////////////////////////////////
// Debug Function
/////////////////////////////////////////////////////////
void SetDebugOption(DWORD option_debugger_present,DWORD option_stand_alone)
{
	g_DebugOpton[ 0] = option_debugger_present;
	g_DebugOpton[ 1] = option_stand_alone;
}

void SetAssertCallback(ASSERT_CALLBACK callback)
{
	g_AssertCallback = callback;
}

void RegisterUnhandledExceptionFilter()
{

}

bool ProcessAssertF(int iLine,TCHAR* filename, TCHAR* expression,...)
{
	// thread safe하게
	g_bTerminateProcess = true;

	DWORD option = g_DebugOpton[ IsDebuggerPresent() ? 0 : 1];

	TCHAR szCallStack[ 4096];
	szCallStack[ 0] = '\0';
	
	if(	(option & DEBUG_OPTION_ASSERT_WRITE_MINIDUMP))
	{
		__try
		{
			g_pCallStack = szCallStack;
			throw _T("oops");
		}
		__except(GetContextInfo_MiniDump(GetExceptionCode(), GetExceptionInformation()))	{}
	}

	TCHAR szMessage[ 1024];

	va_list ap;
	va_start( ap, expression );
	if (SF_vsprintf( szMessage, 1024, expression, ap) != S_OK)
	{
		_ASSERT(FALSE);
		//return false;
	}
	va_end(ap);

	bool bAdvance = false;
	if( (option & DEBUG_OPTION_ASSERT_CALL_CALLBACK)) // callback함수 불러 주세용 
	{
		g_AssertCallback( iLine, filename, szMessage );
	}

	if( (option & DEBUG_OPTION_ASSERT_SHOW_MESSAGEBOX_OKCANCEL) || (option & DEBUG_OPTION_ASSERT_SHOW_MESSAGEBOX_OK)) // message박스 띄워 주세요
	{
		TCHAR szMessageBox[ 4096];
		
		if (SF_sprintf( szMessageBox, _countof(szMessageBox), _T("Debug Assertion Failed.\n\nExpression : %s\nLine : %d\nFile : %S\n\n"), szMessage, iLine, filename) != S_OK)
		{
			_ASSERT(FALSE);
			return false;
		}

		if( option & DEBUG_OPTION_ASSERT_SHOW_MESSAGEBOX_OKCANCEL)
		{
			if (SF_strcat( szMessageBox, 4096, _T("(press \"ok\" to advance, press \"cancel\" to exit)")) != S_OK)
			{
				_ASSERT(FALSE);
				return false;
			}
			if( ::MessageBox( NULL, szMessageBox, _T("BSLib Debug Module"), MB_OKCANCEL | MB_ICONERROR) == IDOK)
			{
				bAdvance = true;
			}
		}
		else
		{
			if (SF_strcat( szMessageBox, 4096, _T("(press \"ok\" to exit program)")) != S_OK)
			{
				_ASSERT(FALSE);
				return false;
			}
			::MessageBox( NULL, szMessageBox, _T("BSLib Debug Module"), MB_OK | MB_ICONERROR);
		}
	}

	if( bAdvance) // assert를 진행하라고?
	{
		if( option & DEBUG_OPTION_ASSERT_ADVANCE_BREAK)
			return false;
	}
	else // assert에서 멈춰라
	{
//		if( option & DEBUG_OPTION_ASSERT_CANCEL_RAISE_EXCEPTION)
//		{
//			throw _T("debug assertion failed");
//			return true;
//		}
		ExitProcess( 0);
	}

	return true;
}

extern bool ProcessAssertLog(int line, TCHAR* filename, TCHAR* expression, ...)
{
	TCHAR expression_msg[ 1024];

	va_list ap;
	va_start(ap, expression);
	if (SF_vsprintf(expression_msg, 1024, expression, ap) != S_OK)
		return false;
	
	va_end(ap);

	PutLog(LOG_WARNING_FILE, L"assert! file:[%s] line:[%d] expression:[%s]", filename, line, expression_msg);

	return true;
}

void WriteMiniDump()
{
	//DWORD option = g_DebugOpton[ IsDebuggerPresent() ? 0 : 1];
	DWORD option = DEBUG_OPTION_ASSERT_WRITE_MINIDUMP;

	if( option & DEBUG_OPTION_ASSERT_WRITE_MINIDUMP)
	{
		__try
		{
			*(int*)(0) = *(int*)"An exception has been occured. You should check callstacks.";
		}
		__except(GetContextInfo_MiniDump(GetExceptionCode(), GetExceptionInformation()))	{}
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
void __cdecl InvalidCRTParameterHandler(const wchar_t * expression,
									   const wchar_t * function, 
									   const wchar_t * file, 
									   unsigned int line,
									   uintptr_t pReserved)
{
	/*
	WCHAR wszMsg[2048];
	swprintf_s(	wszMsg, 
				_countof(wszMsg),
				L"Invalid parameter detected in function %s, " 
				L"File: %s Line: %d\n"
				L"Expression: %s\n",
				function, file, line, expression);

	PutDump("%S", wszMsg);
	*/

	// 여기에서 발생시키는 예외가 핸들링되어도 덤프를 남기기 위해 무조건 여기 걸리면 덤프를 남긴다.
	WriteMiniDump();
}

void SE_Translator(unsigned code, EXCEPTION_POINTERS* info)
{
	GetContextInfo_MiniDump(code, info);
}

void NewHandler()
{
	WriteMiniDump();
}

void PurecallHandler()
{
	WriteMiniDump();
}

void SigabrtHandler(int sig_num)
{
	WriteMiniDump();
}

void SetDebugHelperHandlers()
{
	std::set_new_handler(NewHandler);
	_set_se_translator(SE_Translator);
	_set_invalid_parameter_handler(InvalidCRTParameterHandler);
	_set_purecall_handler(PurecallHandler);
	_set_abort_behavior(0, _WRITE_ABORT_MSG);
	signal(SIGABRT, SigabrtHandler);
}

void SetDumpPath(LPCWSTR path)
{
	dump_path = path;
}

//void MemoryAccessCrash()
//{
//	char *p = 0;
//	*p = 5;
//}

//void OutOfBoundsVectorCrash()
//{
//	std::vector<int> v;
//	v[0] = 5;
//}

//void AbortCrash()
//{
//	abort();
//}
//
//void VirtualFunctionCallCrash()
//{
//	struct B
//	{
//		B()
//		{
//			Bar();
//		}
//
//		virtual void Foo() = 0;
//
//		void Bar()
//		{
//			Foo();
//		}
//	};
//
//	struct D: public B
//	{
//		void Foo()
//		{
//		}
//	};
//
//	B* b = new D;
//	// Just to silence the warning C4101: 'VirtualFunctionCallCrash::B::Foo' : unreferenced local variable
//	b->Foo(); 
//}

//void TestDump(BYTE option)
//{
//	switch (option)
//	{
//	case 0:
//		MemoryAccessCrash();
//		break;
//	case 1:
//		OutOfBoundsVectorCrash();
//		break;
//	case 2:
//		AbortCrash();
//		break;
//	case 3:
//		VirtualFunctionCallCrash();
//		break;
//	}
//}