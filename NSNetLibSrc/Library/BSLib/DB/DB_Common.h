#pragma once

#include "../util/timespan.h"
#include "../thread/runtime.h"

#include "sql_query.h"
#include <OleAuto.h>

#define MAX_STRING (size_t)1024

// 일단 string은 지원 안한다.
struct BSVAR
{
	union
	{
		char		c;
		BYTE		bt;
		short		sh;
		WORD		w;
		long		l;
		DWORD		dw;
		LONGLONG	ll;
		float		f;
		double		d;
		PVOID		pv;
		char		st[MAX_STRING];

	};

	BOOL operator == (char arg)		{ return (c == arg); }
	BOOL operator == (BYTE arg)		{ return (bt == arg); }
	BOOL operator == (short arg)	{ return (sh == arg); }
	BOOL operator == (WORD arg)		{ return (w == arg); }
	BOOL operator == (long arg)		{ return (l == arg); }
	BOOL operator == (LONGLONG arg) { return (ll == arg); }
	BOOL operator == (DWORD arg)	{ return (dw == arg); }
	BOOL operator == (float arg)	{ return (f == arg); }
	BOOL operator == (double arg)	{ return (d == arg); }
};

///////////////////////////////////////////////////////////
// SQL native types override
///////////////////////////////////////////////////////////
typedef void*			_SQLHANDLE;

typedef _SQLHANDLE		_SQLHENV;
typedef _SQLHANDLE		_SQLHDBC;
typedef _SQLHANDLE		_SQLHSTMT;
typedef _SQLHANDLE		_SQLHDESC;



///////////////////////////////////////////////////////////

enum _RESERVED_DATA_GROUP_
{
	SYSDATA_GROUP_KEY,
	SYSDATA_GROUP_NEVERCHANGE,

	SYSDATA_GROUP_NUM,
	DATA_GROUP_USER = SYSDATA_GROUP_NUM
};		

#define MAX_RECORD_SIZE				8053
#define MAX_COLUMN_COUNT			1024
#define MAX_DATAGROUP_COUNT			64	// 이거 수정하게 되면 CRecord::m_DirtyGroup <-- 요거 데이터 타입도 함께 수정해야 한다.
#define MAX_DATAGROUP_ARRAY_SIZE	(MAX_COLUMN_COUNT / MAX_DATAGROUP_COUNT)

enum DATA_TYPE
{
	DATA_TYPE_UNKNOWN = -1,

	DATA_TYPE_1D = 0,
	DATA_TYPE_2D,
	DATA_TYPE_4D,
	DATA_TYPE_8D,
	DATA_TYPE_4F,
	DATA_TYPE_8F,
	DATA_TYPE_STRING,
	DATA_TYPE_WIDE_STRING,
	DATA_TYPE_TIMESTAMP
};

enum PARAM_INOUT_TYPE
{
	PARAM_TYPE_UNKNOWN	= 0,	// SQL_PARAM_TYPE_UNKNOWN
	PARAM_TYPE_IN		= 1,	// SQL_PARAM_INPUT
	PARAM_TYPE_OUT		= 4,	// SQL_PARAM_OUTPUT
	PARAM_TYPE_INOUT	= 2,	// SQL_PARAM_INPUT_OUTPUT
};

class CTypeDet
{
public:
	static LPCTSTR GetArgNotation(DATA_TYPE type) 
	{ 
		switch (type)
		{	
		case DATA_TYPE_1D: return _T("%u");
		case DATA_TYPE_2D: return _T("%d");
		case DATA_TYPE_4D: return _T("%d");
		case DATA_TYPE_8D: return _T("%I64d");
		case DATA_TYPE_4F: return _T("%f");
		case DATA_TYPE_STRING:
		case DATA_TYPE_WIDE_STRING:
			return _T("'%s'");
		case DATA_TYPE_TIMESTAMP: return _T("convert(datetime, '%d-%d-%d %d:%d:%d')");
		default:
			_ASSERT(FALSE);
			return NULL;
		}
	}

	template <class T> static DATA_TYPE GetDataType(T arg) 
	{
		switch (sizeof(arg))
		{
		case 1: return DATA_TYPE_1D;
		case 2: return DATA_TYPE_2D;
		case 4: return DATA_TYPE_4D;
		case 8: return DATA_TYPE_8D;
		default: _ASSERT(FALSE); return DATA_TYPE_UNKNOWN;
		}
	}
	template <> static DATA_TYPE GetDataType(float)			{ return DATA_TYPE_4F; }
	template <> static DATA_TYPE GetDataType(double)		{ _ASSERT(FALSE); return DATA_TYPE_8F; }	// 우리 인간적으로 double형은 쓰지 맙시다!
	template <> static DATA_TYPE GetDataType(std::wstring)	{ return DATA_TYPE_WIDE_STRING; }
	template <> static DATA_TYPE GetDataType(std::string)	{ return DATA_TYPE_STRING; }
	template <> static DATA_TYPE GetDataType(STIMESTAMP)	{ return DATA_TYPE_TIMESTAMP; }

