#pragma once

//////////////////////////////////////////
template <class T>
class IQue
{
public:
	virtual ~IQue() {};

	virtual BOOL Close() = 0;
	virtual size_t GetSize() = 0;
	virtual BOOL IsEmpty() = 0;
	virtual BOOL IsFull()  = 0;
	virtual BOOL Enque(T NewItem) = 0;
	virtual BOOL Deque(T* QueuedItem, DWORD dwTimeout = INFINITE, BOOL bForCleanup = FALSE) = 0;
	virtual BOOL Deactivate() = 0;
	virtual void Activate() = 0;
	virtual void Dump() const = 0;
};