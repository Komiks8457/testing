#include "stdafx.h"
#include "TimeSpan.h"
#include <Time.h>


// CBSTime and CBSTimeSpan
CBSTimeSpan::CBSTimeSpan() : m_time(-1)
{ 
}

CBSTimeSpan::CBSTimeSpan(time_t time)
{ 
	m_time = time; 
}
CBSTimeSpan::CBSTimeSpan(LONG lDays, int nHours, int nMins, int nSecs)
{ 
	m_time = nSecs + 60* (nMins + 60* (nHours + 24* lDays)); 
}
CBSTimeSpan::CBSTimeSpan(const CBSTimeSpan& timeSpanSrc)
{ 
	m_time = timeSpanSrc.m_time; 
}
const CBSTimeSpan& CBSTimeSpan::operator=(const CBSTimeSpan& timeSpanSrc)
{ 
	m_time = timeSpanSrc.m_time; return *this; 
}
LONG CBSTimeSpan::GetDays() const
{ 
	return (LONG)(m_time / (24*3600L)); 
}
LONG CBSTimeSpan::GetTotalHours() const
{ 
	return (LONG)(m_time/3600);
}
int CBSTimeSpan::GetHours() const
{ 
	return (int)(GetTotalHours() - GetDays()*24); 
}
LONG CBSTimeSpan::GetTotalMinutes() const
{ 
	return (LONG)(m_time/60);
}
int CBSTimeSpan::GetMinutes() const
{ 
	return (int)(GetTotalMinutes() - GetTotalHours()*60); 
}
LONG CBSTimeSpan::GetTotalSeconds() const
{ 
	return (LONG)m_time; 
}
int CBSTimeSpan::GetSeconds() const
{ 
	return (int)(GetTotalSeconds() - GetTotalMinutes()*60); 
}
CBSTimeSpan CBSTimeSpan::operator-(CBSTimeSpan timeSpan) const
{ 
	return CBSTimeSpan(m_time - timeSpan.m_time); 
}
CBSTimeSpan CBSTimeSpan::operator+(CBSTimeSpan timeSpan) const
{ 
	return CBSTimeSpan(m_time + timeSpan.m_time); 
}
const CBSTimeSpan& CBSTimeSpan::operator+=(CBSTimeSpan timeSpan)
{ 
	m_time += timeSpan.m_time; return *this; 
}
const CBSTimeSpan& CBSTimeSpan::operator-=(CBSTimeSpan timeSpan)
{ 
	m_time -= timeSpan.m_time; return *this; 
}
BOOL CBSTimeSpan::operator==(CBSTimeSpan timeSpan) const
{ 
	return m_time == timeSpan.m_time; 
}
BOOL CBSTimeSpan::operator!=(CBSTimeSpan timeSpan) const
{ 
	return m_time != timeSpan.m_time; 
}
BOOL CBSTimeSpan::operator<(CBSTimeSpan timeSpan) const
{ 
	return m_time < timeSpan.m_time; 
}
BOOL CBSTimeSpan::operator>(CBSTimeSpan timeSpan) const
{
	return m_time > timeSpan.m_time; 
}
BOOL CBSTimeSpan::operator<=(CBSTimeSpan timeSpan) const
{
	return m_time <= timeSpan.m_time; 
}
BOOL CBSTimeSpan::operator>=(CBSTimeSpan timeSpan) const
{
	return m_time >= timeSpan.m_time; 
}


CBSTime::CBSTime() : m_time(-1)
{ 
	ZeroMemory( (LPVOID)&m_tm, sizeof(m_tm) );
}