	// 구조체나 클래스의 주소를 찾아내기 위해 사용하는 함수다.
	static int	GetFirstVarSize(DATA_TYPE type)
	{
		switch (type)
		{
		case DATA_TYPE_1D: return 1;
		case DATA_TYPE_2D: return 2;
		case DATA_TYPE_4D: return 4;
		case DATA_TYPE_8D: return 8;
		case DATA_TYPE_4F: return 4;
		case DATA_TYPE_STRING:
		case DATA_TYPE_WIDE_STRING:
#ifdef _WIN64
			return 8;
#else
			return 4;
#endif
		case DATA_TYPE_TIMESTAMP: return 4;
		default:
			_ASSERT(FALSE);
			return -1;
		}
	}

	static int	GetVarSize(DATA_TYPE type)
	{
		switch (type)
		{
		case DATA_TYPE_1D: return 1;
		case DATA_TYPE_2D: return 2;
		case DATA_TYPE_4D: return 4;
		case DATA_TYPE_8D: return 8;
		case DATA_TYPE_4F: return 4;
		case DATA_TYPE_WIDE_STRING: return sizeof(std::string);
		case DATA_TYPE_STRING: return sizeof(std::wstring);
		case DATA_TYPE_TIMESTAMP: return sizeof(STIMESTAMP);
		default:
			_ASSERT(FALSE);
			return -1;
		}
	}

	static short			GetSQL_C_Type(DATA_TYPE type);
	static ADODB::DataType	GetSQL_ADODB_Type(DATA_TYPE type);
	static DATA_TYPE		GetTypeFromSQLType(SHORT sql_type);
};

struct ColDesc
{
	ColDesc()
	{
		type	 = DATA_TYPE_UNKNOWN;
		indexed = false;
		preapred = true;
		offset	 = 0;
		col_size = 0;
		col_offset = 0;	// CDBRecord 오브젝트에서의 멤버 offset이 아니고 실제 DB상에서의 column offset이다!
		nullable = false;
		ordinal	 = -1;
		identity = false;
		column_idx = 0;
		matched_column_found = false;
	}

	std::tstring name;
	bool		indexed;
	bool		preapred;
	DATA_TYPE	type;
	int			offset;
	int			col_size;
	int			col_offset;
	bool		nullable;
	int			ordinal;
	bool		identity;
	std::tstring arg_notation;
	size_t		column_idx;
	bool		matched_column_found;
};

typedef std::vector<ColDesc*>	vecCOLDESC;
typedef std::list<ColDesc*>		LIST_COLUMN_DESCS;

#define TIMESTAMP_BUFFER_SIZE 20

class SQLPreparedParam : public SQLParam
{
public:
	SQLPreparedParam(ColDesc& col_desc) : SQLParam(ADODB::ParameterInput, CTypeDet::GetSQL_ADODB_Type(col_desc.type), NULL, col_desc.col_size), col_desc_(&col_desc)
	{
	}

	ColDesc* col_desc() const { return col_desc_; }

private:
	ColDesc* col_desc_;
};

class SQLPreparedQuery : public SQLDynamicQuery
{
public:
	~SQLPreparedQuery()
	{
		for each (SQLPreparedParam* param in params_) {
			delete param;
		}
		fields_.clear();
	}
	
	void BindParam(SQLPreparedParam* prepared_param)
	{
		SQLDynamicQuery::BindParam(prepared_param->col_desc()->name.c_str(), prepared_param);
	}

	const std::wstring& fields() const { return fields_; }
	void set_fields(std::wstring fields) { fields_ = fields; }

private:
	std::wstring fields_;
};

class SQLRuntimeParam : public SQLPreparedParam
{
public:
	SQLRuntimeParam(ColDesc& col_desc) : SQLPreparedParam(col_desc)
	{
	}

	const std::wstring& field() const { return field_; }
	void set_field(std::wstring fields) { field_ = fields; }

	const std::wstring& definition() const { return definition_; }
	void set_definition(std::wstring definition) { definition_ = definition; }

private:
	std::wstring field_;
	std::wstring definition_;
};

class SQLRuntimeQuery : public SQLDynamicQuery
{
public:
	~SQLRuntimeQuery()
	{
		for each (SQLRuntimeParam* param in params_) {
			delete param;
		}
	}

	void BindParam(SQLRuntimeParam* runtime_param)
	{
		SQLDynamicQuery::BindParam(runtime_param->col_desc()->name.c_str(), runtime_param);
		runtime_param->set_definition(params_def_);
		params_def_.clear();
	}
};

struct ParamGroup
{
	BYTE id;
	SQLPreparedQuery* preapred_update_query;
	SQLRuntimeQuery* runtime_update_query;
	vecCOLDESC ColDescs;

	explicit ParamGroup(BYTE id) : id(id), preapred_update_query(nullptr), runtime_update_query(nullptr)
	{
	}

	~ParamGroup()
	{
		delete preapred_update_query;
		delete runtime_update_query;
	}
};

typedef std::map<int, ParamGroup*>	mapPARAMGROUP;

class SQLBuildQuery : public SQLDynamicQuery
{
public:
	SQLBuildQuery() : param_offset_(0)
	{
		ZeroMemory(param_buffer_, MAX_RECORD_SIZE);
	}

