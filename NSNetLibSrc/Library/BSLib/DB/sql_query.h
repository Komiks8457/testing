#ifndef BSLIB_SQL_QUERY_H_
#define BSLIB_SQL_QUERY_H_

#include "sql_param.h"

class ADODB;
class SQLQuery
{
public:
	enum Type
	{
		DYNAMIC,
		PROCEDURE,
	};

	virtual ~SQLQuery();
	virtual bool Execute(ADODB* adodb) = 0;
	virtual const char* GetTypeName() const { return typeid(*this).name(); }

	void BindParam(SQLParam* param);

	Type type() const { return type_; }
	typedef std::vector<SQLParam*> VECTOR_PARAMS;
	const VECTOR_PARAMS& params() const { return params_; }
	void set_timeout(unsigned long timeout) { timeout_ = timeout; }
	
protected:
	explicit SQLQuery(Type type);

	template<typename Record>
	bool SelectRecords(ADODB* adodb, std::list<Record>& records)
	{
		records.clear();
		Record record;
		void* buffer = &record;
		short column_count = 0;
		ADODB::DataType column_type;
		std::size_t column_size = 0;
		std::size_t buffer_size = 0;
		std::size_t offset = 0;
		while (!adodb->IsRowEnd()) {
			offset = 0;
			ZeroMemory(buffer, sizeof(Record));

			if (!adodb->GetColumnCount(column_count))
				return false;

			for (short column = 0; column < column_count; ++column)
			{
				if (!adodb->GetColumnType(column, column_type))
					return false;
				else if (!adodb->GetColumnDefinedSize(column, column_size))
					return false;

				switch (column_type)
				{
				case ADODB::DataChar:
				case ADODB::DataVarChar:
					buffer_size = column_size + 1;
					break;
				case ADODB::DataWChar:
				case ADODB::DataVarWChar:
					buffer_size = (column_size + 1) * sizeof(wchar_t);
					break;
				default:
					buffer_size = column_size;
				}

				if (offset + buffer_size > sizeof(Record))
					return false;
				else if (!adodb->GetColumnData(column, column_type, static_cast<unsigned char*>(buffer) + offset, buffer_size))
					return false;

				offset += buffer_size;
			}

			if (adodb->MoveNextRow() == false)
				return false;

			records.push_back(record);
		}

		return true;
	}
	
	Type type_;	
	VECTOR_PARAMS params_;
	unsigned long timeout_;
};

class SQLDynamicQuery : public SQLQuery
{
public:
	SQLDynamicQuery();
	virtual ~SQLDynamicQuery();

	virtual bool Execute(ADODB* adodb) override;

	void BindParam(const wchar_t* name, SQLParam* param);

	void AddStmt(const wchar_t* stmt);
	void AddParamsDef(const wchar_t* params_def);

	const std::wstring& stmt() const { return stmt_; }
	void set_stmt(const wchar_t* stmt) { stmt_ = stmt; }

	const std::wstring& params_def() const { return params_def_; }
	void set_params_def(const wchar_t* params_def) { params_def_ = params_def; }	

protected:
	std::wstring stmt_;
	std::wstring params_def_;
};

class SQLProcedureQuery : public SQLQuery
{
public:
	explicit SQLProcedureQuery(const wchar_t* name);
	virtual ~SQLProcedureQuery();

	void BindVariableParam(SQLVariableParam* variable_param);

	virtual bool Execute(ADODB* adodb) override;

	const std::wstring& name() const { return name_; }

protected:
	std::wstring name_;
};