CBSTime::CBSTime(time_t time)
{ 
	m_time = time; 
	ZeroMemory( (LPVOID)&m_tm, sizeof(m_tm) );
}
CBSTime::CBSTime(const CBSTime& timeSrc)
{ 
	m_time = timeSrc.m_time;
	m_tm = timeSrc.m_tm;
}
const CBSTime& CBSTime::operator=(const CBSTime& timeSrc)
{ 
	m_time	= timeSrc.m_time;
	m_tm	= timeSrc.m_tm;
	return *this; 
}
const CBSTime& CBSTime::operator=(time_t t)
{
	m_time = t;
	return *this; 
}
time_t CBSTime::GetTime() const
{
	return m_time; 
}
int CBSTime::GetYear() const
{
	return (GetLocalTm(NULL)->tm_year) + 1900; 
}
int CBSTime::GetMonth() const
{
	return GetLocalTm(NULL)->tm_mon + 1; 
}
int CBSTime::GetDay() const
{
	return GetLocalTm(NULL)->tm_mday; 
}
int CBSTime::GetHour() const
{
	return GetLocalTm(NULL)->tm_hour; 
}
int CBSTime::GetMinute() const
{
	return GetLocalTm(NULL)->tm_min; 
}
int CBSTime::GetSecond() const
{
	return GetLocalTm(NULL)->tm_sec; 
}
int CBSTime::GetDayOfWeek() const
{
	return GetLocalTm(NULL)->tm_wday + 1; 
}
CBSTimeSpan CBSTime::operator-(CBSTime time) const
{
	return CBSTimeSpan(m_time - time.m_time); 
}
CBSTime CBSTime::operator-(CBSTimeSpan timeSpan) const
{
	return CBSTime(m_time - timeSpan.m_time); 
}
CBSTime CBSTime::operator+(CBSTimeSpan timeSpan) const
{
	return CBSTime(m_time + timeSpan.m_time); 
}
const CBSTime& CBSTime::operator+=(CBSTimeSpan timeSpan)
{
	m_time += timeSpan.m_time; return *this; 
}
const CBSTime& CBSTime::operator-=(CBSTimeSpan timeSpan)
{
	m_time -= timeSpan.m_time; return *this; 
}
BOOL CBSTime::operator==(CBSTime time) const
{
	return m_time == time.m_time; 
}
BOOL CBSTime::operator!=(CBSTime time) const
{
	return m_time != time.m_time; 
}
BOOL CBSTime::operator<(CBSTime time) const
{
	return m_time < time.m_time; 
}
BOOL CBSTime::operator>(CBSTime time) const
{
	return m_time > time.m_time; 
}
BOOL CBSTime::operator<=(CBSTime time) const
{
	return m_time <= time.m_time; 
}
BOOL CBSTime::operator>=(CBSTime time) const
{
	return m_time >= time.m_time; 
}

struct tm* CBSTime::GetLocalTm(struct tm* ptm) const
{
	if (ptm != NULL)
	{
		tm tmTemp;
		
		//struct tm* ptmTemp = ::localtime(&m_time);
		//if (ptmTemp == NULL)
		if ( 0 != ::localtime_s( &tmTemp, &m_time) )
			return NULL;    // indicates the m_time was not initialized!

		*ptm = tmTemp;
		return ptm;
	}
	else
	{
		ZeroMemory( (LPVOID)&m_tm, sizeof(m_tm) );

		if ( 0 != ::localtime_s( (tm*)&m_tm, &m_time) )
			return NULL;

		return (tm*)&m_tm;
	}
}

CBSTime PASCAL CBSTime::GetCurrentTime()
// return the current system time
{
	return CBSTime(::time(NULL));
}


CBSTime::CBSTime(int nYear, int nMonth, int nDay, int nHour, int nMin, int nSec, int nDST)
{
	SetTime(nYear, nMonth, nDay, nHour, nMin, nSec, nDST);
	ZeroMemory( (LPVOID)&m_tm, sizeof(m_tm) );
}

void CBSTime::SetTime(int nYear, int nMonth, int nDay, int nHour, int nMin, int nSec, int nDST)
{
	struct tm atm;
	atm.tm_sec = nSec;
	atm.tm_min = nMin;
	atm.tm_hour = nHour;
	ASSERT(nDay >= 1 && nDay <= 31);
	atm.tm_mday = nDay;
	ASSERT(nMonth >= 1 && nMonth <= 12);
	atm.tm_mon = nMonth - 1;        // tm_mon is 0 based
	ASSERT(nYear >= 1900);
	atm.tm_year = nYear - 1900;     // tm_year is 1900 based
	atm.tm_isdst = nDST;
	m_time = mktime(&atm);
	ASSERT(m_time != -1);       // indicates an illegal input time
}

CBSTime::CBSTime(WORD wDosDate, WORD wDosTime, int nDST)
{
	struct tm atm;
	atm.tm_sec = (wDosTime & ~0xFFE0) << 1;
	atm.tm_min = (wDosTime & ~0xF800) >> 5;
	atm.tm_hour = wDosTime >> 11;

	atm.tm_mday = wDosDate & ~0xFFE0;
	atm.tm_mon = ((wDosDate & ~0xFE00) >> 5) - 1;
	atm.tm_year = (wDosDate >> 9) + 80;
	atm.tm_isdst = nDST;
	m_time = mktime(&atm);
	ASSERT(m_time != -1);       // indicates an illegal input time

	ZeroMemory( (LPVOID)&m_tm, sizeof(m_tm) );
}

CBSTime::CBSTime(const SYSTEMTIME& sysTime, int nDST)
{
	if (sysTime.wYear < 1900)
	{
		time_t time0 = 0L;
		CBSTime timeT(time0);
		*this = timeT;
	}
	else
	{
		CBSTime timeT(
			(int)sysTime.wYear, (int)sysTime.wMonth, (int)sysTime.wDay,
			(int)sysTime.wHour, (int)sysTime.wMinute, (int)sysTime.wSecond,
			nDST);
		*this = timeT;
	}
}