	virtual ~SQLBuildQuery()
	{
		for each (SQLParam* param in params_)
			delete param;
	}

	void Reset()
	{
		for each (SQLParam* param in params_)
			delete param;

		params_.clear();
		param_offset_ = 0;
	}

	void BindParams(const SQLQuery::VECTOR_PARAMS& params, va_list& param_buffer)
	{
		std::size_t src_size = 0;
		std::size_t need_offset = 0;
		std::size_t dest_remain_size = 0;
		void* src_buffer = NULL;
		void* dest_buffer = NULL;
		for each (SQLPreparedParam* prepared_param in params) {
			ColDesc* col_desc = prepared_param->col_desc();
			need_offset = src_size = prepared_param->size();
			dest_remain_size = MAX_RECORD_SIZE - param_offset_;
			SQLParam* param = new SQLParam(*prepared_param);
			switch (col_desc->type) {
			case DATA_TYPE_8D:
				src_buffer = &va_arg(param_buffer, __int64);
				break;
			case DATA_TYPE_4D:
				src_buffer = &va_arg(param_buffer, int);
				break;
			case DATA_TYPE_2D:
				src_buffer = &va_arg(param_buffer, short);
				break;
			case DATA_TYPE_1D:
				src_buffer = &va_arg(param_buffer, BYTE);
				break;
			case DATA_TYPE_4F:
				src_buffer = &va_arg(param_buffer, float);
				break;
			case DATA_TYPE_STRING:
				{
					char* str = reinterpret_cast<char*>(reinterpret_cast<int*>(va_arg(param_buffer, int)));
					std::size_t len = 0;
					if (StringCchLengthA(str, dest_remain_size, &len) == S_OK) {
						src_buffer = str;
						src_size = len;
						if (src_size > 0)
							need_offset = src_size + sizeof(char);
						else
							need_offset = 0;
					}
				}
				break;
			case DATA_TYPE_WIDE_STRING:
				{
					wchar_t* str = reinterpret_cast<wchar_t*>(reinterpret_cast<int*>(va_arg(param_buffer, int)));
					std::size_t len = 0;
					if (SF_strlen(str, dest_remain_size, &len) == S_OK) {
						src_buffer = str;
						src_size = len * sizeof(wchar_t);
						if (src_size > 0)
							need_offset = src_size + sizeof(wchar_t);
						else
							need_offset = 0;
					}
				}
				break;
			default:
				ASSERT(FALSE);
			}
			if (need_offset > dest_remain_size) {
				ASSERT(FALSE);
				break;
			}
			else if (need_offset > 0)
			{
				dest_buffer = param_buffer_ + param_offset_;
				memcpy(dest_buffer, src_buffer, src_size);
				param->set_buffer(dest_buffer);
				param_offset_ += need_offset;
			}
			params_.push_back(param);
		}
	}

	void BindParams(const SQLQuery::VECTOR_PARAMS& params, BYTE* param_buffer)
	{
		for each (SQLPreparedParam* prepared_param in params) {
			if (BindParam(prepared_param, param_buffer) == false) {
				break;
			}
		}
	}

	void BindParams(ULONGLONG* dirty_group_array, const SQLQuery::VECTOR_PARAMS& params, BYTE* param_buffer)
	{
		SQLRuntimeParam* runtime_param = nullptr;
		for (unsigned char offset = 0; offset < MAX_DATAGROUP_ARRAY_SIZE; ++offset)
		{
			ULONGLONG dirty_group_array_mask = dirty_group_array[offset];
			for (unsigned short idx = offset * MAX_DATAGROUP_COUNT; dirty_group_array_mask != 0; ++idx, dirty_group_array_mask = dirty_group_array_mask >> 1)
			{
				if (dirty_group_array_mask & 1)
				{
					if (params_.empty() == false) {
						stmt_.append(L",");
						params_def_.append(L",");
					}
					runtime_param = static_cast<SQLRuntimeParam*>(params[idx]);
					if (BindParam(runtime_param, param_buffer) == false) {
						break;
					}
					stmt_.append(runtime_param->field());
					params_def_.append(runtime_param->definition());
				}
			}
		}
	}

