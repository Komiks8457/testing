#pragma once

class CSafeCount
{
	public:
	explicit CSafeCount(long count = 0) : m_nCount(count) {}

	long operator++() { return InterlockedIncrement(&m_nCount); }
	long operator--() { return InterlockedDecrement(&m_nCount); }

	operator long() const { return m_nCount; }

private:
	CSafeCount(const CSafeCount&);
	CSafeCount& operator=(const CSafeCount&);

	long m_nCount;
};

#ifndef _BS_MULTI_THREADS
	typedef long		SafeCounter;
#else
	typedef CSafeCount	SafeCounter;
#endif
