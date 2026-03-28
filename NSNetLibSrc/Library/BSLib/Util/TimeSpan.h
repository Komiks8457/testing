#pragma once

struct STIMESTAMP;

class CBSTimeSpan
{
public:

// Constructors
	CBSTimeSpan();
	CBSTimeSpan(time_t time);
	CBSTimeSpan(LONG lDays, int nHours, int nMins, int nSecs);

	CBSTimeSpan(const CBSTimeSpan& timeSpanSrc);
	const CBSTimeSpan& operator=(const CBSTimeSpan& timeSpanSrc);

// Attributes
	// extract parts
	LONG GetDays() const;   // total # of days
	LONG GetTotalHours() const;
	int GetHours() const;
	LONG GetTotalMinutes() const;
	int GetMinutes() const;
	LONG GetTotalSeconds() const;
	int GetSeconds() const;

// Operations
	// time math
	CBSTimeSpan operator-(CBSTimeSpan timeSpan) const;
	CBSTimeSpan operator+(CBSTimeSpan timeSpan) const;
	const CBSTimeSpan& operator+=(CBSTimeSpan timeSpan);
	const CBSTimeSpan& operator-=(CBSTimeSpan timeSpan);
	BOOL operator==(CBSTimeSpan timeSpan) const;
	BOOL operator!=(CBSTimeSpan timeSpan) const;
	BOOL operator<(CBSTimeSpan timeSpan) const;
	BOOL operator>(CBSTimeSpan timeSpan) const;
	BOOL operator<=(CBSTimeSpan timeSpan) const;
	BOOL operator>=(CBSTimeSpan timeSpan) const;

private:
	time_t m_time;
	friend class CBSTime;
};

class CBSTime
{
public:

// Constructors
	static CBSTime PASCAL GetCurrentTime();
	
	CBSTime();
	CBSTime(time_t time);
	CBSTime(int nYear, int nMonth, int nDay, int nHour, int nMin, int nSec, int nDST = -1);
	CBSTime(WORD wDosDate, WORD wDosTime, int nDST = -1);
	CBSTime(const CBSTime& timeSrc);

	CBSTime(const SYSTEMTIME& sysTime, int nDST = -1);
	CBSTime(const FILETIME& fileTime, int nDST = -1);
	const CBSTime& operator=(const CBSTime& timeSrc);
	const CBSTime& operator=(time_t t);

// Attributes
	struct tm* GetGmtTm(struct tm* ptm = NULL) const;
	struct tm* GetLocalTm(struct tm* ptm = NULL) const;
	BOOL GetAsSystemTime(SYSTEMTIME& timeDest) const;
	void GetAsTimeStamp(STIMESTAMP& stamp) const;

	time_t GetTime() const;
	int GetYear() const;
	int GetMonth() const;       // month of year (1 = Jan)
	int GetDay() const;         // day of month
	int GetHour() const;
	int GetMinute() const;
	int GetSecond() const;
	int GetDayOfWeek() const;   // 1=Sun, 2=Mon, ..., 7=Sat

	void SetTime(int nYear, int nMonth, int nDay, int nHour, int nMin, int nSec, int nDST = -1);
	void SetAsInvalidTime() { m_time = -1; }
	bool IsInvalidTime() const { return (m_time == -1); }

// Operations
	// time math
	CBSTimeSpan operator-(CBSTime time) const;
	CBSTime operator-(CBSTimeSpan timeSpan) const;
	CBSTime operator+(CBSTimeSpan timeSpan) const;
	const CBSTime& operator+=(CBSTimeSpan timeSpan);
	const CBSTime& operator-=(CBSTimeSpan timeSpan);
	BOOL operator==(CBSTime time) const;
	BOOL operator!=(CBSTime time) const;
	BOOL operator<(CBSTime time) const;
	BOOL operator>(CBSTime time) const;
	BOOL operator<=(CBSTime time) const;
	BOOL operator>=(CBSTime time) const;

	// formatting using "C" strftime
//	std::tstring Format(LPCSTR pFormat) const;
//	std::tstring FormatGmt(LPCSTR pFormat) const;
//	std::tstring Format(UINT nFormatID) const;
//	std::tstring FormatGmt(UINT nFormatID) const;

private:
	tm		m_tm;
	time_t	m_time;
};


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
struct STIMESTAMP
{
	enum eThreshold
	{
		eTimeThresholdDay,
		eTimeThresholdHour,
		eTimeThresholdMinute,
		eTimeThresholdSecond
	};

