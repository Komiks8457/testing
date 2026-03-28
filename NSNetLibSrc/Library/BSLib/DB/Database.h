#pragma once

#include "adodb.h"
#include "DB_Common.h"
#include "../thread/synchobject.h"
#include <sqltypes.h>
#include <ppl.h>
#include <stack>
#include <list>

#define DB_MAX_CONNECTIONS 32767

class Database
{
public:
	class Session : public ADODB
	{
	public:
		Session(DWORD number, const ADODB::ErrorReporter& error_reporter) : number_(number)
		{
			set_error_reporter(error_reporter);
		}

		DWORD number() const { return number_; }

	private:
		DECLARE_OBJECT_POOL(Session, ChunkAllocatorMT)

		DWORD number_;
	};

	Database();
	~Database();

	void Finalize();

	bool Connect(const wchar_t* connection_string, DWORD timeout = 0);
	void SetDatabaseProcessLog(bool is_database_process_log, DWORD query_profile_tick)
	{
		is_database_process_log_ = is_database_process_log;
		query_profile_tick_ = query_profile_tick;
	}

	bool ExecuteQuery(SQLQuery& query);

	Session* AllocSession(DWORD& reconnect_count);
	void FreeSession(Session& session, bool destroy = false);

	DWORD session_count() const { return session_count_; }
	void set_error_reporter(const ADODB::ErrorReporter& error_reporter) { error_reporter_ = error_reporter; }

private:
	CCriticalSectionBS cs_;
	ADODB::ErrorReporter error_reporter_;
	std::wstring connection_string_;
	volatile long session_count_;
	long session_number_;
	ADODB session_;
	bool is_database_process_log_;
	DWORD query_profile_tick_;
	std::stack<Session*> session_pool_;

};

//////////////////////////////////////////////////////////////////////////
// CDBTable
//////////////////////////////////////////////////////////////////////////
#define QUERY_BUFFER_LEN			65536

class CDBRecord;
class _record_allocator;

typedef std::list<CDBRecord*>		listRECORDS;

enum LOCK_LEVEL
{
	SELECT_LOCKLEVEL_READ_COMMITTED,
	SELECT_LOCKLEVEL_READ_UNCOMMITTED,

	SELECT_LOCKLEVEL_NUM
};

#define QUERY_CALL_STOREDPROCEDURE		0x00000001
#define QUERY_HAS_RETURN_VALUE_4BYTE	0x00000002
#define QUERY_HAS_RETURN_VALUE_8BYTE	0x00000004

#define UPDATE_ALL_MODIFIED_GROUP		-1

struct DQExParam
{
	DQExParam()
	{
		param_type		 = DATA_TYPE_UNKNOWN;
		param_inout_type = ADODB::ParameterInput; // #define SQL_PARAM_TYPE_UNKNOWN           0 (defined in sqlext.h)
		bound_value.ll	 = 0;
		LenInd = 0;
		size = 0;
	}

	DATA_TYPE	param_type;
	ADODB::ParameterDirection param_inout_type;
	BSVAR		bound_value;
	int			size;
	long		LenInd;
};
typedef std::list<DQExParam>	QUERY_PARAMS;

class CDBTable  
{
public:
	explicit CDBTable(DWORD id = 0);
	~CDBTable();

protected:
	DWORD id_;
	Database*		database_;
	TabDesc*		m_pTabDesc;
	_record_allocator*	m_pAllocator;

	bool		(CDBTable::*LPF_LOADER)(SQLQuery& query, listRECORDS* pResultSet);

public:
	void		Reset();
	bool		Create(Database* database, TabDesc* pTabDesc, _record_allocator* pAllocator = NULL);
		
	CDBRecord*	AllocNewRecord();
	void		FreeRecord(CDBRecord* pRecord);
	void		FreeRecords(listRECORDS& pRecordset);

	bool		LoadAllRecords(listRECORDS* pResultSet);
	bool		SelectCustomCondition(listRECORDS* pResultSet, SQLDynamicQuery& Condition, LOCK_LEVEL LockLevel = SELECT_LOCKLEVEL_READ_COMMITTED);
	bool		SelectRecordsWhere(listRECORDS* pResultSet, SQLDynamicQuery& Where, LOCK_LEVEL LockLevel = SELECT_LOCKLEVEL_READ_COMMITTED);
	bool		SelectRecords(listRECORDS* pResultSet, LOCK_LEVEL LockLevel, int KeyCount, ...);
	bool		DeleteRecords(int KeyCount, ...);
	bool		UpdateRecord(CDBRecord* pRecord, int nGroup);
	bool		InsertRecord(CDBRecord* pRecord, void* lpIdentity, int identity_size);
	bool		BuildInsertQuery(CDBRecord* pRecord, SQLInsertQuery& query);
	
