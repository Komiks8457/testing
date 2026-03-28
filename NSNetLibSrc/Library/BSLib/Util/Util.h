#pragma once 

static DWORD ChangeByteOrder(DWORD data)
{
	return ((data >> 24) & 0xFF) +
		(((data >> 16) & 0xFF) << 8) +
		(((data >> 8) & 0xFF) << 16) +
		((data & 0xFF) << 24);
}

// log level
enum eLogLevel
{
	LL_NOTIFY,	// 그냥 알려주고 싶을때
	LL_WARNING,	// 경고 (서버를 내릴정도는 아니지만 주의하라 머 그런거)
	LL_FATAL,	// 더이상 서버를 진행 할 수 없을때
	LL_DEBUG,	// 로컬 디버그 환경에서 보고 싶을때
	LL_TRACE,	// 특정 상황시 해당 내용을 추적 하고 싶을때
	LL_COUNT,
};

#define EXTRACT_LOG_LEVEL(type) (type >> 24)

// minor option mask
#define LOG_MINOR_OPTION_NONE			0x0000
#define LOG_MINOR_OPTION_TO_FILE		0x0001
#define LOG_MINOR_OPTION_TO_JSON_FILE	0x0002
#define LOG_MINOR_OPTION_TO_FILE_BE		0x0003

#define HAS_LOG_MINOR_OPTION(type, option) ((type & 0xffff) == option)
#define CHANGE_LOG_MINOR_OPTION(value, minor_option) (DWORD)( ( value & 0xFFFF0000 ) | (minor_option & 0xFFFF) )
#define IS_LOG_TO_FILE(type) (HAS_LOG_MINOR_OPTION(type, LOG_MINOR_OPTION_TO_FILE))
#define IS_LOG_TO_JSON_FILE(type) (HAS_LOG_MINOR_OPTION(type, LOG_MINOR_OPTION_TO_JSON_FILE))
#define IS_LOG_TO_FILE_BE(type) (HAS_LOG_MINOR_OPTION(type, LOG_MINOR_OPTION_TO_FILE_BE))

#define LOGTYPE( level, major_index, minor_index) (DWORD)(((level & 0xFF) << 24) | ((major_index & 0xFF) << 16) | (minor_index & 0xFFFF))
#define LOG_NOTIFY					LOGTYPE(LL_NOTIFY, 0, LOG_MINOR_OPTION_NONE)
#define LOG_NOTIFY_FILE				LOGTYPE(LL_NOTIFY, 0, LOG_MINOR_OPTION_TO_FILE)
#define LOG_NOTIFY_FILE_BE			LOGTYPE(LL_NOTIFY, 0, LOG_MINOR_OPTION_TO_FILE_BE)
#define LOG_WARNING					LOGTYPE(LL_WARNING, 0, LOG_MINOR_OPTION_NONE)
#define LOG_WARNING_FILE			LOGTYPE(LL_WARNING, 0, LOG_MINOR_OPTION_TO_FILE)
#define LOG_WARNING_FILE_BE			LOGTYPE(LL_WARNING, 0, LOG_MINOR_OPTION_TO_FILE_BE)
#define LOG_FATAL					LOGTYPE(LL_FATAL, 0, LOG_MINOR_OPTION_NONE)
#define LOG_FATAL_FILE				LOGTYPE(LL_FATAL, 0, LOG_MINOR_OPTION_TO_FILE)
#define LOG_FATAL_FILE_BE			LOGTYPE(LL_FATAL, 0, LOG_MINOR_OPTION_TO_FILE_BE)
#define LOG_DEBUG					LOGTYPE(LL_DEBUG, 0, LOG_MINOR_OPTION_NONE)
#define LOG_DEBUG_FILE				LOGTYPE(LL_DEBUG, 0, LOG_MINOR_OPTION_TO_FILE)
#define LOG_DEBUG_FILE_BE			LOGTYPE(LL_DEBUG, 0, LOG_MINOR_OPTION_TO_FILE_BE)
#define LOG_TRACE					LOGTYPE(LL_TRACE, 0, LOG_MINOR_OPTION_NONE)
#define LOG_TRACE_FILE				LOGTYPE(LL_TRACE, 0, LOG_MINOR_OPTION_TO_FILE)
#define LOG_TRACE_FILE_BE			LOGTYPE(LL_TRACE, 0, LOG_MINOR_OPTION_TO_FILE_BE)

#include "CPConv.h"
#include "SafeCount.h"
#include "buffer.h"
#include "fstream.h"
//#include "atlaux.h"
#include "que.h"
#include "chunkallocator.h"
//#include "CPUDetector.h"

#include "alignedalloc.h"
#include "objmap.h"
#include "Facilities.h"
#include "SimpleTime.h"
#include "timespan.h"
#include "TickCount64.h"
#include "../Todo.h"		
#include "bs_reg.h"

#ifndef MEMORY_PROFILER
#include "SimpleMemPooler.h"
#endif