	short	year;
	WORD	month;
	WORD	day;
	WORD	hour;
	WORD	minute;
	WORD	second;
	DWORD	fraction;	// doesn't used

	STIMESTAMP() : year(1900), month(1), day(1), hour(0), minute(0), second(0), fraction(0) {}

	friend BOOL operator == (const STIMESTAMP& arg1, const STIMESTAMP& arg2)
	{
		return (arg1.year == arg2.year && arg1.month == arg2.month && arg1.day == arg2.day &&
				arg1.hour == arg2.hour && arg1.minute == arg2.minute && arg1.second == arg2.second);	// millisec 이하는 무시!
	}

	friend BOOL operator != (const STIMESTAMP& arg1, const STIMESTAMP& arg2)
	{
		return (arg1.year != arg2.year || arg1.month != arg2.month || arg1.day != arg2.day ||
				arg1.hour != arg2.hour || arg1.minute != arg2.minute || arg1.second != arg2.second);	// millisec 이하는 무시!
	}

	friend BOOL operator > (const STIMESTAMP& arg1, const STIMESTAMP& arg2)
	{
		if(arg1.year > arg2.year)
		{
			return true;
		}
		else if(arg1.year < arg2.year)
		{
			return false;
		}

		if(arg1.month > arg2.month)
		{
			return true;
		}
		else if(arg1.month < arg2.month)
		{
			return false;
		}

		if(arg1.day > arg2.day)
		{
			return true;
		}
		else if(arg1.day < arg2.day)
		{
			return false;
		}

		if(arg1.hour > arg2.hour)
		{
			return true;
		}
		else if(arg1.hour < arg2.hour)
		{
			return false;
		}

		if(arg1.minute > arg2.minute)
		{
			return true;
		}
		else if(arg1.minute < arg2.minute)
		{
			return false;
		}

		if(arg1.second > arg2.second)
		{
			return true;
		}
		else if(arg1.second < arg2.second)
		{
			return false;
		}

		return false;
	}


	friend BOOL operator < (const STIMESTAMP& arg1, const STIMESTAMP& arg2)
	{
		if(arg1.year < arg2.year)
		{
			return true;
		}
		else if(arg1.year > arg2.year)
		{
			return false;
		}

		if(arg1.month < arg2.month)
		{
			return true;
		}
		else if(arg1.month > arg2.month)
		{
			return false;
		}

		if(arg1.day < arg2.day)
		{
			return true;
		}
		else if(arg1.day > arg2.day)
		{
			return false;
		}

		if(arg1.hour < arg2.hour)
		{
			return true;
		}
		else if(arg1.hour > arg2.hour)
		{
			return false;
		}

		if(arg1.minute < arg2.minute)
		{
			return true;
		}
		else if(arg1.minute > arg2.minute)
		{
			return false;
		}

		if(arg1.second < arg2.second)
		{
			return true;
		}
		else if(arg1.second > arg2.second)
		{
			return false;
		}

		return false;
	}

	void SetCurLocalTime()
	{
		SYSTEMTIME stime;
		::GetLocalTime(&stime);

		year	= stime.wYear;
		month	= stime.wMonth;
		day		= stime.wDay;
		hour	= stime.wHour;
		minute	= stime.wMinute;
		second	= stime.wSecond;
		fraction= 0;
	}
	
	// @redpixel STIMESTAMP -> time_t 변환을 쉽게 하기 위해 추가
	void GetAsStructTM(struct tm &tmp_tm)
	{
		// tm_wday, tm_yday는 설정하지 않아도 mktime() 함수 실행가능
		tmp_tm.tm_year = year - 1900;	
		tmp_tm.tm_mon = month - 1;		
		tmp_tm.tm_mday = day;
		tmp_tm.tm_hour = hour;
		tmp_tm.tm_min = minute;
		tmp_tm.tm_sec = second;
		tmp_tm.tm_isdst = -1;
	}
	
	void MakeTimeString(std::wstring& strTime) const
	{
		TCHAR buf[256];
		if(SF_sprintf(buf, 256, _T("%d-%d-%d %d:%d:%d"), year, month, day, hour, minute, second) == S_OK)
			strTime = (LPCTSTR)buf;
		else
		{
			_ASSERT(FALSE);
		}
	}