	TabDesc*	GetTabDesc() { return m_pTabDesc; }
	LPCTSTR		GetTabName() { return m_pTabDesc->strTabName.c_str(); }

	void		GetUsageInfo(DWORD& dwAllocated, DWORD& dwInUse);

	DWORD id() const { return id_; }
		
protected:
	bool		_BulkLoad_PartialColumns(SQLQuery& query, listRECORDS* pResultSet);
	bool		_BulkLoad_AllColumns(SQLQuery& query, listRECORDS* pResultSet);

	bool		_RetrieveTableAnatomy();
	bool		_CheckKeyCount(int KeyCount);
	void		_BuildQuery(TabDesc& tab_desc, size_t TotalColNumOfDBTable);

};

//////////////////////////////////////////////////////////////////////////
// CDBRecord
//////////////////////////////////////////////////////////////////////////
class CDBRecord : public CBase
{
	BS_DECLARE_DYNAMIC(CDBRecord);

public:
	CDBRecord()
	{
		m_pAssocTable = NULL;
		m_DirtyGroups = 0;
		ZeroMemory(m_DirtyGroupArray, (sizeof(ULONGLONG) * MAX_DATAGROUP_ARRAY_SIZE) * MAX_DATAGROUP_COUNT);
	}

	virtual ~CDBRecord() {}

protected:
	ULONGLONG	m_DirtyGroups;
	ULONGLONG	m_DirtyGroupArray[MAX_DATAGROUP_COUNT][MAX_DATAGROUP_ARRAY_SIZE];
	CDBTable*	m_pAssocTable;

public:
	static void Release( CDBRecord* pRecord );
	static void Release( listRECORDS& records );

	virtual CDBRecord*	Spawn();

public:
	void		SetOwnerTable(CDBTable* pTab) { m_pAssocTable = pTab; }
	CDBTable*	GetOwnerTable() const { return m_pAssocTable; }

	bool IsDirtyGroup(int nGroup) 
	{
		return ((m_DirtyGroups & ULONGLONG(ULONGLONG(1) << nGroup)) != 0); 
	}

	bool IsDirtyGroup(int nGroup, unsigned short idx)
	{
		return ((m_DirtyGroupArray[nGroup][idx / MAX_DATAGROUP_COUNT] & ULONGLONG(ULONGLONG(1) << (idx % MAX_DATAGROUP_COUNT))) != 0);
	}

	void ResetAllDirtyFlag()
	{
		for (BYTE group = 0; m_DirtyGroups != 0; ++group, m_DirtyGroups >>= 1)
		{
			if (m_DirtyGroups & 1)
				ZeroMemory(m_DirtyGroupArray[group], sizeof(ULONGLONG) * MAX_DATAGROUP_ARRAY_SIZE);
		}
	}

	void ResetGroupDirtyFlag(int group)
	{
		if (group < 0 || group >= MAX_DATAGROUP_COUNT)
		{
			_ASSERT(FALSE);
			return;
		}

		ULONGLONG Mask = ULONGLONG(0xffffffffffffffff) - ((ULONGLONG)(ULONGLONG(1) << group));
		m_DirtyGroups &= Mask;
		ZeroMemory(m_DirtyGroupArray[group], sizeof(ULONGLONG) * MAX_DATAGROUP_ARRAY_SIZE);
	}

	void ResetGroupDirtyFlag(int group, int idx)
	{
		if (group < 0 || group >= MAX_DATAGROUP_COUNT || idx < 0 || idx >= MAX_COLUMN_COUNT)
		{
			_ASSERT(FALSE);
			return;
		}

		ULONGLONG Mask = ULONGLONG(0xffffffffffffffff) - ((ULONGLONG)(ULONGLONG(1) << (idx % MAX_DATAGROUP_COUNT)));
		ULONGLONG& dirty_group_array = m_DirtyGroupArray[group][idx / MAX_DATAGROUP_COUNT];
		dirty_group_array &= Mask;
		if (dirty_group_array == 0)
		{
			Mask = ULONGLONG(0xffffffffffffffff) - ((ULONGLONG)(ULONGLONG(1) << group));
			m_DirtyGroups &= Mask;
		}
	}
	
