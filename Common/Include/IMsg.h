#pragma once

#define MSG_HEADER_SIZE 15

enum MsgException
{
	MsgReadException = 1,
	MsgReverseReadException,
	MsgTypeReadException,
	MsgWriteException,
	MsgOverwriteException,
};

class __declspec(novtable) IMsg
{
public:
	virtual void	SetBuffer(BYTE* pBuffer, DWORD cap) = 0;
	virtual void	Resize(DWORD wSize) = 0;
	virtual WORD	GetMsgID() = 0;
	virtual DWORD	GetDataSize() = 0;
	virtual void	SetDataSizeAtHeader(DWORD wSize) = 0;
	virtual DWORD	GetRdPos() = 0;
	virtual DWORD	GetWrPos() = 0;
	virtual void	IncRdPos(DWORD Pos) = 0;
	virtual void	IncWrPos(DWORD Pos) = 0;
	virtual void	DecRdPos(DWORD Pos) = 0;
	virtual void	DecWrPos(DWORD Pos) = 0;
	virtual DWORD	GetRemainBytesToRead() = 0;

	// buffer offset
	virtual BOOL	SetRdPos(DWORD wPos, BOOL bIsOffsetFromHeader = FALSE) = 0;
	virtual BOOL	SetWrPos(DWORD wPos, BOOL bIsOffsetFromHeader = FALSE) = 0;

	// diagnostics
	virtual BOOL	IsEncMsg() = 0;
	virtual void	SetAsEncrypted() = 0;
	virtual void	SetAsPlaneMsg() = 0;

	// buffer pointer
	virtual void	ReadBytes(void* pBuf, DWORD nLen) = 0;
	virtual void	WriteBytes(void* pBuf, DWORD nLen) = 0;
	virtual void	ReadBytesReverse(void* pBuf, DWORD nLen) = 0;

	virtual BYTE*	GetRdBuffer() = 0;
	virtual BYTE*	GetWrBuffer() = 0;

	virtual BYTE*	GetBufferAt(DWORD nOffset) = 0;

	virtual void	Reset(long) = 0;

	virtual DWORD	CopyMsg(IMsg* pSrc, BOOL bUnreadOnly = TRUE) = 0;

	virtual BOOL	IsReadForward() const = 0;

	template <class T>
	void ReadBytesReverseEx(T& arg)
	{
		DWORD nLen = sizeof(T);
		if (nLen > (GetWrPos() - MSG_HEADER_SIZE))
			throw MsgReverseReadException;

		DecWrPos(nLen);
		arg = *((T*)GetWrBuffer());

		SetDataSizeAtHeader(GetWrPos() - MSG_HEADER_SIZE);
	}

	template <class T>
	void ReadBytesEx(T& arg)
	{
		DWORD nLen = sizeof(T);
		if ((GetRdPos() + nLen) > GetWrPos())
			throw MsgReadException;

		arg = *((T*)GetRdBuffer());
		IncRdPos(nLen);
	}

	template <class T>
	void Overwrite(T arg, DWORD nPos)
	{
		if ((nPos + sizeof(T)) > GetWrPos())
			throw MsgOverwriteException;

		*((T*)(GetBufferAt(nPos))) = arg;
	}

	template <class T>
	IMsg& operator << (const T& arg)
	{
		WriteBytes((void*)&arg, sizeof(T));
		return *this;
	}

	template <class T>
	IMsg& operator >> (T& arg)
	{
		if (IsReadForward())
			ReadBytesEx(arg);
		else
			ReadBytesReverseEx(arg);

		return *this;
	}

	IMsg& operator << (const char* rhs)
	{
		DWORD wLen = (rhs == NULL) ? (0) : DWORD(::strlen(rhs));
		WriteBytes(&wLen, sizeof(DWORD));

		if (wLen < 1)
			return *this;

		WriteBytes((void*)rhs, wLen);
		return *this;
	}

	IMsg& operator << (const wchar_t* rhs)
	{
		DWORD wLen = (rhs == NULL) ? (0) : DWORD(::lstrlenW(rhs));
		WriteBytes(&wLen, sizeof(DWORD));

		if (wLen < 1)
			return *this;

		WriteBytes( (void*)rhs, wLen*sizeof(WCHAR) );
		return *this;
	}

	//--------------------------------------------------------------	
	template <>
	IMsg& operator << (const std::wstring& wstr)
	{
		return *this << wstr.c_str();
	}

	template <>
	IMsg& operator >> (std::wstring& wstrString)
	{
		DWORD wLen = 0;
		if (IsReadForward())
		{
			ReadBytes(&wLen, sizeof(DWORD));

			if (wLen > 0)
			{	
				if (wLen >= GetRemainBytesToRead())
					wLen = GetRemainBytesToRead();

				wstrString.resize( wLen );
				ReadBytes( (void*)wstrString.c_str(), wLen*sizeof(WCHAR) );
			}
			else
			{
				wstrString.clear();
			}
		}
		else
		{
			_ASSERT(FALSE);
		}
		return *this;
	}
	//--------------------------------------------------------------

	template <>
	IMsg& operator << (const std::string& str)
	{
		return *this << str.c_str();
	}

	template <>
	IMsg& operator >> (std::string& strString)
	{
		DWORD wLen = 0;
		if (IsReadForward())
		{
			ReadBytes(&wLen, sizeof(DWORD));

			if (wLen > 0)
			{	
				if (wLen >= GetRemainBytesToRead())
					wLen = GetRemainBytesToRead();

				strString.resize(wLen);
				ReadBytes((void*)strString.c_str(), wLen);
			}
			else
			{
				strString.clear();
			}
		}
		else
		{
			_ASSERT(FALSE);
		}
		return *this;
	}

#define _GET_TYPED_VALUE(type)				\
	DWORD nLen = sizeof(type);				\
	if ((GetRdPos() + nLen) > GetWrPos())	\
		throw MsgTypeReadException;			\
	type temp = *((type*)GetRdBuffer());	\
	IncRdPos(nLen);							\
	return temp;

	operator char()		{ _GET_TYPED_VALUE(char); }
	operator BYTE()		{ _GET_TYPED_VALUE(BYTE); }
	operator short()	{ _GET_TYPED_VALUE(short); }
	operator WORD()		{ _GET_TYPED_VALUE(WORD); }
	operator long()		{ _GET_TYPED_VALUE(long); }
	operator LONGLONG() { _GET_TYPED_VALUE(LONGLONG); }
	operator DWORD()	{ _GET_TYPED_VALUE(DWORD); }
	operator int()		{ _GET_TYPED_VALUE(int);}
	operator ULONGLONG(){ _GET_TYPED_VALUE(ULONGLONG); }
	operator float()	{ _GET_TYPED_VALUE(float); }
	operator double()	{ _GET_TYPED_VALUE(double); }
};