#define DECLARE_SQL_QUERY_RECORD_BEGIN(Name)							\
	__pragma(pack(push,1))												\
	struct Name															\
	{

#define DECLARE_SQL_QUERY_RECORD_VALUE(Name, Type)						\
		Type Name;

#define DECLARE_SQL_QUERY_RECORD_STRING_A(Name, Size)					\
		char Name[Size + 1];

#define DECLARE_SQL_QUERY_RECORD_STRING_W(Name, Size)					\
		wchar_t Name[Size + 1];

#ifdef UNICODE
#define DECLARE_SQL_QUERY_RECORD_STRING(Name, Size)	DECLARE_SQL_QUERY_RECORD_STRING_W(Name, Size)
#else
#define DECLARE_SQL_QUERY_RECORD_STRING(Name, Size)	DECLARE_SQL_QUERY_RECORD_STRING_A(Name, Size)
#endif

#define DECLARE_SQL_QUERY_RECORD_END(Name)								\
	};																	\
	__pragma(pack(pop))													\
	typedef std::list<Name> Name##s;

#define DECLARE_SQL_QUERY_BEGIN(Name, Query)									\
class Name : public SQL##Query													\
{																				\
	public:		
		
#define DECLARE_SQL_QUERY_SELECT_RECORD(Name, Query)							\
	public:																		\
		const Name##s& records() const { return records_; }						\
	private:																	\
		Name##s records_;														\
		virtual bool Execute(ADODB* adodb) override								\
		{																		\
			if (SQL##Query::Execute(adodb) == false)							\
				return false;													\
			if (SelectRecords(adodb, records_) == false)						\
				return false;													\
			return true;														\
		}

#define DECLARE_SQL_QUERY_PARAM(Name, Type)										\
	private:																	\
		SQLParam Name##_param_;													\
		Type Name##_;															\
	public:																		\
		const Type& Name() const { return Name##_; }							\
		void set_##Name(const Type& value) { Name##_ = value; }

#define DECLARE_SQL_QUERY_PARAM_BUFFER(Name, Size)								\
	private:																	\
		SQLParam Name##_param_;													\
		BYTE Name##_[Size];														\
		std::size_t Name##_size_;												\
	public:																		\
		const BYTE* Name() const { return Name##_; }							\
		std::size_t Name##_size() const { return Name##_size_; }				\
		bool set_##Name(const void* buffer, std::size_t size) {					\
			if (Size < size) {													\
				ASSERT(FALSE);													\
				return false;													\
			}																	\
			memcpy(Name##_, buffer, size);										\
			Name##_size_ = size;												\
			return true;														\
		}

#define DECLARE_SQL_QUERY_PARAM_ARRAY(Name, Type, Size)							\
	private:																	\
		SQLParam Name##_param_[Size];											\
		Type Name##_[Size];														\
	public:																		\
		const Type& Name(size_t idx) { return Name##_[idx]; }					\
		void set_##Name(size_t idx, const Type& value)							\
		{																		\
			ASSERT(idx >= 0 && idx < Size);										\
			Name##_[idx] = value;												\
		}

#define DECLARE_SQL_QUERY_SELECT_PARAM(Name, Type)								\
	private:																	\
		SQLParam Name##_param_;													\
		Type Name##_;															\
	public:																		\
		Type Name() const { return Name##_; }

#define DECLARE_SQL_QUERY_ROW_COUNT_PARAM										\
	DECLARE_SQL_QUERY_SELECT_PARAM(row_count, DWORD)

#define DECLARE_SQL_QUERY_PARAM_STRING_A(Name, Length)							\
	private:																	\
		SQLParam Name##_param_;													\
		char Name##_[Length + 1];												\
	public:																		\
		const char* Name() const { return Name##_; }							\
		bool set_##Name(const char* Name) { return SUCCEEDED(StringCchCopyA(Name##_, Length + 1, Name)); }

#define DECLARE_SQL_QUERY_PARAM_STRING_W(Name, Length)							\
	private:																	\
		SQLParam Name##_param_;													\
		wchar_t Name##_[Length + 1];											\
	public:																		\
		const wchar_t* Name() const { return Name##_; }							\
		bool set_##Name(const wchar_t* Name) { return SUCCEEDED(StringCchCopyW(Name##_, Length + 1, Name)); }

#define DECLARE_SQL_QUERY_PARAM_STRING_ARRAY_A(Name, Length, Count)				\
	private:																	\
		SQLParam Name##_param_[Count];											\
		char Name##_[Count][Length + 1];										\
	public:																		\
		const char* Name(size_t idx) const { return Name##_[idx]; }				\
		bool set_##Name(size_t idx, const char* Name)							\
		{ 																		\
			ASSERT(idx >= 0 && idx < Array_Count);								\
			return SUCCEEDED(StringCchCopyA(Name##_[idx], Length + 1, Name));	\
		}																		\

#define DECLARE_SQL_QUERY_PARAM_STRING_ARRAY_W(Name, Length, Count)				\
	private:																	\
		SQLParam Name##_param_[Count];											\
		wchar_t Name##_[Count][Length + 1];										\
	public:																		\
		const wchar_t* Name(size_t idx) const { return Name##_[idx]; }			\
		bool set_##Name(size_t idx, const wchar_t* Name)						\
		{ 																		\
			ASSERT(idx >= 0 && idx < Count);									\
			return SUCCEEDED(StringCchCopyW(Name##_[idx], Length + 1, Name));	\
		}																		\

#ifdef UNICODE
#define DECLARE_SQL_QUERY_PARAM_STRING_ARRAY(Name, Length, Array_Count)	DECLARE_SQL_QUERY_PARAM_STRING_ARRAY_W(Name, Length, Array_Count)
#else
#define DECLARE_SQL_QUERY_PARAM_STRING_ARRAY(Name, Length, Array_Count)	DECLARE_SQL_QUERY_PARAM_STRING_ARRAY_A(Name, Length, Array_Count)
#endif

#ifdef UNICODE
#define DECLARE_SQL_QUERY_PARAM_STRING(Name, Size) DECLARE_SQL_QUERY_PARAM_STRING_W(Name, Size)
#else // UNICODE
#define DECLARE_SQL_QUERY_PARAM_STRING(Name, Size) DECLARE_SQL_QUERY_PARAM_STRING_A(Name, Size)
#endif // UNICODE

#define DECLARE_SQL_QUERY_VARIABLE_PARAM(Name)										\
	private:																		\
		SQLVariableParam Name##_param_;												\
		std::wostream& Name##_;														\
	public:																			\
		SQLVariableParam& Name() { return Name##_param_;	}

#define DECLARE_SQL_QUERY_CONSTRUCT(Name)											\
	public:																			\
		Name()

#define DECLARE_SQL_QUERY_CONSTRUCT_INIT_PROC(Name)									\
			: SQLProcedureQuery(L#Name)

#define DECLARE_SQL_QUERY_CONSTRUCT_INIT_PARAM_BEGIN(Name, Direction, Type)			\
			: Name##_param_(Direction, Type, &Name##_,								\
			(Type == ADODB::DataWChar) || (Type == ADODB::DataVarWChar)				\
			? sizeof(Name##_) / sizeof(wchar_t) : sizeof(Name##_))

#define DECLARE_SQL_QUERY_CONSTRUCT_INIT_PARAM(Name, Direction, Type)				\
			,Name##_param_(Direction, Type, &Name##_,								\
			(Type == ADODB::DataWChar) || (Type == ADODB::DataVarWChar)				\
			? sizeof(Name##_) / sizeof(wchar_t) : sizeof(Name##_))

#define DECLARE_SQL_QUERY_CONSTRUCT_INIT_PARAM_BUFFER_BEGIN(Name, Direction, Type)	\
			: Name##_param_(Direction, Type, &Name##_, sizeof(Name##_), &Name##_size_)

#define DECLARE_SQL_QUERY_CONSTRUCT_INIT_PARAM_BUFFER(Name, Direction, Type)		\
			,Name##_param_(Direction, Type, &Name##_, sizeof(Name##_), &Name##_size_)

#define DECLARE_SQL_QUERY_CONSTRUCT_INIT_SELECT_PARAM(Name, Type)					\
			,Name##_param_(ADODB::ParameterOutput, Type, &Name##_, sizeof(Name##_))

#define DECLARE_SQL_QUERY_CONSTRUCT_INIT_ROW_COUNT_PARAM							\
			DECLARE_SQL_QUERY_CONSTRUCT_INIT_SELECT_PARAM(row_count, ADODB::DataInt)

#define DECLARE_SQL_QUERY_CONSTRUCT_INIT_VARIABLE_PARAM_BEGIN(Name, Size)			\
			: Name##_param_(Size), Name##_(Name##_param_)

#define DECLARE_SQL_QUERY_CONSTRUCT_INIT_VARIABLE_PARAM(Name, Size)					\
			,Name##_param_(Size), Name##_(Name##_param_)

#define DECLARE_SQL_QUERY_CONSTRUCT_INIT_END										\
		{
		
#define DECLARE_SQL_QUERY_CONSTRUCT_SET_STMT(Stmt)									\
			std::wstring stmt = L#Stmt;												\
			ASSERT(stmt.front() == '"');											\
			stmt_.assign(stmt.begin() + 1, stmt.end() - 1);

#define DECLARE_SQL_QUERY_CONSTRUCT_BIND_PARAM(Name)								\
			SQLQuery::BindParam(&##Name##_param_);									\
			ZeroMemory(&##Name##_, sizeof(Name##_));

#define DECLARE_SQL_QUERY_CONSTRUCT_BIND_PARAM_ARRAY(Name, Direction, Type)			\
			ZeroMemory(&##Name##_, sizeof(Name##_));								\
			for (size_t idx = 0; idx < _countof(Name##_); ++idx)					\
			{																		\
				Name##_param_[idx].set_direction(Direction);						\
				Name##_param_[idx].set_type(Type);									\
				Name##_param_[idx].set_buffer(&##Name##_[idx]);						\
				Name##_param_[idx].set_size(sizeof(Name##_[idx]));					\
				SQLQuery::BindParam(&##Name##_param_[idx]);							\
			}

#define DECLARE_SQL_QUERY_CONSTRUCT_BIND_PARAM_NAME(Name)							\
			SQLDynamicQuery::BindParam(L#Name, &##Name##_param_);					\
			ZeroMemory(&##Name##_, sizeof(Name##_));

#define DECLARE_SQL_QUERY_CONSTRUCT_BIND_PARAM_BUFFER_NAME(Name)					\
			SQLDynamicQuery::BindParam(L#Name, &##Name##_param_);					\
			ZeroMemory(&##Name##_, sizeof(Name##_));								\
			ZeroMemory(&##Name##_size_, sizeof(Name##_size_));

#define DECLARE_SQL_QUERY_CONSTRUCT_BIND_PARAM_NAME_ARRAY(Name, Direction, Type)				\
			ZeroMemory(&##Name##_, sizeof(Name##_));											\
			for (size_t idx = 0; idx < _countof(Name##_); ++idx)								\
			{																					\
				Name##_param_[idx].set_direction(Direction);									\
				Name##_param_[idx].set_type(Type);												\
				Name##_param_[idx].set_buffer(&##Name##_[idx]);									\
				Name##_param_[idx].set_size(sizeof(Name##_[idx]));								\
				std::wstringstream param_name;													\
				param_name << L#Name << (idx + 1);												\
				SQLDynamicQuery::BindParam(param_name.str().c_str(), &##Name##_param_[idx]);	\
			}

#define DECLARE_SQL_QUERY_CONSTRUCT_BIND_VARIABLE_PARAM(Name)						\
			SQLProcedureQuery::BindVariableParam(&##Name##_param_);

#define DECLARE_SQL_QUERY_CONSTRUCT_BIND_SELECT_PARAM(Stmt, Name)					\
			SQLDynamicQuery::AddStmt(L#Stmt);										\
			DECLARE_SQL_QUERY_CONSTRUCT_BIND_PARAM_NAME(Name)

#define DECLARE_SQL_QUERY_CONSTRUCT_BIND_ROW_COUNT_PARAM							\
			DECLARE_SQL_QUERY_CONSTRUCT_BIND_SELECT_PARAM(;SELECT @ROW_COUNT=@@ROWCOUNT, row_count)

#define DECLARE_SQL_QUERY_END														\
		}																			\
	};

#endif // BSLIB_SQL_QUERY_H_