	bool IsEmpty() const { return params_.empty(); }

protected:
	std::size_t param_offset_;
	char param_buffer_[MAX_RECORD_SIZE];

private:
	bool BindParam(SQLPreparedParam* prepared_param, BYTE* buffer)
	{
		long value = 0;
		void* src_buffer = NULL;
		ColDesc* col_desc = prepared_param->col_desc();
		std::size_t src_size = prepared_param->size();
		std::size_t need_offset = src_size;
		std::size_t dest_remain_size = MAX_RECORD_SIZE - param_offset_;
		SQLParam* param = new SQLParam(*prepared_param);
		switch (col_desc->type) {
		case DATA_TYPE_8D:
		case DATA_TYPE_4F:
		case DATA_TYPE_TIMESTAMP:
			src_buffer = buffer + col_desc->offset;
			break;
		case DATA_TYPE_STRING:
			{
				std::string* str = reinterpret_cast<std::string*>(buffer + col_desc->offset);
				src_buffer = const_cast<char*>(str->c_str());
				src_size = str->size();
				if (src_size > 0)
					need_offset = src_size + sizeof(char);
				else
					need_offset = 0;
			}
			break;
		case DATA_TYPE_WIDE_STRING:
			{
				std::wstring* str = reinterpret_cast<std::wstring*>(buffer + col_desc->offset);
				src_buffer = const_cast<wchar_t*>(str->c_str());
				src_size = str->size() * sizeof(wchar_t);
				if (src_size > 0)
					need_offset = src_size + sizeof(wchar_t);
				else
					need_offset = 0;
			}
			break;
		default:								
			{										
				value = *reinterpret_cast<long*>(buffer + col_desc->offset);
				if (col_desc->type == DATA_TYPE_2D) {
					value &= 0x0000ffff;		// 4바이트에 못미치는 녀석의 경우 매스킹해서 쓰레기값 날려버리자
					if (value & 0x8000) {	// short 형 처리... 이렇게 안하면 무조건 unsigned 형이 되어버린다
						value |= 0xffff0000;
					}
				} else if (col_desc->type == DATA_TYPE_1D) {
					value &= 0x000000ff;
				}
				src_buffer = &value;
			}
			break;
		}
		if (need_offset > dest_remain_size) {
			return false;
		}
		else if (need_offset > 0)
		{
			void* dest_buffer = param_buffer_ + param_offset_;
			memcpy(dest_buffer, src_buffer, src_size);
			param->set_buffer(dest_buffer);
			param_offset_ += need_offset;
		}
		params_.push_back(param);
		return true;
	}

};

class SQLInsertQuery : public SQLBuildQuery
{
public:
	SQLInsertQuery(void* identity_buffer, size_t identity_size)
		: identity_param_(nullptr)
	{
		if (identity_buffer == nullptr)
			return;

		ADODB::DataType data_type;

		switch (identity_size)
		{
		case 1:
			data_type = ADODB::DataTinyInt;
			break;
		case 2:
			data_type = ADODB::DataSmallInt;
			break;
		case 4:
			data_type = ADODB::DataInt;
			break;
		case 8:
			data_type = ADODB::DataBigInt;
			break;
		default:
			ASSERT(FALSE);
			return;
		}

		identity_param_ = new SQLParam(ADODB::ParameterOutput, data_type, identity_buffer, identity_size);
	}

	bool Execute(ADODB* adodb) override
	{
		if (identity_param_)
		{
			AddStmt(L";SELECT @identity=@@IDENTITY");
			SQLDynamicQuery::BindParam(L"identity", identity_param_);
		}

		if (SQLDynamicQuery::Execute(adodb) == false)
			return false;

		return true;
	}

	void BindParams(const mapPARAMGROUP& param_groups, BYTE* param_buffer)
	{
		long value = 0;
		std::size_t src_size = 0;
		std::size_t need_offset = 0;
		std::size_t dest_remain_size = 0;
		void* src_buffer = NULL;
		void* dest_buffer = NULL;
		for each (const mapPARAMGROUP::value_type& param_group in param_groups) {
			for each (const ColDesc* col_desc in param_group.second->ColDescs) {
				if (col_desc->identity == TRUE) {
					continue;
				}
				need_offset = src_size = col_desc->col_size;
				dest_remain_size = MAX_RECORD_SIZE - param_offset_;
				SQLParam* param = new SQLParam(ADODB::ParameterInput, CTypeDet::GetSQL_ADODB_Type(col_desc->type), nullptr, (col_desc->type == DATA_TYPE_WIDE_STRING) ? col_desc->col_size / sizeof(wchar_t) : col_desc->col_size);
				switch (col_desc->type) {
				case DATA_TYPE_8D:
				case DATA_TYPE_4F:
				case DATA_TYPE_TIMESTAMP:
					src_buffer = param_buffer + col_desc->offset;
					break;
				case DATA_TYPE_STRING:
					{
						std::string* str = reinterpret_cast<std::string*>(param_buffer + col_desc->offset);
						src_buffer = const_cast<char*>(str->c_str());

						src_size = str->size();
						if (src_size > 0)
							need_offset = src_size + sizeof(char);
						else
							need_offset = 0;
					}
					break;
				case DATA_TYPE_WIDE_STRING:
					{
						std::wstring* str = reinterpret_cast<std::wstring*>(param_buffer + col_desc->offset);
						src_buffer = const_cast<wchar_t*>(str->c_str());

						src_size = str->size() * sizeof(wchar_t);
						if (src_size > 0)
							need_offset = src_size + sizeof(wchar_t);
						else
							need_offset = 0;
					}
					break;
				default:
					{										
						value = *reinterpret_cast<long*>(param_buffer + col_desc->offset);
						if (col_desc->type == DATA_TYPE_2D) {
							value &= 0x0000ffff;		// 4바이트에 못미치는 녀석의 경우 매스킹해서 쓰레기값 날려버리자
							if (value & 0x8000) {	// short 형 처리... 이렇게 안하면 무조건 unsigned 형이 되어버린다
								value |= 0xffff0000;
							}
						} else if (col_desc->type == DATA_TYPE_1D) {
							value &= 0x000000ff;
						}
						src_buffer = &value;
					}
					break;
				}
				if (need_offset > dest_remain_size)
				{
					ASSERT(FALSE);
					break;
				}
				else if (need_offset > 0)
				{
					dest_buffer = param_buffer_ + param_offset_;
					memcpy(dest_buffer, src_buffer, src_size);
					param->set_buffer(dest_buffer);
					param_offset_ += need_offset;
				}
				params_.push_back(param);
			}
		}
	}

private:
	SQLParam* identity_param_;
};

