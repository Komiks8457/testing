#pragma once

/////////////////////////////////////////////////////////
// simple time
/////////////////////////////////////////////////////////
#define SIMPLETIME_YEAR_BASE (WORD)2000

struct SimpleTime
{
	union
	{
		DWORD dwTime;
		struct
		{
			DWORD year	: 6;	// 2000 ~ 2063
			DWORD month : 4;	// 0 ~ 15
			DWORD day	: 5;	// 0 ~ 31
			DWORD hour	: 5;	// 0 ~ 15
			DWORD min	: 6;	// 0 ~ 63
			DWORD sec	: 6;	// 0 ~ 63
		};
	};

	SimpleTime() { dwTime = 0; }
	SimpleTime(DWORD t) { dwTime = t; }
	SimpleTime(SYSTEMTIME& stime) 
	{
		year	= (stime.wYear - SIMPLETIME_YEAR_BASE);
		month	= stime.wMonth;
		day		= stime.wDay;
		hour	= stime.wHour;
		min		= stime.wMinute;
		sec		= stime.wSecond;
	}

	static int CompareTime(SimpleTime& t1, SimpleTime& t2)
	{
		if (t1.dwTime == t2.dwTime)
			return 0;	// t1 == t2

		if (t1.year > t2.year)
			return -1;
		else if (t1.year < t2.year)
			return +1;

		if (t1.month > t2.month)
			return -1;
		else if (t1.month < t2.month)
			return +1;

		if (t1.day > t2.day)
			return -1;
		else if (t1.day < t2.day)
			return +1;

		if (t1.hour > t2.hour)
			return -1;
		else if (t1.hour < t2.hour)
			return +1;

		if (t1.min > t2.min)
			return -1;
		else if (t1.min < t2.min)
			return +1;

		if (t1.sec > t2.sec)
			return -1;
		else if (t1.sec < t2.sec)
			return +1;

		return 0;
	}

	void SetCurLocalTime()
	{
		SYSTEMTIME stime;
		::GetLocalTime(&stime);

		year	= (stime.wYear - SIMPLETIME_YEAR_BASE);
		month	= stime.wMonth;
		day		= stime.wDay;
		hour	= stime.wHour;
		min		= stime.wMinute;
		sec		= stime.wSecond;
	}

	void ConvertTimeTo(SYSTEMTIME& stime) const
	{
		stime.wYear		= (WORD)(year + SIMPLETIME_YEAR_BASE);
		stime.wMonth	= (WORD)month;
		stime.wDay		= (WORD)day;
		stime.wHour		= (WORD)hour;
		stime.wMinute	= (WORD)min;
		stime.wSecond	= (WORD)sec;
	}
};