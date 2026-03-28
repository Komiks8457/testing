#pragma once

// 앞으로 이넘은 Exporting전용 include로 
/*
#pragma warning(disable:4514) // pragma to get rid of math.h inline function removal warnings.
#pragma warning(disable:4018)
#pragma warning(disable:4786)
#pragma warning(disable:4251) // non-exported memeber of exported class
#pragma warning(disable:4275) // deriving exported class from non-exported
#pragma warning(disable:4290) // throw() not supported without __declspec(nothrow)
#pragma warning(disable:4146)
#pragma warning(disable:4244) // long to word possible loss of data warning
#pragma warning(disable:4786) // identifier was truncated to '255' characters in the debug information - by novice.
*/

#pragma warning(disable:4100)
#pragma warning(disable:4127)
#pragma warning(disable:4201)	// union struct에 이름 없다고!!
#pragma warning(disable:4481)	// virtual - override 썼다고!!
#pragma warning(disable:4505)

#define WIN32_LEAN_AND_MEAN

#ifndef _WIN32_WINDOWS 
#define _WIN32_WINDOWS 0x0410
#endif

#define APPLY_SECURED_KEY_EXCHANGE
/* VS2010에선 _ITERATOR_DEBUG_LEVEL 를 사용함 
// stl iterator에 대한 secure check 기능을 넣는다!
#ifdef _DEBUG
#	define _HAS_ITERATOR_DEBUGGING	1
#else
#	define _SECURE_SCL				1
#	define _SECURE_SCL_THROWS		1
#endif
*/

//*
#ifdef _UNICODE
#	define tstring wstring
#else
#	define tstring string
#endif 
//*/

/*
/////////////////////////////////////// multi-threaded compatible compile
# if !defined (_REENTRANT)
#   define _REENTRANT 1
# endif

/////////////////////////////////////// 
//# if defined (BSLIB_USE_DEBUG_STL_ALLOCATOR)
	#define _STLP_DEBUG_ALLOC
//# else
//	#undef  _STLP_DEBUG_ALLOC
//#endif

/////////////////////////////////////// 
#define _STLP_USE_SYSTEM_ASSERT
#define _STLP_USE_OWN_NAMESPACE
//#define _STLP_USE_ABBREVS

/////////////////////////////////////// 
# ifdef _DEBUG
	#define _STLP_DEBUG 1
# else
	# undef _STLP_DEBUG
# endif

/////////////////////////////////////// */
# ifdef _STLP_USE_MFC
#	include <afx.h>
# else
#    include <windows.h>
# endif

#include <atlstr.h>
#include <stdio.h>
#include <tchar.h>
#include <math.h>
#include <time.h>

#include <vector>
#include <set>
#include <map>

#if defined (_MSC_VER)
	#if (_MSC_VER <= 1400)
		#include <hash_map>
		#include <hash_set>
		#define HASH_MAP std::hash_map
		#define HASH_SET std::hash_set
	#else
		#include <unordered_map>
		#include <unordered_set>
		#define HASH_MAP std::unordered_map
		#define HASH_SET std::unordered_set
	#endif
#endif

#include <queue>
#include <string>
#include <sstream>
#include <list>
#include <algorithm>
#include <functional>
//#include <alloc>

#ifdef SERVER_BUILD
#include "Json/JsonValue.h"
extern void PutLog(DWORD logtype, JSONValue& value);
#endif // #ifdef SERVER_BUILD

//#include "util/fastmemcpy.h"
#include "dbg/debug.h"
#include "StringFunc/StringFunc.h"
#include "util/util.h"
#include "math/MathLib.h"
#include "pattern/singleton.h"