	void SetGroupDirtyFlag(int group)
	{
		_ASSERT(group >= 0 && group < MAX_DATAGROUP_COUNT);
		m_DirtyGroups |= ULONGLONG(ULONGLONG(1) << group);
	}

	void SetGroupDirtyFlag(int group, int idx)
	{
		_ASSERT(group >= 0 && group < MAX_DATAGROUP_COUNT && idx >= 0 && idx < MAX_COLUMN_COUNT);
		m_DirtyGroupArray[group][idx / MAX_DATAGROUP_COUNT] |= ULONGLONG(ULONGLONG(1) << (idx % MAX_DATAGROUP_COUNT));
		m_DirtyGroups |= ULONGLONG(ULONGLONG(1) << group);
	}

	bool HasDirtyDataGroup() const
	{
		return (m_DirtyGroups != 0);
	}

	virtual LPCTSTR		GetTableName()	= 0;
	virtual TabDesc*	GetTabDesc()	= 0;
	virtual void		Init(size_t Data)
	{
		Reset();
	}

	virtual void		Reset()
	{
		ResetAllDirtyFlag();
	};

	virtual BYTE*		GetRecordBuffer() 
	{ 
		BYTE* pThis = (BYTE*)this;
		return (pThis + GetTabDesc()->dwSizeOfParentRecordObj);
	}

	virtual void		SetColumnData(ColDesc* col_desc, BYTE* pSrcBuffer)
	{
		switch (col_desc->type)
		{
		case DATA_TYPE_STRING:
			{
				std::string* pstr = (std::string*)(GetRecordBuffer() + col_desc->offset);
				pstr->assign((LPCSTR)pSrcBuffer);
			}
			break;
		case DATA_TYPE_WIDE_STRING:
			{
				std::wstring* pstr = (std::wstring*)(GetRecordBuffer() + col_desc->offset);
				pstr->assign((LPCWSTR)pSrcBuffer);
			}
			break;
		default:
			BYTE* buffer = GetRecordBuffer() + col_desc->offset;
			::memcpy( buffer, pSrcBuffer, col_desc->col_size );
		}
	}

	bool BuildUpdateQuery(int nGroup, SQLBuildQuery& query, bool bResetDirtyFlags = true);
	
protected:
	virtual void		CopyData(CDBRecord* pTarget)
	{
		ASSERTA1( FALSE, _T("have to override db record copy functor! table_name[%s]"), GetTableName() );
	}
};

//////////////////////////////////////////////////////////////////////////
// CDBRecord
//////////////////////////////////////////////////////////////////////////
class CDBRecStream : public CDBRecord
{
	enum { STR_HEADER_SIZE = sizeof(WORD) + sizeof(WORD) };

	BS_DECLARE_DYNAMIC(CDBRecStream);
public:
	CDBRecStream()
	{
		Reset();

		m_Capacity = 0;
		m_pBuffer  = NULL;
	}

	virtual ~CDBRecStream()
	{
		SAFE_DELVEC(m_pBuffer);
		
		Reset();

		m_Capacity = 0;
	}

protected:
	BYTE*	m_pBuffer;
	long	m_nROffset;
	long	m_nWOffset;
	DWORD	m_Capacity;
	long	m_nCurColumn;

protected:
	void	WriteBytes(void* pBuf, WORD nLen)
	{
		_ASSERT(m_pBuffer != NULL);

		if (DWORD(m_nWOffset + nLen) > m_Capacity)	
		{
			_ASSERT(FALSE);
			throw;
		}

		::memcpy(&m_pBuffer[m_nWOffset], pBuf, nLen);

		m_nWOffset += nLen;
	}

