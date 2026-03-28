#pragma once

/////////////////////////////// 
// useful constants
/////////////////////////////// 

#define SIN(a)		sinf(a)
#define COS(a)		cosf(a)
#define TAN(a)		tanf(a)
#define ASIN(a)		asinf(a)
#define ACOS(a)		acosf(a)
#define ATAN(a)		atanf(a)
#define ATAN2(a)	atan2f(a)
#define SQRT(a)		sqrtf(a)
#define ABS(a)		fabsf(a)

#define MIN(a,b)			((a) < (b) ? (a) : (b))
#define MAX(a,b)			((a) > (b) ? (a) : (b))

#ifndef min
#define min(a,b)			((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a,b)			((a) > (b) ? (a) : (b))
#endif

#define FABS(a)				((a) < 0.0f ? -(a) : (a))
#define OUT_OF_RANGE(a)		((a) < 0.0f || (a) > 1.f)

// 졸라 min > max 인 경우에 대한 아무런 보호장치가 없다.. 조때는 거다아...
//#define CLAMP(x, min, max)  (x) = ((x) < (min) ? (min) : (x) < (max) ? (x) : (max));
// 그래서! 아래와 같이 고친다.

// BSLib가 아직 유니코드가 아니기땜시.. LPCTSTR을 하면 다른곳에 포함을 못시킨다 by novice
extern void PutLog(DWORD logtype, LPCWSTR foramt, ...);
#ifdef SERVER_BUILD
extern void PutLog(DWORD logtype, JSONValue& value);
#endif // #ifdef SERVER_BUILD

#define CLAMP(x, min, max)	\
{							\
	if (min > max)			\
	{						\
		PutLog(LOG_FATAL_FILE, _T("CLAMP() ==> min(%.3f) exceeded max(%.3f) value), File: %s, Line: %d"), (float)min, (float)max, __FILE__, __LINE__);	\
		x = min;			\
	}						\
	(x) = ((x) < (min) ? (min) : (x) < (max) ? (x) : (max));\
}

#define _PI				3.14159265358979323846f // Pi
#define _2_PI			6.28318530717958623200f // 2 * Pi
#define _PI_DIV_2		1.57079632679489655800f // Pi / 2
#define _PI_DIV_4		0.78539816339744827900f // Pi / 4
#define _INV_PI			0.31830988618379069122f // 1 / Pi
#define _FLOAT_HUGE		1.0e+38f              // Huge number for FLOAT
#define _EPSILON		1.0e-6f               // Tolerance for FLOATs

#define _DEGTORAD(x)		(float)((x) * 0.01745329251994329547f)	// Degrees to Radians
#define _RADTODEG(x)		(float)((x) * 57.29577951308232286465f)	// Radians to Degrees
#define RANDOM_NUM			(((float)rand() - (float)rand()) / RAND_MAX)	// RAND_MAX --> 0x7fff 라고 define되있네...
#define RANDOM_VECTOR		Vector(RANDOM_NUM, RANDOM_NUM, RANDOM_NUM)
#define RANDOM_NORM(v)		(v) = RANDOM_VECTOR; (v).Normalize();	

#define AXISMASK_NONE	0x00000000
#define AXISMASK_X		0x00000001
#define AXISMASK_Y		0x00000002
#define AXISMASK_Z		0x00000004
#define AXISMASK_ALL	0x00000007

// For physics
#define	PHY_GRAVITY					 -9.8f
#define	PHY_SPRINGTENSIONCONSTANT	100.0f
#define	PHY_SPRINGDAMPINGCONSTANT	  2.0f
#define	PHY_STIFFNESS				 0.01f
#define	PHY_WINDFACTOR				100.0f
#define	PHY_COLLISIONTOLERANCE		 0.05f
#define	PHY_KRESTITUTION			 0.25f
#define	PHY_FRICTIONFACTOR			  0.5f

#define _ALGN16			__declspec(align(16))

#define VecDot(a,b) (a.x * b.x + a.y * b.y + a.z * b.z)
#define VecDot2(a,b) (a.x * b.x + a.y * b.y)
#define VecDot4(a,b) (a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w)
#define	VecCross(a,b) Vector( a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x)