CBSTime::CBSTime(const FILETIME& fileTime, int nDST)
{
	// first convert file time (UTC time) to local time
	FILETIME localTime;
	if (!FileTimeToLocalFileTime(&fileTime, &localTime))
	{
		m_time = 0;
		return;
	}

	// then convert that time to system time
	SYSTEMTIME sysTime;
	if (!FileTimeToSystemTime(&localTime, &sysTime))
	{
		m_time = 0;
		return;
	}

	// then convert the system time to a time_t (C-runtime local time)
	CBSTime timeT(sysTime, nDST);
	*this = timeT;
}


BOOL CBSTime::GetAsSystemTime(SYSTEMTIME& timeDest) const
{
	struct tm* ptm = GetLocalTm(NULL);
	if (ptm == NULL)
		return FALSE;

	timeDest.wYear = (WORD) (1900 + ptm->tm_year);
	timeDest.wMonth = (WORD) (1 + ptm->tm_mon);
	timeDest.wDayOfWeek = (WORD) ptm->tm_wday;
	timeDest.wDay = (WORD) ptm->tm_mday;
	timeDest.wHour = (WORD) ptm->tm_hour;
	timeDest.wMinute = (WORD) ptm->tm_min;
	timeDest.wSecond = (WORD) ptm->tm_sec;
	timeDest.wMilliseconds = 0;

	return TRUE;
}


void CBSTime::GetAsTimeStamp(STIMESTAMP& stamp) const
{
	if ( IsInvalidTime() == true )
	{
		STIMESTAMP infinite;
		stamp = infinite;
	}
	else
	{
		stamp.year = static_cast<short>( GetYear() );
		stamp.month = static_cast<WORD>( GetMonth() );
		stamp.day = static_cast<WORD>( GetDay() );
		stamp.hour = static_cast<WORD>( GetHour() );
		stamp.minute = static_cast<WORD>( GetMinute() );
		stamp.second = static_cast<WORD>( GetSecond() );
		stamp.fraction = 0;
	}
}
	
void ConvertToLocalTimeToUTC(const SimpleTime& src, SimpleTime& dst)
{
	time_t now = time(NULL);
	struct tm lcl, gmt;
	localtime_s(&lcl, &now);
	gmtime_s(&gmt, &now);
	int delta_year = lcl.tm_year - gmt.tm_year;
	int delta_yday = lcl.tm_yday - gmt.tm_yday; // 연중날짜 (0~365)
	int delta_month = (int)(delta_yday / 12);
	int delta_day = delta_yday % 12;
	int delta_hour = lcl.tm_hour - gmt.tm_hour;
	int delta_min = lcl.tm_min - gmt.tm_min;
	int delta_sec = lcl.tm_sec - gmt.tm_sec;
		
	dst.year	= src.year - delta_year;
	dst.month	= src.month - delta_month;
	dst.day		= src.day - delta_day;
	dst.hour	= src.hour - delta_hour;
	dst.min		= src.min - delta_min;
	dst.sec		= src.sec - delta_sec;
}

DWORD GetDateAbsSecond(SYSTEMTIME st)
{
	INT64 i64;
	FILETIME fst;
	SystemTimeToFileTime(&st,&fst);
	i64=(((INT64)fst.dwHighDateTime) << 32) + fst.dwLowDateTime;
	// 초 단위로 환산하고 2000년 1월 1일 자정 기준으로 바꾼다.
	i64 = i64 / 10000000 - (INT64)145731 * 86400;
	return (DWORD)i64;
}

void DateAbsSecondToSystemTime(DWORD Abs, SYSTEMTIME &st)
{
	INT64 i64;
	FILETIME fst;
	i64=(Abs + (INT64)145731 * 86400)*10000000;
	fst.dwHighDateTime = (DWORD)(i64 >> 32);
	fst.dwLowDateTime = (DWORD)(i64 & 0xffffffff);
	FileTimeToSystemTime(&fst, &st);
}

SYSTEMTIME GetElapsedTimeSinceBoot()
{
	LARGE_INTEGER now, freq;
	SYSTEMTIME stRet = {0,};

	// 부팅 후 경과한 시간을 초 단위로 구한다.
	QueryPerformanceCounter(&now);
	QueryPerformanceFrequency(&freq);
	stRet.wSecond = (WORD)(now.QuadPart / freq.QuadPart);
	stRet.wDay = static_cast<WORD>(stRet.wSecond / 86400);
	stRet.wHour = (stRet.wSecond % 86400) / 3600;
	stRet.wMinute = (stRet.wSecond % 3600) / 60;

	return stRet;
}

SYSTEMTIME GetBootTime()
{
	SYSTEMTIME stCurrent = {0,};
	SYSTEMTIME stBootElapsed = GetElapsedTimeSinceBoot();
	SYSTEMTIME stRet = {0,};
	DWORD abs = 0;

	// 현재 시간에서 경과 시간을 빼서 부팅한 시간을 구한다.
	GetLocalTime(&stCurrent);
	abs = GetDateAbsSecond(stCurrent);
	DateAbsSecondToSystemTime(abs - stBootElapsed.wSecond, stRet);

	return stRet;
}