	void	ReadBytes(void* pBuf, WORD nLen)
	{
		if ((m_nROffset + nLen) > m_nWOffset)
		{
			_ASSERT(FALSE);
			throw;
		}
		
		::memcpy(pBuf, &m_pBuffer[m_nROffset], nLen);

		m_nROffset += nLen;
	}

public:
	virtual BYTE*		GetRecordBuffer()  { _ASSERT(m_pBuffer != NULL); return m_pBuffer; }
	virtual void		SetColumnData(ColDesc* col_desc, BYTE* pSrcBuffer) override
	{
		switch (col_desc->type)
		{
		case DATA_TYPE_STRING:
			{
				StringTrimRight( (LPSTR)pSrcBuffer, col_desc->col_size );
				size_t length = ::lstrlenA( (LPCSTR)pSrcBuffer );
				_ASSERT( length < 0xffff);

				WORD wStrLen = (WORD)length;
				WORD wColLen = (WORD)col_desc->col_size;

				wStrLen = (wStrLen > (col_desc->col_size - STR_HEADER_SIZE)) ? (WORD)((col_desc->col_size - STR_HEADER_SIZE)) : wStrLen;
				WriteBytes(&wStrLen, sizeof(WORD));
				WriteBytes(&wColLen, sizeof(WORD));

				BYTE* pPaddingBuffer = NULL;
				DWORD PaddingSize = wColLen - STR_HEADER_SIZE;
				if (wStrLen == 0)
				{
					_ASSERT(PaddingSize > 0);
					pPaddingBuffer = new BYTE[PaddingSize];
					::ZeroMemory(pPaddingBuffer, PaddingSize);

					pSrcBuffer = pPaddingBuffer;
				}

				// 2번째 인자로 wLen을 사용하지 않는 이유는, 차후에 ++ 를 이용해서 컬럼 건너뛰기를 할때
				// 데이터 위치가 틀어지는 것을 방지하기 위한 것이다. ++ 가 동작할때엔 static 데이터를 사용하기때문에
				// 정확히 Offset이 DB Table과 일치하는 곳에 각각의 데이터들이 들어있어야 하지않겠냐...
				// -2 해준 이유는... ^^; 좀 구리긴한데... 일단은... wLen 사이즈만큼 까준거다... 이렇게 안하면...
				// wLen은 사실 부가적인 데이터기때문에 결국 Capacity를 초과해서 ASSERT 뜨거덩...
				WriteBytes((void*)&pSrcBuffer[0], (WORD)PaddingSize);

				if (pPaddingBuffer != NULL)
				{
					delete [] pPaddingBuffer;
					pPaddingBuffer = NULL;
				}
			}
			break;
		case DATA_TYPE_WIDE_STRING:
			{
				WStringTrimRight( (LPTSTR)pSrcBuffer, (col_desc->col_size/sizeof(TCHAR)) );
				size_t length = ::lstrlen( (LPCTSTR)pSrcBuffer );
				_ASSERT( length < 0xffff);

				WORD wStrLen = (WORD)length;
				WORD wColLen = (WORD)col_desc->col_size;

				wStrLen = (wStrLen > (col_desc->col_size - STR_HEADER_SIZE)) ? (WORD)((col_desc->col_size - STR_HEADER_SIZE)) : wStrLen;
				WriteBytes(&wStrLen, sizeof(WORD));
				WriteBytes(&wColLen, sizeof(WORD));

				BYTE* pPaddingBuffer = NULL;
				DWORD PaddingSize = wColLen - STR_HEADER_SIZE;
				if (wStrLen == 0)
				{
					_ASSERT(PaddingSize > 0);
					pPaddingBuffer = new BYTE[PaddingSize];
					::ZeroMemory(pPaddingBuffer, PaddingSize);

					pSrcBuffer = pPaddingBuffer;
				}

				// 2번째 인자로 wLen을 사용하지 않는 이유는, 차후에 ++ 를 이용해서 컬럼 건너뛰기를 할때
				// 데이터 위치가 틀어지는 것을 방지하기 위한 것이다. ++ 가 동작할때엔 static 데이터를 사용하기때문에
				// 정확히 Offset이 DB Table과 일치하는 곳에 각각의 데이터들이 들어있어야 하지않겠냐...
				// -2 해준 이유는... ^^; 좀 구리긴한데... 일단은... wLen 사이즈만큼 까준거다... 이렇게 안하면...
				// wLen은 사실 부가적인 데이터기때문에 결국 Capacity를 초과해서 ASSERT 뜨거덩...
				WriteBytes((void*)&pSrcBuffer[0], (WORD)PaddingSize);

				if (pPaddingBuffer != NULL)
				{
					delete [] pPaddingBuffer;
					pPaddingBuffer = NULL;
				}
			}
			break;
		default:
			WriteBytes(pSrcBuffer, (WORD)col_desc->col_size);
		}
			
	}

