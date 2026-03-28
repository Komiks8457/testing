#pragma once

/*
template<class _Ty, int _Align>
class bsalloc
{
public:
	typedef _Ty	alloc_type;

	_Ty* alloc(size_t size) 
	{ 
		return (alloc_type*)::_mm_malloc(size, _Align);
	}
	void free(alloc_type* p) 
	{ 
		::_mm_free(p);
	}
};

template<class _Ty, int _Align>
class bsallocator : public alloc
{
	typedef _Ty*	pointer;
	typedef DWORD	size_type;

public:
	static pointer allocate(size_type _N)
	{
		return (pointer)_alloc.alloc( (size_t)_N * sizeof(_Ty) );
	}

	static char *_Charalloc(size_type _N)
	{
		return (char *)_alloc.alloc( (difference_type)_N);
	}

	static void deallocate(void *_P, size_type)
	{
		_alloc.free((pointer)_P);
	}

	static void destroy(pointer _P)
	{
		allocator<_Ty>::destroy(_P); 
	}

protected:
	static bsalloc<_Ty, _Align> _alloc;
};

template <class _T, int _Align> bsalloc<_T, _Align>	bsallocator<_T, _Align>::_alloc;
*/