	void MakeTimeString2(std::wstring& strTime) const
	{
		TCHAR buf[256];
		if(SF_sprintf(buf, 256, _T("%d-%02d-%02d %02d:%02d:%02d"), year, month, day, hour, minute, second) == S_OK)
			strTime = (LPCTSTR)buf;
		else
		{
			_ASSERT(FALSE);
		}
	}

	int DateDiff( STIMESTAMP& sTime, eThreshold eResultThreshold )
	{
		CBSTime sTime1( year,		month,			day,		hour,		minute,			second );
		CBSTime sTime2( sTime.year,	sTime.month,	sTime.day,	sTime.hour,	sTime.minute,	sTime.second );

		CBSTimeSpan sTimeSpan = sTime2 - sTime1;

		switch ( eResultThreshold )
		{
		case eTimeThresholdDay:			return sTimeSpan.GetDays();
		case eTimeThresholdHour:		return sTimeSpan.GetTotalHours();
		case eTimeThresholdMinute:		return sTimeSpan.GetTotalMinutes();
		case eTimeThresholdSecond:		return sTimeSpan.GetTotalSeconds();
		}

		return 0;
	}

	int DateDiffWithCurTime( eThreshold eResultThreshold )
	{
		STIMESTAMP sNow;
		sNow.SetCurLocalTime();

		return DateDiff( sNow, eResultThreshold );
	}

	bool IsInfinite() const
	{
		return (year == 1900);
	}

	// @redpixel DB 필드로서 받았을 경우 NULL 값이면 모두 0으로 설정되어있다. DB 필드일때만 사용.
	bool IsNull() const
	{
		return (year == 0 && month == 0 && day == 0 && hour == 0 && minute == 0 && second == 0 && fraction == 0);
	}
};

static void ConvertBSTimeToTimeStamp(const CBSTime& time, STIMESTAMP& stamp)
{
	if (time.IsInvalidTime())
	{
		STIMESTAMP infinite;
		stamp = infinite;
	}
	else
	{
		stamp.year = static_cast<short>(time.GetYear());
		stamp.month = static_cast<WORD>(time.GetMonth());
		stamp.day = static_cast<WORD>(time.GetDay());
		stamp.hour = static_cast<WORD>(time.GetHour());
		stamp.minute = static_cast<WORD>(time.GetMinute());
		stamp.second = static_cast<WORD>(time.GetSecond());
		stamp.fraction = 0;
	}
}

static void SetTimeTo( SimpleTime& st, const STIMESTAMP& sts )
{
	if (sts.IsInfinite()) 
	{ 
		st.dwTime = 0; 
	}
	else
	{
		st.year = sts.year - SIMPLETIME_YEAR_BASE;
		st.month = sts.month;
		st.day = sts.day;
		st.hour = sts.hour;
		st.min = sts.minute;
		st.sec = sts.second;
	}
}

static void SetTimeTo( STIMESTAMP& sts, const SimpleTime& st )
{
	if (st.dwTime == 0) 
	{
		STIMESTAMP inf;
		sts = inf;
	}
	else
	{
		sts.year = static_cast<short>(st.year + SIMPLETIME_YEAR_BASE);
		sts.month = st.month;
		sts.day = st.day;
		sts.hour = st.hour;
		sts.minute = st.min;
		sts.second = st.sec;
		sts.fraction = 0;
	}
}

static void ConvertTimeStampToBSTime(const STIMESTAMP& stamp, CBSTime& time)
{
	if (stamp.IsInfinite())
		time.SetAsInvalidTime();
	else
		time.SetTime(stamp.year, stamp.month, stamp.day, stamp.hour, stamp.minute, stamp.second);
}

static void ConvertTimeStampToTimeT( const STIMESTAMP& stamp, time_t& time )
{
	CBSTime bsTime;
	ConvertTimeStampToBSTime( stamp, bsTime );
	time = bsTime.GetTime();
}

void ConvertToLocalTimeToUTC(const SimpleTime& src, SimpleTime& dst);

DWORD GetDateAbsSecond(SYSTEMTIME st);
void DateAbsSecondToSystemTime(DWORD Abs, SYSTEMTIME &st);
SYSTEMTIME GetElapsedTimeSinceBoot();
SYSTEMTIME GetBootTime();