	virtual void	Init(size_t BufSize) override
	{
		_ASSERT( BufSize <= 0xffffffff );
		if (m_pBuffer != NULL)
		{
			if (BufSize > m_Capacity)
			{
				delete [] m_pBuffer;
				m_pBuffer = NULL;
			}
		}
		
		if (m_pBuffer == NULL)
		{
			m_pBuffer  = new BYTE[BufSize];
			m_Capacity = (DWORD)BufSize;
		}

		Reset();
	}

	virtual void	Reset() { m_nROffset = m_nWOffset = m_nCurColumn = 0; }

	long	GetDataSize() { return m_nWOffset; }
	
	long	GetAvailableBufSize() { return (m_Capacity - m_nWOffset); }
	
	template <class T>
	CDBRecStream& operator >> (T& arg)
	{
		long nLen = sizeof(T);

		_ASSERT((m_nROffset + nLen) <= m_nWOffset);
		
		arg = *((T*)&m_pBuffer[m_nROffset]);
		
		m_nROffset += nLen;
		++m_nCurColumn;
		return *this;
	}

	template <>
	CDBRecStream& operator >> (std::wstring& strString)
	{
		WORD wStrLen = 0;
		WORD wColLen = 0;
		ReadBytes(&wStrLen, sizeof(WORD));
		ReadBytes(&wColLen, sizeof(WORD));

		WORD size_of_string_to_read = wStrLen*sizeof(WCHAR);
		if (wStrLen > 0)
		{
			_ASSERT((m_nROffset + size_of_string_to_read) <= m_nWOffset);

			strString.resize(wStrLen);
			ReadBytes( (void*)strString.c_str(), size_of_string_to_read );
		}
		
		//m_nROffset += (wColLen - (wStrLen + STR_HEADER_SIZE));
		m_nROffset += (wColLen - (size_of_string_to_read + STR_HEADER_SIZE));

		++m_nCurColumn;

		return *this;
	}

public:
	void _Skip()
	{
		TabDesc* desc = GetTabDesc();
		for each( ColDesc* col_desc in desc->listColumnDescs )
		{
			if (col_desc->ordinal == (m_nCurColumn + 1))
			{
				m_nROffset += col_desc->col_size;
				break;
			}
		}

		++m_nCurColumn;
	}

	void Rewind()
	{
		m_nROffset = m_nCurColumn = 0;
	}
};

//////////////////////////////////////////////////////////////////////////
// CRecAlloactor - derive this class for your own alloc & free
//////////////////////////////////////////////////////////////////////////
class _record_allocator
{
public:
	_record_allocator(CDBTable* pTab, TabDesc* pTabDesc = NULL, DWORD dwParam = 0) { m_pTab = pTab; }
	virtual ~_record_allocator() {}

protected:
	CDBTable* m_pTab;

public:
	virtual CDBRecord*	Allocate()
	{
		return (CDBRecord*)m_pTab->GetTabDesc()->pRuntime->CreateObject();
	}

	virtual void	Free(void* pRec)
	{
		m_pTab->GetTabDesc()->pRuntime->DeleteObject(pRec);
	}
	
	virtual void	GetUsageInfo(DWORD& dwAllocated, DWORD& InUse)
	{
		dwAllocated = 0;
		InUse = 0;
	}
};

// thread-safe 하게 하구싶으면
// template 인자로다가 'CChunkAllocatorMT<T_record_type>' 요거 넣어주시라!
template <class T_record_type, class T_pool_type = ChunkAllocatorST<T_record_type> >
class CCustomAllocator : public _record_allocator
{
	typedef T_pool_type		_RecordPool;

public:
	CCustomAllocator(CDBTable* pTab, TabDesc* pTabDesc = NULL, DWORD dwChunkSize = 1000) : _record_allocator(pTab), 
			m_Pool(dwChunkSize, NULL, 0, (pTabDesc != NULL) ? pTabDesc->strTabName.c_str() : _T("Unknown DB Table") ) {}

protected:
	_RecordPool	m_Pool;

public:
	virtual CDBRecord*	Allocate()	
	{ 
		return (CDBRecord*)m_Pool.NewItem(); 
	}
	virtual void Free(void* pRec)	{ m_Pool.FreeItem((T_record_type*)pRec); }

	virtual void GetUsageInfo(size_t& Allocated, size_t& InUse)
	{
		size_t free = 0;
		m_Pool.GetUsage(Allocated, InUse, free);
	}
};