typedef std::vector<SQLBuildQuery*> VECTOR_BUILD_UPDATE_QUERIES;

enum DB_OPERATION
{
	OPERATION_INVALID = -1,
	OPERATION_UPDATE = 0,
	OPERATION_INSERT,
	OPERATION_SELECT,
	OPERATION_SELECT_NOLOCK,
	OPERATION_DELETE,
	OPERATION_CHECKEXIST,

	OPERATION_NUM
};

#define	OPERATION_MASK_NONE			DWORD(0)
#define OPERATION_MASK_ALL			DWORD(0xffffffff)

#define	OPERATION_MASK_UPDATE		DWORD(1 << OPERATION_UPDATE)
#define	OPERATION_MASK_INSERT		DWORD(1 << OPERATION_INSERT)
#define	OPERATION_MASK_SELECT		DWORD(1 << OPERATION_SELECT)
#define	OPERATION_MASK_DELETE		DWORD(1 << OPERATION_DELETE)
#define	OPERATION_MASK_CHECKEXIST	DWORD(1 << OPERATION_CHECKEXIST)

class CDBTable;

struct TabDesc
{
	TabDesc() 
	{ 
		pRuntime = NULL; 
		RecordStrideToBind = 0;	// DB Table 전체 컬럼 사이즈가 아니라... 실제 CDBRecord의 데이터와 Bind되야할 컬럼 사이즈의 합을 말한다.
		dwAvailableOperations = OPERATION_MASK_NONE; 
		pIdentityCol		= NULL; 
		BoundTotalColNum	= 0;
		pOwnerTable			= NULL;
		dwSizeOfParentRecordObj = 0;
		dwRecordObjSize = 0;
	}

	~TabDesc()
	{
		for each (const mapPARAMGROUP::value_type& param_group in ParamGroups)
		{
			delete param_group.second;
		}
		ParamGroups.clear();

		for each( ColDesc* desc in listColumnDescs )
		{
			delete desc;
		}
		listColumnDescs.clear();
	}

	CRuntime*			pRuntime;
	std::tstring		strTabName;
	SQLPreparedQuery	prepared_query_[OPERATION_NUM];
	SQLPreparedQuery	where_query_;
	
	mapPARAMGROUP		ParamGroups;
	LIST_COLUMN_DESCS	listColumnDescs;

	size_t				RecordStrideToBind;
	DWORD				dwAvailableOperations;
	size_t				BoundTotalColNum;
	ColDesc*			pIdentityCol;
	CDBTable*			pOwnerTable;
	DWORD				dwSizeOfParentRecordObj;
	DWORD				dwRecordObjSize;

	ParamGroup* GetGroup(int group)
	{
		mapPARAMGROUP::iterator it_key = ParamGroups.find(group);
		if (it_key == ParamGroups.end())
			return NULL;
		
		return (*it_key).second;
	}
};

//////////////////////////////////////////////////////////////////////////
// Column Binder
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
#define	DECLARE_COL_DESC(class_name)																		\
protected:																									\
	static TabDesc s_##class_name##TabDesc;																	\
	friend class class_name##Param_Mapper;																	\
