#pragma once

template <class T> 
class Singleton
{
    static T* s_pObject;

public:
	Singleton()
	{
		_ASSERT( s_pObject == nullptr );
		ULONG_PTR offset = (ULONG_PTR)(T*)1 - (ULONG_PTR)(Singleton<T>*)(T*)1;
		s_pObject = (T*)((ULONG_PTR)this + offset);
	}
	~Singleton()
	{  
		s_pObject = 0;  
	}
	
	static T& GetSingleton()
	{  
		_ASSERT(s_pObject);
		return (*s_pObject);
	}
	
	static T* GetSingletonPtr()
	{  
		return s_pObject;
	}
};


