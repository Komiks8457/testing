#pragma once

class CTickCount64
{
	LARGE_INTEGER m_Frequency;
	CTickCount64()
	{
		QueryPerformanceFrequency(&m_Frequency);	
	}

public:	

	inline static CTickCount64& GetInstance()
	{
		static CTickCount64 tick64;
		return tick64;
	}
	
	ULONGLONG GetTickCount()
	{
		// 292471208년까지 가능함.
		
		LARGE_INTEGER d;
		QueryPerformanceCounter(&d);
		
		return ULONGLONG((double)d.QuadPart/m_Frequency.QuadPart*1000.);
	}	
};

#define GetTickCount64() CTickCount64::GetInstance().GetTickCount()