public:																										\
	static class_name instance_;																			\
	virtual LPCTSTR GetTableName()  { return class_name::s_##class_name##TabDesc.strTabName.c_str(); }		\
	virtual TabDesc* GetTabDesc()	{ return &class_name::s_##class_name##TabDesc; }						\
	static	size_t GetBoundRecordStride() { return class_name::s_##class_name##TabDesc.RecordStrideToBind; }

//////////////////////////////////////////////////////////////////////////
#define MAX_TEMP_BUFFER_SIZE	512

#define BEGIN_PARAM(class_name, table_name, operations)		\
	TabDesc		  class_name::s_##class_name##TabDesc;		\
	class_name	class_name::instance_;						\
	class class_name##Param_Mapper		\
	{									\
	public:								\
		class_name##Param_Mapper()		\
		{								\
			WCHAR szTempBuf[MAX_TEMP_BUFFER_SIZE] = {0,};	\
			class_name& instance = class_name::instance_;	\
			instance.Reset();								\
			std::tstring upper_table_name(L#table_name);	\
			std::transform(upper_table_name.begin(), upper_table_name.end(), upper_table_name.begin(), toupper);	\
			class_name::s_##class_name##TabDesc.strTabName = upper_table_name;				\
			ASSERT( (operations) != OPERATION_MASK_NONE);									\
			class_name::s_##class_name##TabDesc.dwAvailableOperations = (operations);		\
			class_name::s_##class_name##TabDesc.dwRecordObjSize = offsetof(class_name, m_EndOfData);	\
			mapPARAMGROUP& desc_group = class_name::s_##class_name##TabDesc.ParamGroups;	\
			LIST_COLUMN_DESCS& descs = class_name::s_##class_name##TabDesc.listColumnDescs;	\
			for (BYTE g = 0; g < SYSDATA_GROUP_NUM; ++g)		/* reserved group 들은 미리 추가시켜 둔다.*/\
				desc_group.insert(mapPARAMGROUP::value_type(g,new ParamGroup(g)));	\
			ParamGroup* pGroup = nullptr;	\
			ColDesc*	pColumn = nullptr;	\
			int column_idx = 0;

//////////////////////////////////////////////////////////////////////////
#define BIND_COL(group, var)	\
	pColumn = new ColDesc;		\
	pColumn->name = _T(#var);	\
	pColumn->offset = (int)((ULONG_PTR)&instance.var##_ - (ULONG_PTR)&instance);	\
	pColumn->col_size = sizeof( instance.var##_ );					\
	mapPARAMGROUP::iterator it##var##idx = desc_group.find(group);	\
	if (it##var##idx == desc_group.end())							\
		it##var##idx = desc_group.insert(desc_group.begin(), mapPARAMGROUP::value_type(group, new ParamGroup(group)));	\
	pGroup = (*it##var##idx).second;								\
	pColumn->column_idx = column_idx++;								\
	pGroup->ColDescs.push_back( pColumn );							\
	descs.push_back( pColumn );

#define BIND_COL_IDX(group, var)	\
	pColumn = new ColDesc;		\
	pColumn->indexed = true;	\
	pColumn->preapred = false;	\
	pColumn->name = _T(#var);	\
	pColumn->offset = (int)((ULONG_PTR)&instance.var##_ - (ULONG_PTR)&instance);	\
	pColumn->col_size = sizeof( instance.var##_ );					\
	mapPARAMGROUP::iterator it##var##idx = desc_group.find(group);	\
	if (it##var##idx == desc_group.end())							\
		it##var##idx = desc_group.insert(desc_group.begin(), mapPARAMGROUP::value_type(group, new ParamGroup(group)));	\
	pGroup = (*it##var##idx).second;								\
	instance.var##_idx_ = static_cast<int>(pGroup->ColDescs.size());			\
	pColumn->column_idx = column_idx++;								\
	pGroup->ColDescs.push_back( pColumn );							\
	descs.push_back( pColumn );

#define BIND_COL_ARRAY(group, var, size)	\
	for (int idx = 0; idx < size; ++idx)					\
	{										\
		::_itow_s(idx + 1, szTempBuf, _countof(szTempBuf), 10);		\
		pColumn = new ColDesc;				\
		pColumn->name = _T(#var);			\
		pColumn->name += &szTempBuf[0];		\
		pColumn->offset = (int)((ULONG_PTR)&instance.var##_[idx] - (ULONG_PTR)&instance); \
		pColumn->col_size = sizeof( instance.var##_[idx] );		\
		pColumn->column_idx = column_idx++;	\
		mapPARAMGROUP::iterator it##var##idx = desc_group.find(group);	\
		if (it##var##idx == desc_group.end())							\
			it##var##idx = desc_group.insert(desc_group.begin(), mapPARAMGROUP::value_type(group, new ParamGroup(group)));	\
		pGroup = (*it##var##idx).second;								\
		pGroup->ColDescs.push_back( pColumn );	\
		descs.push_back( pColumn );	\
	}

#define BIND_COL_ARRAY_NON_PREPARED(group, var, size)	\
	for (int idx = 0; idx < size; ++idx)	\
	{										\
		::_itow_s(idx + 1, szTempBuf, _countof(szTempBuf), 10);		\
		pColumn = new ColDesc;				\
		pColumn->preapred = false;			\
		pColumn->name = _T(#var);			\
		pColumn->name += &szTempBuf[0];		\
		pColumn->offset = (int)((ULONG_PTR)&instance.var##_[idx] - (ULONG_PTR)&instance); \
		pColumn->col_size = sizeof( instance.var##_[idx] );					\
		pColumn->column_idx = column_idx++;		\
		mapPARAMGROUP::iterator it##var##idx = desc_group.find(group);	\
		if (it##var##idx == desc_group.end())						\
			it##var##idx = desc_group.insert(desc_group.begin(), mapPARAMGROUP::value_type(group, new ParamGroup(group)));	\
		pGroup = (*it##var##idx).second;							\
		pGroup->ColDescs.push_back( pColumn );	\
		descs.push_back( pColumn );	\
	}

#define BIND_COL_STR(group, var, strLen)	\
	pColumn = new ColDesc;					\
	pColumn->name = _T(#var);				\
	pColumn->offset = (int)((ULONG_PTR)&instance.var##_ - (ULONG_PTR)&instance);	\
	pColumn->column_idx = column_idx++;		\
	pColumn->col_size = strLen;				\
	mapPARAMGROUP::iterator it##var##idx = desc_group.find(group);	\
	if (it##var##idx == desc_group.end())							\
		it##var##idx = desc_group.insert(desc_group.begin(), mapPARAMGROUP::value_type(group, new ParamGroup(group)));	\
	pGroup = (*it##var##idx).second;								\
	pGroup->ColDescs.push_back( pColumn );							\
	descs.push_back( pColumn );

//////////////////////////////////////////////////////////////////////////
#define END_PARAM(class_name, base_class)	\
			class_name::s_##class_name##TabDesc.pRuntime = BS_RUNTIME_CLASS(class_name);	\
			class_name::s_##class_name##TabDesc.dwSizeOfParentRecordObj = sizeof(base_class);	/* 상속과 무관하게 binding할 각 컬럼의 offset 이 0을 base로하도록! */\
			for( mapPARAMGROUP::iterator it = desc_group.begin() ; it != desc_group.end() ; ++it )			\
			{																								\
				ParamGroup* param_group = it->second;														\
				size_t column_count = param_group->ColDescs.size();											\
				for ( size_t i = 0 ; i < column_count ; ++i )												\
					param_group->ColDescs[i]->offset -= class_name::s_##class_name##TabDesc.dwSizeOfParentRecordObj;	\
			}																								\
		}																									\
	};																										\
class_name##Param_Mapper g_##class_name##Param_Mapper;

// 이 경우는 SELECT Only 이다... Reference Table 같은 경우에만 사용하자!
#define		BIND_ALL_FIELDS(class_name, base_class_name, tab_name)	\
	BEGIN_PARAM(class_name, tab_name, OPERATION_MASK_SELECT)		\
	END_PARAM(class_name, base_class_name)

//////////////////////////////////////////////////////////////////////////
// Variable Builder
//////////////////////////////////////////////////////////////////////////

///////////////////////////////////////// Stream Buffer를 사용할 경우
#define USE_STREAM_BUFFER()			\
	BEGIN_VAR_DECLARE()				\
	END_VAR_DECLARE()

///////////////////////////////////////// 바인딩 해서 사용할 멤버 변수를 지정할 경우
#define BEGIN_VAR_DECLARE			\
	public:

// Debugging때만 컬럼바인딩이 잘못 이루어지는 것을 알아내기 위해 검사용으로 변수를 하나 만들어 쓴다.
#define END_VAR_DECLARE			\
	private:					\
		BYTE	m_EndOfData;

#define DECLARE_VAR(class_name, group, type, var)		\
	private:											\
		type var##_;									\
	public:												\
		const type&	var() const { return var##_; }		\
		void	set_##var(type new_val)					\
		{												\
			if (new_val == var##_)						\
				return;									\
			if (s_##class_name##TabDesc.dwAvailableOperations & OPERATION_MASK_UPDATE)	\
			{											\
				var##_ = new_val;						\
				if (group != SYSDATA_GROUP_KEY && group != SYSDATA_GROUP_NEVERCHANGE)	\
				{										\
					SetGroupDirtyFlag(group);			\
				}										\
			}											\
			else										\
			{	ASSERT(FALSE); }						\
		}


#define DECLARE_VAR_IDX(class_name, group, type, var)	\
	private:											\
		type var##_;									\
		int var##_idx_;									\
	public:												\
		const type&	var() const { return var##_; }		\
		void	set_##var(type new_val)					\
		{												\
			if (new_val == var##_)						\
				return;									\
			if (s_##class_name##TabDesc.dwAvailableOperations & OPERATION_MASK_UPDATE)	\
			{											\
				var##_ = new_val;						\
				if (group != SYSDATA_GROUP_KEY && group != SYSDATA_GROUP_NEVERCHANGE)	\
				{										\
					SetGroupDirtyFlag(group, class_name::instance_.var##_idx_);			\
				}										\
			}											\
			else										\
			{	ASSERT(FALSE); }						\
		}

#define DECLARE_VAR_ARRAY(class_name, group, type, var, size)		\
	private:														\
		type var##_[size];											\
	public:															\
		const type (&var())[size] { return var##_; }				\
		const type&	var(int idx) { return var##_[idx]; }			\
		void	set_##var(int idx, type new_val)					\
		{															\
			_ASSERT(idx >= 0 && idx < (size));						\
			if (new_val == var##_[idx])								\
				return;												\
			if (s_##class_name##TabDesc.dwAvailableOperations & OPERATION_MASK_UPDATE)	\
			{														\
				var##_[idx] = new_val;								\
				if (group != SYSDATA_GROUP_KEY && group != SYSDATA_GROUP_NEVERCHANGE)	\
				{													\
					SetGroupDirtyFlag(group, idx);					\
				}													\
			}														\
			else													\
			{	_ASSERT(FALSE);	}									\
		}


//////////////////////////////////////////////////////////////////////////
// 가변인자
//////////////////////////////////////////////////////////////////////////
#define PUSH_DATA(pThis, cd, pop)						\
	{													\
		switch (cd.type)								\
		{												\
		case DATA_TYPE_8D:								\
			{											\
				unsigned long Data1 = *((ULONG_PTR*)(pThis + cd.offset));		\
				unsigned long Data2 = *((ULONG_PTR*)(pThis + cd.offset + 4));	\
				pop += 8;								\
				__asm push Data2						\
				__asm push Data1						\
			}											\
			break;										\
		case DATA_TYPE_STRING:							\
			{											\
				ULONG_PTR Data = (ULONG_PTR)((std::tstring*)(pThis + cd.offset))->c_str();			\
				pop += 8;								\
				__asm push Data							\
			}											\
			break;										\
		case DATA_TYPE_4F:								\
			{											\
				float fData = *((float*)(pThis + cd.offset));		\
				pop += 8;								\
				__asm fld	fData			/* 8바이트 실수 레지스터로 데이터(값) 로드 */	\
				__asm sub	esp, 8			/* 스택 포인터 이동 */	\
				__asm fstp  QWORD PTR [esp]	/* *esp = data */		\
			}											\
			break;										\
		case DATA_TYPE_TIMESTAMP:						\
			{											\
				STIMESTAMP* ts = ((STIMESTAMP*)(pThis + cd.offset));		\
				pop += (sizeof(DWORD) * 6);									\
				DWORD Datas1 = (*((ULONG_PTR*)(&ts->year))   & (DWORD)0x0000ffff);		\
				DWORD Datas2 = (*((ULONG_PTR*)(&ts->month))  & (DWORD)0x0000ffff);		\
				DWORD Datas3 = (*((ULONG_PTR*)(&ts->day))    & (DWORD)0x0000ffff);		\
				DWORD Datas4 = (*((ULONG_PTR*)(&ts->hour))   & (DWORD)0x0000ffff);		\
				DWORD Datas5 = (*((ULONG_PTR*)(&ts->minute)) & (DWORD)0x0000ffff);		\
				DWORD Datas6 = (*((ULONG_PTR*)(&ts->second)) & (DWORD)0x0000ffff);		\
				__asm push Datas6	\
				__asm push Datas5	\
				__asm push Datas4	\
				__asm push Datas3	\
				__asm push Datas2	\
				__asm push Datas1	\
			}											\
			break;										\
		default:										\
			{											\
				long nData = *((long*)(pThis + cd.offset));		\
				if (cd.type == DATA_TYPE_2D)			\
				{										\
					nData &= 0x0000ffff;	/* 4바이트에 못미치는 녀석의 경우 매스킹해서 쓰레기값 날려버리자 */	\
					if (nData & 0x8000)		/* short 형 처리... 이렇게 안하면 무조건 unsigned 형이 되어버린다*/ \
						nData |= 0xffff0000;			\
				}										\
				else if (cd.type == DATA_TYPE_1D)		\
					nData &= 0x000000ff;				\
				pop += 4;								\
				__asm push nData						\
			}											\
			break;										\
		}												\
	}

#define RESTORE_STACK(pop)		\
		__asm add esp, (pop)	\
		__asm mov pop, 0

#define REMOVE_LAST_COMMA(query)		\
	{									\
	int nQueryLen = ::lstrlen((LPCTSTR)&query[0]);	\
	query[nQueryLen - 1] = 0;		\
	}


extern inline void StringTrimRight(LPSTR pBuffer, int max_buffer_length)
{
	if (pBuffer == NULL)
	{
		_ASSERT(FALSE);
		return;
	}
	if (max_buffer_length == 0)
		return;

	LPSTR pNull = pBuffer + ( max_buffer_length - 1 );
	*pNull = '\0';
	
	if (max_buffer_length == 1)
		return;

	size_t str_length = ::lstrlenA( pBuffer );

	LPSTR pTemp = pBuffer + (str_length - 1);
	
	while (pTemp != pBuffer && *pTemp == _T(' '))
	{
		*pTemp = '\0';
		--pTemp;
	}
}

extern inline void WStringTrimRight(LPWSTR pBuffer, int max_buffer_length)
{
	if (pBuffer == NULL)
	{
		_ASSERT(FALSE);
		return;
	}
	if (max_buffer_length == 0)
		return;

	LPTSTR pNull = pBuffer + ( max_buffer_length - 1 );
	*pNull = '\0';

	if (max_buffer_length == 1)
		return;

	size_t str_length = ::lstrlen( pBuffer );

	LPTSTR pTemp = pBuffer + (str_length - 1);

	while (pTemp != pBuffer && *pTemp == _T(' '))
	{
		*pTemp = '\0';
		--pTemp;
	}
}

////////////////////////////////////////////////////////
// time 조작 관련 (VC <---> SQL Server)
#define DAY_IN_MIN						DWORD(60 * 24)
#define DAY_IN_SEC						DWORD(60 * 60 * 24)
#define ADDER_FOR_SQL_SMALLTIME_MIN		DWORD(25567 * 60 * 24)	// 25568: 1900/1/1 ~ 1970/1/1 사이에 몇일이나 있지? (윤달 포함이다)
#define ADDER_FOR_SQL_SMALLTIME_SEC		(DWORD)((DWORD)(25567) * (DWORD)(60) * (DWORD)(60) * (DWORD)(24))	// by minjin for removing warning message

extern DWORD	GetCurSQLSmallDateTime();
extern LONGLONG GetCurSQLDateTime();