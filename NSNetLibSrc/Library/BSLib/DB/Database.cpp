#include "stdafx.h"
#include "Database.h"
#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>
#include <malloc.h>
#include <time.h>
#include <comdef.h>

IMPLEMENT_OBJECT_POOL(Database::Session, ChunkAllocatorMT)

Database::Database() 
	: session_count_(0), session_number_(0), is_database_process_log_(false), cs_(TRUE), query_profile_tick_(0)
{
	error_reporter_ = [&](const ADODB& owner, HRESULT result, const wchar_t* desc, const wchar_t* func, unsigned long line, const wchar_t* sql)
	{
#if SERVER_BUILD
		JSONValue log;
		log["LOG_EVENT"] = "DB_ERROR";
		if (session_count_)
			log["SESSION_COUNT"] = session_count_;

		log["SESSION_NUMBER"] = static_cast<const Session&>(owner).number();

		if (owner.reconnect_count())
			log["SESSION_RECONNECT_COUNT"] = owner.reconnect_count();

		if (owner.retry_count())
			log["SESSION_RETRY_COUNT"] = owner.retry_count();

		log["LAST_SESSION_NUMBER"] = session_number_;

		std::stringstream ss;
		ss << result;
		log["RESULT"] = ss.str();

		if (desc)
			log["DESC"] = desc;

		if (func)
			log["FUNC"] = func;

		log["LINE"] = line;

		if (sql)
			log["SQL"] = sql;

		PutLog(LOG_FATAL_FILE, log);
#endif
	};
}

Database::~Database()
{
	Finalize();
}

void Database::Finalize()
{
	Session* session = nullptr;
	while (session_count_ > 0)
	{
		cs_.Lock();
		if (!session_pool_.empty())
		{
			session = session_pool_.top();
			session_pool_.pop();			
			FreeSession(*session, true);
		}
		cs_.Unlock();
		Sleep(1);
	}
}

bool Database::Connect(const wchar_t* connection_string, DWORD timeout)
{
	if (session_.Connect(connection_string, timeout) == false)
		return false;

	connection_string_ = connection_string;
	return true;
}

bool Database::ExecuteQuery(SQLQuery& query)
{
	ULONGLONG start_tick_count = GetTickCount64();
	bool result = false;

	DWORD reconnect_count = 0;
	Session* session = AllocSession(reconnect_count);

	if (session)
		result = query.Execute(session);

	ULONGLONG delta_tick = GetTickCount64() - start_tick_count;
	if (is_database_process_log_ && delta_tick >= query_profile_tick_)
	{
#if SERVER_BUILD
		JSONValue log;
		log["LOG_EVENT"] = "DB_QUERY_PROFILE";
		log["TYPE_NAME"] = query.GetTypeName();		
		log["RESULT"] = result;
		log["DELTA_TICK"] = delta_tick;

		if (session_count_)
			log["SESSION_COUNT"] = session_count_;	

		if (session)
		{
			log["SESSION_NUMBER"] = session->number();
			
			if (session->reconnect_count())
				log["SESSION_RECONNECT_COUNT"] = session->reconnect_count();

			if (session->retry_count())
				log["SESSION_RETRY_COUNT"] = session->retry_count();
		}
		else
		{
			if (reconnect_count)
				log["DB_RECONNECT_COUNT"] = reconnect_count;
		}

		log["LAST_SESSION_NUMBER"] = session_number_;
		
		switch (query.type())
		{
		case SQLQuery::DYNAMIC:
			log["STMT"] = static_cast<SQLDynamicQuery&>(query).stmt().c_str();
			break;
		case SQLQuery::PROCEDURE:
			log["PROCEDURE"] = static_cast<SQLProcedureQuery&>(query).name().c_str();
			break;
		}

		PutLog(LOG_NOTIFY_FILE, log);
#endif
	}
	
	if (session)
		FreeSession(*session);

	return result;
}

Database::Session* Database::AllocSession(DWORD& reconnect_count)
{
	Session* session = nullptr;

	while (true)
	{
		cs_.Lock();
		if (session_pool_.empty())
		{
			InterlockedIncrement(&session_count_);
			session = new Session(++session_number_, error_reporter_);
		}
		else
		{
			session = session_pool_.top();
			session_pool_.pop();
		}
		cs_.Unlock();

		if (session->Connect(connection_string_.c_str()))
			break;

		FreeSession(*session, true);

		if (ADODB::failover_count() == 0 || reconnect_count < ADODB::failover_count())
		{
			++reconnect_count;
			Sleep(1000);
			continue;
		}

		return nullptr;
	}
	
	return session;
}

void Database::FreeSession(Session& session, bool destroy)
{	
	if (!destroy && session.Disconnect())
	{
		cs_.Lock();
		session_pool_.push(&session);
		cs_.Unlock();
	}
	else
	{
		delete &session;
		InterlockedDecrement(&session_count_);
	}	
}

////////////////////////////////////////////////////////////////////////////////////////////////
// CDBSessionMgr
////////////////////////////////////////////////////////////////////////////////////////////////
#define _FETCH_BUF_SIZE_		(size_t)8192
//#define MAX_STRING				(size_t)1024 // DB에서 스트링을 받아올 수 있는 최대 사이즈

////////////////////////////////////////////////////////////////////////////////////////////////
// CDBTable
////////////////////////////////////////////////////////////////////////////////////////////////
CDBTable::CDBTable(DWORD id) : id_(id)
{
	database_ = nullptr;
    m_pTabDesc  = NULL;
    m_pAllocator= NULL;
    LPF_LOADER	= NULL;
}

CDBTable::~CDBTable()
{
    Reset();
}

void CDBTable::Reset()
{
	database_ = nullptr;

    if (m_pTabDesc != NULL)
        m_pTabDesc->pOwnerTable = NULL;

    m_pTabDesc  = NULL;

    SAFE_DELETE(m_pAllocator);
}

bool CDBTable::Create(Database* database, TabDesc* pTabDesc, _record_allocator* pAllocator)
{
	database_ = database;

	_ASSERT( m_pAllocator == nullptr );

    m_pTabDesc		= pTabDesc;

    m_pTabDesc->pOwnerTable = this;
    
    // allocator 를 넘겨주지 않았다면... default allocator 만들어서 붙여주자.
	m_pAllocator = (pAllocator == nullptr) ? new _record_allocator(this) : pAllocator;

	return _RetrieveTableAnatomy();
}

bool CDBTable::_BulkLoad_AllColumns(SQLQuery& query, listRECORDS* pResultSet)
{
    _ASSERT(pResultSet != NULL);

    size_t DBRecordStride  = (m_pTabDesc->RecordStrideToBind + (sizeof(SQLINTEGER) * m_pTabDesc->BoundTotalColNum));
    size_t RecordCount		= _FETCH_BUF_SIZE_ / DBRecordStride;

    if (RecordCount == 0) {
        _ASSERT(FALSE);
        return false;
    }

	DWORD reconnect_count = 0;
	Database::Session* session = database_->AllocSession(reconnect_count);
	if (session == nullptr)
		return false;

	bool result = false;

	if (!query.Execute(session))
		goto FREE_SESSION;
	
	CDBRecord* pNewRecord = nullptr;
	short column_idx = 0;
	byte* column_buffer = nullptr;
	size_t buffer_size = 0;
	
	while (!session->IsRowEnd())
	{
        pNewRecord = AllocNewRecord();

        for each( ColDesc* col_desc in m_pTabDesc->listColumnDescs ) 
		{
			switch (col_desc->type)
			{
			case DATA_TYPE_STRING:
				buffer_size = col_desc->col_size + sizeof(char);
				break;
			case DATA_TYPE_WIDE_STRING:
				buffer_size = col_desc->col_size + sizeof(wchar_t);
				break;
			default:
				buffer_size = col_desc->col_size;
			}

			column_buffer = static_cast<byte*>(alloca(buffer_size));
			ZeroMemory(column_buffer, buffer_size);

			column_idx = static_cast<short>(col_desc->ordinal - 1);

			if (!session->GetColumnData(column_idx, CTypeDet::GetSQL_ADODB_Type(col_desc->type), column_buffer, buffer_size)) {
				goto _FAILED;
			}

            pNewRecord->SetColumnData(col_desc, column_buffer);
        }

		if (!session->MoveNextRow()) {
			goto _FAILED;
		}

        pResultSet->push_back(pNewRecord);
		continue;

	_FAILED:
		_ASSERT(FALSE);
		FreeRecord(pNewRecord);
		goto FREE_SESSION;
    }

    result = true;

FREE_SESSION:
	database_->FreeSession(*session);
	return result;
}

bool CDBTable::_BulkLoad_PartialColumns(SQLQuery& query, listRECORDS* pResultSet)
{
    _ASSERT(pResultSet != NULL);

    size_t DBRecordStride  = (m_pTabDesc->RecordStrideToBind + (sizeof(SQLINTEGER) * m_pTabDesc->BoundTotalColNum));
    size_t RecordCount		= _FETCH_BUF_SIZE_ / DBRecordStride;

    if (RecordCount == 0) {
        _ASSERT(FALSE);
        return false;
    }

	DWORD reconnect_count = 0;
	Database::Session* session = database_->AllocSession(reconnect_count);
	if (session == nullptr)
		return false;

	bool result = false;

	if (!query.Execute(session))
		goto FREE_SESSION;

	CDBRecord* pNewRecord = nullptr;
	byte* column_buffer = nullptr;

    while (!session->IsRowEnd())
	{
		pNewRecord = AllocNewRecord();

        for each( ColDesc* col_desc in m_pTabDesc->listColumnDescs )
		{
			column_buffer = static_cast<byte*>(alloca(col_desc->col_size));
			ZeroMemory(column_buffer, col_desc->col_size);

			if( session->GetColumnData(col_desc->name.c_str(), CTypeDet::GetSQL_ADODB_Type(col_desc->type), column_buffer, col_desc->col_size) == false ){
				goto _FAILED;
			}

            pNewRecord->SetColumnData(col_desc, column_buffer);
        }

		if (!session->MoveNextRow()) {
			goto _FAILED;
		}
    
        pResultSet->push_back(pNewRecord);
		continue;

	_FAILED:
		_ASSERT(FALSE);
		FreeRecord(pNewRecord);
		goto FREE_SESSION;
    }

	result = true;

FREE_SESSION:
	database_->FreeSession(*session);
	return result;
}

CDBRecord* CDBTable::AllocNewRecord() 
{ 
    CDBRecord* pRecord = m_pAllocator->Allocate();    
	_ASSERT( pRecord->GetOwnerTable() == nullptr );

	pRecord->Init(GetTabDesc()->RecordStrideToBind);
    pRecord->Reset();
    pRecord->SetOwnerTable(this);

    return pRecord;
}

void CDBTable::FreeRecord(CDBRecord* pRecord)
{
    if (pRecord == NULL || m_pAllocator == NULL)
        return;
    
    CDBTable* pCurOwnerTableOfRecord = pRecord->GetOwnerTable();
    if (pCurOwnerTableOfRecord == NULL)
    {
        // 이미 소멸된 Record 이거나 밖에서 임의로 만든(new 했거나, 지역변수) Record를 소멸시키려 하는구나!
        _ASSERT(FALSE);
        return;
    }
    else if (pCurOwnerTableOfRecord != this)
    {
        // 너 지금 엉뚱한 테이블에 Record 지우려고 하구 있다 -_-+
        _ASSERT(FALSE);

        __try
        {
            pRecord->GetOwnerTable()->FreeRecord(pRecord);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }

        return;
    }

    pRecord->SetOwnerTable(nullptr);

    m_pAllocator->Free(pRecord);
}

////////////////////////////////////////////////////////////////////////////
// SELECT
////////////////////////////////////////////////////////////////////////////

bool CDBTable::LoadAllRecords(listRECORDS* pResultSet)
{
    _ASSERT(m_pTabDesc != nullptr);
    return (this->*LPF_LOADER)(m_pTabDesc->prepared_query_[OPERATION_SELECT], pResultSet);
}

bool CDBTable::SelectCustomCondition(listRECORDS* pResultSet, SQLDynamicQuery& Condition, LOCK_LEVEL LockLevel)
{
    _ASSERT(m_pTabDesc != NULL);

    SQLDynamicQuery query = Condition;

    std::wstring strTemp;
    ::StrLwr(strTemp, Condition.stmt().c_str());
    if (::_tcsstr(strTemp.c_str(), _T("select ")) == NULL) {
		std::wstring select_custom_stmt;
        if (LockLevel == SELECT_LOCKLEVEL_READ_UNCOMMITTED) {
			select_custom_stmt = m_pTabDesc->prepared_query_[OPERATION_SELECT_NOLOCK].stmt();
		} else {
			select_custom_stmt = m_pTabDesc->prepared_query_[OPERATION_SELECT].stmt();
		}
	
		// truncate 'FROM ... WHERE ...' clause
		size_t Pos = select_custom_stmt.find(_T(" FROM"));
		_ASSERT(Pos != std::tstring::npos);
		select_custom_stmt = select_custom_stmt.substr(0, Pos);
		select_custom_stmt += _T(" ");
		select_custom_stmt += Condition.stmt();
		query.set_stmt(select_custom_stmt.c_str());
    }

    return (this->*LPF_LOADER)(query, pResultSet);
}

bool CDBTable::SelectRecordsWhere(listRECORDS* pResultSet, SQLDynamicQuery& Where, LOCK_LEVEL LockLevel)
{
    _ASSERT(m_pTabDesc != NULL);

    if (Where.stmt().empty())
    {
        ASSERTA0( FALSE, _T("db function SelectRecordsWhere() called without 'where' arguments!\n") );
        return false;
    }

#ifdef _DEBUG
    std::tstring strTemp;
    ::StrLwr(strTemp, Where.stmt().c_str());
    if (::_tcsstr(strTemp.c_str(), _T("where")) == NULL)
    {
        ASSERTA0(FALSE, _T("db function SelectRecordsWhere() must be called with 'where' argutments!\n"));
        return false;
    }
#endif

	SQLDynamicQuery query;
    if (LockLevel == SELECT_LOCKLEVEL_READ_UNCOMMITTED) {
        query.set_stmt(m_pTabDesc->prepared_query_[OPERATION_SELECT_NOLOCK].stmt().c_str());
	} else {
		query.set_stmt(m_pTabDesc->prepared_query_[OPERATION_SELECT].stmt().c_str());
	}

	query.AddStmt(L" ");
	query.AddStmt(Where.stmt().c_str());
	query.AddParamsDef(Where.params_def().c_str());
	for each (SQLParam* param in Where.params()) {
		static_cast<SQLQuery&>(query).BindParam(param);
	}

    return (this->*LPF_LOADER)(query, pResultSet);
}

bool CDBTable::SelectRecords(listRECORDS* pResultSet, LOCK_LEVEL LockLevel, int KeyCount, ...)
{
    _ASSERT(m_pTabDesc != NULL);

    if (_CheckKeyCount(KeyCount) == false)
        return false;
    
    DB_OPERATION operation = (LockLevel == SELECT_LOCKLEVEL_READ_UNCOMMITTED) ? OPERATION_SELECT_NOLOCK : OPERATION_SELECT;
	SQLDynamicQuery& prepared_query = m_pTabDesc->prepared_query_[operation];
	SQLBuildQuery select_query;
	select_query.set_stmt(prepared_query.stmt().c_str());
	if (!m_pTabDesc->where_query_.stmt().empty()) {
		va_list ap;
		va_start(ap, KeyCount);
		select_query.AddStmt(m_pTabDesc->where_query_.stmt().c_str());
		select_query.set_params_def(m_pTabDesc->where_query_.params_def().c_str());
		select_query.BindParams(m_pTabDesc->where_query_.params(), ap);	
		va_end(ap);
	}

    return (this->*LPF_LOADER)(select_query, pResultSet);
}

////////////////////////////////////////////////////////////////////////////
// DELETE
////////////////////////////////////////////////////////////////////////////

bool CDBTable::DeleteRecords(int KeyCount, ...)
{
    _ASSERT(m_pTabDesc != NULL);

    if (!(m_pTabDesc->dwAvailableOperations & OPERATION_MASK_DELETE))
    {
        ASSERTA1(FALSE, _T("db function DeleteRecords() called without delete operation config table[%s]\n"), m_pTabDesc->strTabName.c_str());
        return false;
    }

    if (m_pTabDesc->GetGroup(SYSDATA_GROUP_KEY) == nullptr || m_pTabDesc->where_query_.stmt().empty() == true)
    {
        _ASSERT( m_pTabDesc->GetGroup(SYSDATA_GROUP_KEY) != nullptr );
		_ASSERT( m_pTabDesc->where_query_.stmt().empty() != false );
        return false;
    }

    if (_CheckKeyCount(KeyCount) == false)
        return false;

	SQLDynamicQuery& prepared_query = m_pTabDesc->prepared_query_[OPERATION_DELETE];
	SQLBuildQuery delete_query;
	delete_query.set_stmt(prepared_query.stmt().c_str());
	if (!m_pTabDesc->where_query_.stmt().empty()) {
		va_list ap;
		va_start(ap, KeyCount);
		delete_query.AddStmt(m_pTabDesc->where_query_.stmt().c_str());
		delete_query.set_params_def(m_pTabDesc->where_query_.params_def().c_str());
		delete_query.BindParams(m_pTabDesc->where_query_.params(), ap);	
		va_end(ap);
	}

	if (database_->ExecuteQuery(delete_query) == false)
		return false;

	return true;
}

////////////////////////////////////////////////////////////////////////////
// UPDATE
////////////////////////////////////////////////////////////////////////////
bool CDBTable::UpdateRecord(CDBRecord* pRecord, int nGroup)
{
	SQLBuildQuery update_query;
	if (pRecord->BuildUpdateQuery(nGroup, update_query) == false)
		return false;

	if (database_->ExecuteQuery(update_query) == false)
		return false;

    return true;
}

bool CDBTable::InsertRecord(CDBRecord* pRecord, void* lpIdentity, int identity_size)
{
	SQLInsertQuery insert_query(lpIdentity, identity_size);

	if (!BuildInsertQuery(pRecord, insert_query))
		return false;

	if (database_->ExecuteQuery(insert_query) == false)
		return false;

	return true;
}

bool CDBTable::BuildInsertQuery(CDBRecord* pRecord, SQLInsertQuery& query)
{
	if (!(m_pTabDesc->dwAvailableOperations & OPERATION_MASK_INSERT))
		return false;

	SQLDynamicQuery& prepared_query = m_pTabDesc->prepared_query_[OPERATION_INSERT];

	query.set_stmt(prepared_query.stmt().c_str());
	query.set_params_def(prepared_query.params_def().c_str());
	query.BindParams(m_pTabDesc->ParamGroups, pRecord->GetRecordBuffer());
	pRecord->ResetAllDirtyFlag();
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CDBTable::_RetrieveTableAnatomy()
{
    const int STR_LEN = 128 + 1;

    SQLTCHAR	szColumnName[STR_LEN] = {0, }, szTypeName[STR_LEN] = {0, };
    SQLINTEGER	DataSize, ColumnSize, OrdinalPosition;
    SQLSMALLINT Nullable, DataType;

	int  nPrevOffset	= 0;
	int  nPrevVarSize	= 0;
	size_t TotalColNumOfDBTable = 0;
	size_t current_binding_column_idx = 0;

	mapPARAMGROUP::iterator it_group;
	mapPARAMGROUP::iterator it_group_end;
	std::tstring strUnboundColumns;
	const std::tstring strCommaBlank = _T(", ");

	DWORD reconnect_count = 0;
    Database::Session* session = database_->AllocSession(reconnect_count);
	if (session == nullptr)
		return false;

	bool result = false;

    if (!session->SetCommand(ADODB::CommandSP, L"sp_columns"))
        goto FREE_SESSION;
    
    if (!session->SetParameter(1, ADODB::ParameterInput, ADODB::DataVarWChar, const_cast<wchar_t*>(m_pTabDesc->strTabName.c_str()), lstrlen(m_pTabDesc->strTabName.c_str())))
        goto FREE_SESSION;

    if (!session->SetParameter(2, ADODB::ParameterInput, ADODB::DataVarWChar, L"%", lstrlen(L"%")))
        goto FREE_SESSION;

	if (!session->SetParameter(3, ADODB::ParameterInput, ADODB::DataVarWChar, NULL, 1))
        goto FREE_SESSION;

    if (!session->SetParameter(4, ADODB::ParameterInput, ADODB::DataVarWChar,	NULL, 1))
		goto FREE_SESSION;

    int odbc_ver = 3;
    if (!session->SetParameter(5, ADODB::ParameterInput, ADODB::DataInt, &odbc_ver, sizeof(odbc_ver)))
        goto FREE_SESSION;

    if (!session->ExecuteCommand())
        goto FREE_SESSION;

	mapPARAMGROUP& param_groups = m_pTabDesc->ParamGroups;

    m_pTabDesc->RecordStrideToBind = 0;
    
    DWORD dwCol_Offset = 0;

    while (!session->IsRowEnd())
	{
        ::ZeroMemory( szColumnName, sizeof(szColumnName) );
		::ZeroMemory( szTypeName, sizeof(szTypeName) );

        if (!session->GetColumnData(3, ADODB::DataVarWChar, szColumnName, sizeof(szColumnName)))
            goto FREE_SESSION;

        if (!session->GetColumnData(4, ADODB::DataSmallInt, &DataType, sizeof(DataType)))
            goto FREE_SESSION;

        if (!session->GetColumnData(5, ADODB::DataVarWChar, szTypeName, sizeof(szTypeName)))
            goto FREE_SESSION;

        if (!session->GetColumnData(6, ADODB::DataInt, &ColumnSize, sizeof(ColumnSize)))
            goto FREE_SESSION;

        if (!session->GetColumnData(7, ADODB::DataInt, &DataSize, sizeof(DataSize)))
            goto FREE_SESSION;

        if (!session->GetColumnData(10, ADODB::DataSmallInt, &Nullable, sizeof(Nullable)))
            goto FREE_SESSION;

        if (!session->GetColumnData(16, ADODB::DataInt, &OrdinalPosition, sizeof(OrdinalPosition)))
            goto FREE_SESSION;

        if (::lstrlen((LPCTSTR)&szColumnName[0]) == 0)
        {
			std::wstring error(L"db table not found");
			PutLog(LOG_FATAL_FILE, L"Table:%s Error:%s", m_pTabDesc->strTabName.c_str(), error.c_str());
#ifdef DEBUG
            MessageBox(NULL, error.c_str(), m_pTabDesc->strTabName.c_str(), MB_OK | MB_ICONERROR);
#endif
            goto FREE_SESSION;
        }

		++TotalColNumOfDBTable;

        bool bFound = false;
        for each( mapPARAMGROUP::value_type it in param_groups )
        {					
            ParamGroup* group = it.second;

			size_t desc_count = group->ColDescs.size();
            for ( size_t i = 0 ; i < desc_count ; ++i )
            {
                ColDesc* col_desc = group->ColDescs[ i ];
                if (::SF_stricmp(col_desc->name.c_str(), (LPCTSTR)szColumnName) != 0)
					continue;

				DATA_TYPE data_type = CTypeDet::GetTypeFromSQLType( DataType );

				col_desc->nullable = (Nullable) ? true : false;
				col_desc->ordinal  = OrdinalPosition;
				col_desc->identity = (::_tcsstr((LPCTSTR)szTypeName, _T("identity")) != NULL);
				
				//////////////////////////////////////////////////////////////////////////
				if( current_binding_column_idx != col_desc->column_idx )
				{
					std::wstring error(L"the column order is not match - column:");
					error.append(szColumnName);
					PutLog(LOG_FATAL_FILE, L"Table:%s Error:%s", m_pTabDesc->strTabName.c_str(), error.c_str());
#ifdef DEBUG				
					MessageBox(NULL, error.c_str(), m_pTabDesc->strTabName.c_str(), MB_OK | MB_ICONERROR);
#endif				
					goto FREE_SESSION;
				}
                if (0 != col_desc->col_size)
                {
					switch (data_type)
					{
					case DATA_TYPE_WIDE_STRING:
						col_desc->col_size *= sizeof(wchar_t);
					case DATA_TYPE_STRING:
						if (col_desc->col_size != DataSize)
						{
							std::wstring error(L"different db column string length - column:");
							error.append(szColumnName);
							PutLog(LOG_FATAL_FILE, L"Table:%s Error:%s", m_pTabDesc->strTabName.c_str(), error.c_str());
#ifdef DEBUG							
							MessageBox(NULL, error.c_str(), m_pTabDesc->strTabName.c_str(), MB_OK | MB_ICONERROR);
#endif						
							goto FREE_SESSION;
						}
					}
                }
                //////////////////////////////////////////////////////////////////////////

                col_desc->col_size = DataSize;
                col_desc->col_offset = dwCol_Offset;
                col_desc->type = data_type;
                col_desc->arg_notation = CTypeDet::GetArgNotation(col_desc->type);

				// diagnostics
				int first_var_size = CTypeDet::GetFirstVarSize(col_desc->type);

				int expected_offset	= ((nPrevOffset + nPrevVarSize + (first_var_size - 1)) / first_var_size * first_var_size);

				if (expected_offset != col_desc->offset)
				{
					std::wstring error(L"table column binding failed - column:");
					error.append(szColumnName);
					PutLog(LOG_FATAL_FILE, L"Table:%s Error:%s", m_pTabDesc->strTabName.c_str(), error.c_str());
#ifdef DEBUG
					MessageBox(NULL, error.c_str(), m_pTabDesc->strTabName.c_str(), MB_OK | MB_ICONERROR);
#endif
					goto FREE_SESSION;
				}

				nPrevVarSize = CTypeDet::GetVarSize(col_desc->type);

				if (col_desc->indexed == true)
				{
					nPrevVarSize += sizeof(int);
					nPrevVarSize = (nPrevVarSize + sizeof(int) - 1) & ~(sizeof(int) - 1);
				}

				nPrevOffset = col_desc->offset;

                if (col_desc->identity == true)
                    m_pTabDesc->pIdentityCol = col_desc;

				col_desc->matched_column_found = true;

                m_pTabDesc->RecordStrideToBind += DataSize;
                bFound = true;
                break;
            }

			if (bFound == true)
				break;
        }

		if(bFound)
			++current_binding_column_idx;
		else
		{
			std::wstring error(L"binding column not found - column:");
			error.append(szColumnName);
			PutLog(LOG_FATAL_FILE, L"Table:%s Error:%s", m_pTabDesc->strTabName.c_str(), error.c_str());
#ifdef DEBUG
			MessageBox(NULL, error.c_str(), m_pTabDesc->strTabName.c_str(), MB_OK | MB_ICONERROR);
#endif
			goto FREE_SESSION;
		}
    
        dwCol_Offset += DataSize;

        if (!session->MoveNextRow()) { return false; }
    }

	bool bCheckObjectSize = false;
		
	it_group = param_groups.begin();
	it_group_end = param_groups.end();
	for (; it_group != it_group_end; ++it_group)
	{					
		ParamGroup* pGroup = (*it_group).second;
		vecCOLDESC::iterator it_col = pGroup->ColDescs.begin();
		vecCOLDESC::iterator it_col_end = pGroup->ColDescs.end();
		for (; it_col != it_col_end; ++it_col)
		{
			ColDesc* col_desc = (*it_col);
			if (col_desc->matched_column_found == false)
			{
				strUnboundColumns.append(col_desc->name);
				strUnboundColumns.append(strCommaBlank);
			}
		}
	}
	if (strUnboundColumns.empty() == false)
	{
		strUnboundColumns.resize(strUnboundColumns.length() - 2); // 맨 뒤에 붙은 쉼표 뺄라구...
		std::wstring error(L"db table has unbinding columns - ");
		error.append(strUnboundColumns);
		PutLog(LOG_FATAL_FILE, L"Table:%s Error:%s", m_pTabDesc->strTabName.c_str(), error.c_str());
#ifdef DEBUG
		MessageBox(NULL, error.c_str(), m_pTabDesc->strTabName.c_str(), MB_OK | MB_ICONERROR);
#endif
		result = false;
		goto FREE_SESSION;
	}

	bCheckObjectSize = (m_pTabDesc->pRuntime->IsDerivedFrom(BS_RUNTIME_CLASS(CDBRecStream)) == false);
	if (bCheckObjectSize == true)
	{
		// 원래는 아래의 <old code> 부분 처럼 8바이트 align 으로 클래스 사이즈를 계산했었다... 
		// 그런데, DB Table과 binding된 User Class 에서 부가적인 멤버변수를 선언해서 사용할 경우 TabDesc::dwRecordObjSize = sizeof(my_record_class) 해버리면
		// 그 user data들까지 sizeof() 에 포함되기 때문에 정확히 DB랑 엮여야 하는 데이터 사이즈의 계산이 불가능하거든? 
		// 그래서 어차피 그 검사 자체가 debugging때에만 필요한 거니까...
		// 약간 구리긴 해도 debug mode compile 시에만 class 의 binding 변수 맨 끝에 m_EndOfData 라는 BYTE 변수를 선언해서 정확히 binding할 사이즈를 계산하게 
		// 했다. 그래서 m_EndOfData를 구조체의 끝이라 생각하고 1 바이트 align으로 RecordObjSize를 계산하게 된 것이다.
		//
		// < old code >
		// DWORD dwRecordObjSize = m_pTabDesc->dwSizeOfParentRecordObj + (nPrevOffset + nPrevVarSize + 7) / 8 * 8; default로 구조체 사이즈 8 byte align 이잖냐... 바꾸면 좆되겠지만...
		DWORD dwRecordObjSize = m_pTabDesc->dwSizeOfParentRecordObj + (nPrevOffset + nPrevVarSize + 0) / 1 * 1;
		
		if (dwRecordObjSize != m_pTabDesc->dwRecordObjSize)
		{
			std::wstring error(L"table column binding error -");
			error.append(szColumnName);
			PutLog(LOG_FATAL_FILE, L"Table:%s Error:%s", m_pTabDesc->strTabName.c_str(), error.c_str());
#ifdef DEBUG
			MessageBox(NULL,  error.c_str(), m_pTabDesc->strTabName.c_str(), MB_OK | MB_ICONERROR);
#endif
			result = false;
			goto FREE_SESSION;
		}
	}

	_BuildQuery(*m_pTabDesc, TotalColNumOfDBTable);

	LPF_LOADER = (m_pTabDesc->BoundTotalColNum == TotalColNumOfDBTable) ? &CDBTable::_BulkLoad_AllColumns : &CDBTable::_BulkLoad_PartialColumns;    

	result = true;

FREE_SESSION:
	database_->FreeSession(*session);
	return result;
}

void CDBTable::_BuildQuery(TabDesc& tab_desc, size_t TotalColNumOfDBTable)
{
    Put(_T("\n===================================================="));
    Put(_T(" QUERY OF [%s]"), tab_desc.strTabName.c_str());
    Put(_T("===================================================="));

    TCHAR szQuery[QUERY_BUFFER_LEN];
    const std::tstring strWithNolock = _T(" WITH (NOLOCK)");
    const std::tstring strBlank = _T(" ");

	std::tstring strFirstKeyColumnName;
    
    /////////////////////////////////////////////////////////////
    // step1. where 절을 일단 만들고
    /////////////////////////////////////////////////////////////
    mapPARAMGROUP::iterator it_key = tab_desc.ParamGroups.find( SYSDATA_GROUP_KEY );
    if( it_key == tab_desc.ParamGroups.end() )
	{
		// Key가 지정이 안됬다면... 당근 update, delete 모두 불가능 하다!
		ASSERTA0((tab_desc.dwAvailableOperations & OPERATION_MASK_UPDATE) == 0 && (tab_desc.dwAvailableOperations & OPERATION_MASK_DELETE) == 0, 
			_T("키 그룹이 지정되지 않으면 Update나 Delete 모두 불가능하다구!\n"));
	}
	
	ParamGroup* group_key = it_key->second;
	if (group_key->ColDescs.empty() == false )
	{
		TCHAR szWhere[QUERY_BUFFER_LEN];
		::SF_sprintf( szWhere, _countof(szWhere), _T(" WHERE ") );
		
		auto column_iter = group_key->ColDescs.begin();
		for (ColDesc* desc = (*column_iter); ; desc = (*column_iter))
		{
			++column_iter;

			if (::SF_sprintf(szWhere, _countof(szWhere), _T("%s%s=@%s"), szWhere, desc->name.c_str(), desc->name.c_str()) != S_OK)
			{
				_ASSERT(FALSE);
			}
			tab_desc.where_query_.BindParam(new SQLPreparedParam(*desc));

			if (strFirstKeyColumnName.empty() == true)
				strFirstKeyColumnName = desc->name.c_str();

			if (column_iter == group_key->ColDescs.end())
			{
				break;
			}
			else if (::SF_sprintf(szWhere, _countof(szWhere), _T("%s AND "), szWhere) != S_OK)
			{
				_ASSERT(FALSE);
			}
		}

		tab_desc.where_query_.set_stmt(szWhere);
	}

    /////////////////////////////////////////////////////////////
    // step2. 각 그룹별 Update 쿼리를 만든다.
    /////////////////////////////////////////////////////////////
    if (tab_desc.dwAvailableOperations & OPERATION_MASK_UPDATE)
    {
		_ASSERT( tab_desc.where_query_.stmt().empty() == false );

		TCHAR szPreapredReplacementOnly[1024] = {0,};
        TCHAR szRuntimeReplacementOnly[1024] = {0,};
        for each( mapPARAMGROUP::value_type it in tab_desc.ParamGroups )
        {
            if( it.first == SYSDATA_GROUP_KEY || it.first == SYSDATA_GROUP_NEVERCHANGE )
                continue;
                
            if (::SF_sprintf(szQuery, _countof(szQuery), _T("UPDATE %s SET "), tab_desc.strTabName.c_str()) != S_OK)
            {
                _ASSERT(FALSE);
            }

			bool prepared_column = false;

			ParamGroup* param_group = it.second;
			for each( ColDesc* desc in param_group->ColDescs )
			{            
				if (desc->preapred == true)
				{
					prepared_column = true;

					if (::SF_sprintf(szQuery, _countof(szQuery), _T("%s%s=@%s"), szQuery, desc->name.c_str(), desc->name.c_str()) != S_OK) {
						_ASSERT(FALSE);
					}

					if (::SF_sprintf(szPreapredReplacementOnly, _countof(szPreapredReplacementOnly), _T("%s%s=@%s"), szPreapredReplacementOnly, desc->name.c_str(), desc->name.c_str()) != S_OK) {
						_ASSERT(FALSE);
					}

					if (param_group->preapred_update_query == nullptr)
					{
						param_group->preapred_update_query = new SQLPreparedQuery;
					}

					param_group->preapred_update_query->BindParam(new SQLPreparedParam(*desc));

					if (::SF_sprintf(szQuery, _countof(szQuery), _T("%s,"), szQuery) != S_OK)
					{
						_ASSERT(FALSE);
					}
					if (::SF_sprintf(szPreapredReplacementOnly, _countof(szPreapredReplacementOnly), _T("%s,"), szPreapredReplacementOnly) != S_OK)
					{
						_ASSERT(FALSE);
					}
				}
				else
				{
					if (::SF_sprintf(szRuntimeReplacementOnly, _countof(szRuntimeReplacementOnly), _T("%s=@%s"), desc->name.c_str(), desc->name.c_str()) != S_OK) {
						_ASSERT(FALSE);
					}

					SQLRuntimeParam* runtime_param = new SQLRuntimeParam(*desc);
					runtime_param->set_field(szRuntimeReplacementOnly);

					if (param_group->runtime_update_query == nullptr)
					{
						param_group->runtime_update_query = new SQLRuntimeQuery;
					}

					param_group->runtime_update_query->BindParam(runtime_param);
				}
			}

			if (prepared_column == true)
			{
				REMOVE_LAST_COMMA( szQuery );
			}
				
			if (param_group->preapred_update_query != nullptr)
			{
				param_group->preapred_update_query->set_stmt(szQuery);
				REMOVE_LAST_COMMA( szPreapredReplacementOnly );
				param_group->preapred_update_query->set_fields(szPreapredReplacementOnly);
			}
			else if (param_group->runtime_update_query != nullptr)  // prepared query가 있으면 쓰지않는다.
			{
				param_group->runtime_update_query->set_stmt(szQuery);
			}

            szQuery[0] = '\0';
            szPreapredReplacementOnly[0] = '\0';
			szRuntimeReplacementOnly[0] = '\0';
        }
    }

    /////////////////////////////////////////////////////////////
    // step3. 테이블 범용 쿼리 만든다.
    /////////////////////////////////////////////////////////////
    
    //****************
    // update (dirty value 존재 여부에 상관없이 레코드 몽창 update하는 쿼리)
    //****************
    if (tab_desc.dwAvailableOperations & OPERATION_MASK_UPDATE)
    {
        if (!tab_desc.where_query_.stmt().empty())
        {
            if (::SF_sprintf(szQuery, _countof(szQuery), _T("UPDATE %s SET "), tab_desc.strTabName.c_str()) != S_OK)
            {
                _ASSERT(FALSE);
            }

            for each( mapPARAMGROUP::value_type it in tab_desc.ParamGroups )
            {
                if (it.first == SYSDATA_GROUP_KEY || it.first == SYSDATA_GROUP_NEVERCHANGE)
                    continue;

                ParamGroup* param_group = it.second;

				for each ( ColDesc* desc in param_group->ColDescs )
				{
					if (::SF_sprintf(szQuery, _countof(szQuery), _T("%s%s=@%s,"), szQuery, desc->name.c_str(), desc->name.c_str()) != S_OK)
					{
						_ASSERT(FALSE);
					}
					
					tab_desc.prepared_query_[OPERATION_UPDATE].BindParam(new SQLPreparedParam(*desc));
				}
            }

            REMOVE_LAST_COMMA(szQuery);
			tab_desc.prepared_query_[OPERATION_UPDATE].set_stmt(szQuery);
        }
        else
        {
            _ASSERT(FALSE); // UPDATE 지정해 놓고... WHERE절이 없다는 게 말이돼? (이건 곧 Key Group을 지정하지 않았다는 거잖아? )
        }
    }
    
    //****************
    // insert
    //****************
    if (tab_desc.dwAvailableOperations & OPERATION_MASK_INSERT)
    {
        // 삽입할 parameter들 지정
        if (::SF_sprintf(szQuery, _countof(szQuery), _T("INSERT INTO %s("), tab_desc.strTabName.c_str()) != S_OK)
        {
            _ASSERT(FALSE);
        }

        for each( mapPARAMGROUP::value_type it in tab_desc.ParamGroups )
        {
            ParamGroup* param_group = it.second;
            for each( ColDesc* desc in param_group->ColDescs )
            {
                if (desc->identity == true)	// auto-incrementing id field 는 sql-server가 알아서 넣어준다.
                    continue;

                if (::SF_sprintf(szQuery, _countof(szQuery), _T("%s%s,"), szQuery, desc->name.c_str()) != S_OK)
                {
                    _ASSERT(FALSE);
                }
            }
        }
        REMOVE_LAST_COMMA(szQuery);
        
        // 실제 값들이 들어갈 format string
        if (::SF_sprintf(szQuery, _countof(szQuery), _T("%s) VALUES("), szQuery) != S_OK)
        {
            _ASSERT(FALSE);
        }

        for each( mapPARAMGROUP::value_type it in tab_desc.ParamGroups )
        {
            ParamGroup* param_group = it.second;
            for each( ColDesc* cd in param_group->ColDescs )
            {
                if (cd->identity == true)	// auto-incrementing id field 는 sql-server가 알아서 넣어준다.
                    continue;

                if (::SF_sprintf(szQuery, _countof(szQuery), _T("%s@%s,"), szQuery, cd->name.c_str()) != S_OK) {
                    _ASSERT(FALSE);
                }
                tab_desc.prepared_query_[OPERATION_INSERT].BindParam(new SQLPreparedParam(*cd));
            }
        }
        REMOVE_LAST_COMMA(szQuery);
        
        if (::SF_sprintf(szQuery, _countof(szQuery), _T("%s)"), szQuery) != S_OK)
        {
            _ASSERT(FALSE);
        }

        tab_desc.prepared_query_[OPERATION_INSERT].set_stmt(szQuery);
    }

    //****************
    // select
    //****************
    tab_desc.BoundTotalColNum = 0;

    for each( mapPARAMGROUP::value_type it in tab_desc.ParamGroups )
    {
        ParamGroup* group = it.second;
        tab_desc.BoundTotalColNum += group->ColDescs.size();
    }

    // 전체 컬럼이 Binding 됬구나? 
    // if (묵시적 바인딩 지정했다 || 명시적 바인딩이지만 전체 컬럼이 지정되었다)
    if (tab_desc.ParamGroups.size() <= 1 || tab_desc.BoundTotalColNum == TotalColNumOfDBTable)
    {
        if (::SF_sprintf(szQuery, _countof(szQuery), _T("SELECT * FROM %s"), tab_desc.strTabName.c_str()) != S_OK)
        {
            _ASSERT(FALSE);
        }

		tab_desc.prepared_query_[OPERATION_SELECT].set_stmt(szQuery);
		tab_desc.prepared_query_[OPERATION_SELECT_NOLOCK].set_stmt(szQuery);
		tab_desc.prepared_query_[OPERATION_SELECT_NOLOCK].AddStmt(strWithNolock.c_str());
    }
    else
    {
        if (::SF_sprintf(szQuery, _countof(szQuery), _T("SELECT ")) != S_OK)
        {
            _ASSERT(FALSE);
        }

        for each( mapPARAMGROUP::value_type it in tab_desc.ParamGroups )
        {
            ParamGroup* param_group = it.second;
            for each( ColDesc* cd in param_group->ColDescs )
            {
                if (::SF_sprintf(szQuery, _countof(szQuery), _T("%s%s,"), szQuery, cd->name.c_str()) != S_OK)
                {
                    _ASSERT(FALSE);
                }
            }
        }
        REMOVE_LAST_COMMA(szQuery);
        
        if (::SF_sprintf(szQuery, _countof(szQuery), _T("%sFROM %s"), szQuery, tab_desc.strTabName.c_str()) != S_OK)
        {
            _ASSERT(FALSE);
        }

		tab_desc.prepared_query_[OPERATION_SELECT].set_stmt(szQuery);
		tab_desc.prepared_query_[OPERATION_SELECT_NOLOCK].set_stmt(szQuery);
		tab_desc.prepared_query_[OPERATION_SELECT_NOLOCK].AddStmt(strWithNolock.c_str());
    }

    Put(_T("%s"), tab_desc.prepared_query_[OPERATION_SELECT].stmt().c_str());
    Put(_T("%s"), tab_desc.prepared_query_[OPERATION_SELECT_NOLOCK].stmt().c_str());

    //****************
    // delete
    //****************
    if (tab_desc.dwAvailableOperations & OPERATION_MASK_DELETE)
    {
        // where 절은 실제로 delete할때 동적으로 붙여서 사용한다. 미리 where절이 붙은 format string을 만들지 않는다.
        if (::SF_sprintf(szQuery, _countof(szQuery), _T("DELETE FROM %s"), tab_desc.strTabName.c_str()) != S_OK)
        {
            _ASSERT(FALSE);
        }
        tab_desc.prepared_query_[OPERATION_DELETE].set_stmt(szQuery);

        Put(_T("%s"), szQuery);
    }

    //****************
    // check if existing record
    //****************
    if (!tab_desc.where_query_.stmt().empty()) {
        _ASSERT(!strFirstKeyColumnName.empty());
        if (::SF_sprintf(szQuery, _countof(szQuery), _T("SELECT COUNT(%s) "), strFirstKeyColumnName.c_str()) != S_OK) {
            _ASSERT(FALSE);
        }
        if (::SF_sprintf(szQuery, _countof(szQuery), _T("%sFROM %s"), szQuery, tab_desc.strTabName.c_str()) != S_OK) {
            _ASSERT(FALSE);
        }
		tab_desc.prepared_query_[OPERATION_CHECKEXIST].set_stmt(szQuery);
		Put(_T("%s"), szQuery);
    }
    else
    {
    //	Put("WHERE 절이 없으므로 조건 만족하는 레코드 갯수 체크용 쿼리는 생성할 수 없다!");
    }
}

void CDBTable::GetUsageInfo(DWORD& dwAllocated, DWORD& dwInUse) 
{ 
    dwAllocated = 0;
    dwInUse = 0;
    
    if (m_pAllocator)
        m_pAllocator->GetUsageInfo(dwAllocated, dwInUse);
}

bool CDBTable::_CheckKeyCount(int KeyCount)
{
	ParamGroup* pKeyGroup = m_pTabDesc->GetGroup(SYSDATA_GROUP_KEY);
	if (pKeyGroup == NULL)
	{
		if (KeyCount == 0)
			return true;
		else
			return false;
	}

#ifdef _DEBUG
	if (pKeyGroup->ColDescs.size() != (size_t)KeyCount)
	{		
		ASSERTA2(FALSE, _T("Query 할때에는 Key Count를 정확히 지정해야 한다! [%s::Assigned_KeyCount: %d]"), m_pTabDesc->strTabName.c_str(), pKeyGroup->ColDescs.size());
	}
#endif

	return (pKeyGroup->ColDescs.size() == (size_t)KeyCount);
}

void CDBTable::FreeRecords(listRECORDS& records)
{
	for each( CDBRecord* record in records )
	{
		FreeRecord( record );
	}
	records.clear();
}

////////////////////////////////////////////////////////////////////////////////////////////////
// CDBRecord
////////////////////////////////////////////////////////////////////////////////////////////////
BS_IMPLEMENT_DYNAMIC(CDBRecord, CBase);

void CDBRecord::Release( CDBRecord* pRecord )
{
	if (pRecord == nullptr)
		return;

    CDBTable* pOwner = pRecord->GetOwnerTable();
    if( pOwner != NULL )
    {
        pOwner->FreeRecord( pRecord );
        pRecord->SetOwnerTable( NULL );
    }
}

void CDBRecord::Release( listRECORDS& records )
{
    for each( CDBRecord* pRecord in records )
    {
        CDBRecord::Release( pRecord );
    }

    records.clear();
}

#define PUSH_KEYGROUP_DATA()	\
    vecCOLDESC::reverse_iterator it_c;	\
    for (it_c = group_key->ColDescs.rbegin(); it_c != group_key->ColDescs.rend(); ++it_c)	\
    {							\
        ColDesc& cd = *it_c;	\
        PUSH_DATA(pThis, cd, nBytesToPop);	\
    }

bool CDBRecord::BuildUpdateQuery(int nGroup, SQLBuildQuery& query, bool bResetDirtyFlags)
{
    TabDesc* pTabDesc = GetTabDesc();

    if (!(pTabDesc->dwAvailableOperations & OPERATION_MASK_UPDATE))
    {		
        ASSERTA1(FALSE, _T("failed to build update query-invalid operation setting! [%s]\n"), pTabDesc->strTabName.c_str());
        return false;
    }
    
    if (nGroup == SYSDATA_GROUP_KEY)
    {
        ASSERTA1(FALSE, _T("can't build system key group update query![%s]\n"), pTabDesc->strTabName.c_str());
        return false;
    }

    //////////////////////////////////////// key group 얻는다.
    ParamGroup* group_key = pTabDesc->GetGroup(SYSDATA_GROUP_KEY);
    if (group_key == NULL)
    {
        _ASSERT(FALSE);
        return false;
    }

    // Group이 명시적으로 지정되었군...
    if (nGroup != UPDATE_ALL_MODIFIED_GROUP)
    {
        ASSERTA1((size_t)nGroup < pTabDesc->ParamGroups.size(), _T("invalid group for build update query![%s]\n"), pTabDesc->strTabName.c_str());

        // 얘들은 원래 바꿀 수 없다구...
        if (nGroup == SYSDATA_GROUP_KEY || nGroup == SYSDATA_GROUP_NEVERCHANGE)
            return false;

        if (IsDirtyGroup(nGroup) == false) // 한번도 수정되지 않은 데이터 그룹인데?
            return false;
    }

    //////////////////////////////////////// Query 만든다.
	BYTE* recored_buffer = GetRecordBuffer();

	// 지정된 그룹을 위한 쿼리를 생성한다
    if (nGroup != UPDATE_ALL_MODIFIED_GROUP)
	{
        mapPARAMGROUP::iterator it_group = pTabDesc->ParamGroups.find(nGroup);
        ParamGroup* group = (*it_group).second;

		SQLDynamicQuery* update_query = nullptr;
		if (group->preapred_update_query != nullptr)
		{
			update_query = group->preapred_update_query;

			query.set_stmt(update_query->stmt().c_str());
			query.set_params_def(update_query->params_def().c_str());
			query.BindParams(update_query->params(), recored_buffer);
			update_query = group->runtime_update_query;
			if (update_query != nullptr)
			{
				query.BindParams(m_DirtyGroupArray[group->id], update_query->params(), recored_buffer);
			}
		}
		else if (group->runtime_update_query != nullptr)
		{
			update_query = group->runtime_update_query;

			query.set_stmt(update_query->stmt().c_str());
			query.BindParams(m_DirtyGroupArray[group->id], update_query->params(), recored_buffer);
		}

		if (!pTabDesc->where_query_.stmt().empty())
		{
			query.AddStmt(pTabDesc->where_query_.stmt().c_str());
			query.AddParamsDef(pTabDesc->where_query_.params_def().c_str());
			query.BindParams(pTabDesc->where_query_.params(), recored_buffer);
		}
    }
	else if (HasDirtyDataGroup() == true)
	{
		query.set_stmt(L"UPDATE ");
		query.AddStmt(pTabDesc->strTabName.c_str());
		query.AddStmt(L" SET ");

		int group_id = -1;
		mapPARAMGROUP::iterator it_group_end = pTabDesc->ParamGroups.end();
		for (mapPARAMGROUP::iterator it_group = pTabDesc->ParamGroups.begin(); it_group != it_group_end; ++it_group)
		{
			group_id = (*it_group).first;

			if (group_id == SYSDATA_GROUP_KEY || group_id == SYSDATA_GROUP_NEVERCHANGE)
				continue;

			if (IsDirtyGroup(group_id) == false)
				continue;

			ParamGroup* group = (*it_group).second;

			if (group->preapred_update_query != nullptr)
			{
				if (query.params().empty() == false)
					query.AddStmt(L",");
				
				SQLPreparedQuery* update_query = group->preapred_update_query;
				query.AddStmt(update_query->fields().c_str());
				query.AddParamsDef(update_query->params_def().c_str());
				query.BindParams(update_query->params(), recored_buffer);
			}
			else if (group->runtime_update_query != nullptr)
				query.BindParams(m_DirtyGroupArray[group->id], group->runtime_update_query->params(), recored_buffer);
		}

		if (!pTabDesc->where_query_.stmt().empty())
		{
			query.AddStmt(pTabDesc->where_query_.stmt().c_str());
			query.AddParamsDef(pTabDesc->where_query_.params_def().c_str());
			query.BindParams(pTabDesc->where_query_.params(), recored_buffer);
		}
    }
    
    if (bResetDirtyFlags == true)
	{
        if (nGroup != UPDATE_ALL_MODIFIED_GROUP)
            ResetGroupDirtyFlag(nGroup);
		else
            ResetAllDirtyFlag();
    }

    return true;
}

CDBRecord* CDBRecord::Spawn()
{
    if (GetTabDesc()->pOwnerTable != nullptr)
    {
        CDBRecord* pSpawned = GetTabDesc()->pOwnerTable->AllocNewRecord();
        CopyData(pSpawned);

        return pSpawned;
    }
    return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Common Functions
////////////////////////////////////////////////////////////////////////////////////////////////

#define MAX_DATALEN			8192
#define SQLERR_FORMAT       _T("SQL Error State:%s, Native Error Code: %lX, ODBC Error: %s")

short CTypeDet::GetSQL_C_Type(DATA_TYPE type)
{
    switch (type)
    {
    case DATA_TYPE_1D: return SQL_C_UTINYINT;
    case DATA_TYPE_2D: return SQL_C_SSHORT;
    case DATA_TYPE_4D: return SQL_C_SLONG;
    case DATA_TYPE_8D: return SQL_C_SBIGINT;
    case DATA_TYPE_4F: return SQL_C_FLOAT;
    case DATA_TYPE_STRING: return SQL_C_CHAR;
	case DATA_TYPE_WIDE_STRING: return SQL_C_WCHAR;
    case DATA_TYPE_TIMESTAMP: return SQL_C_TYPE_TIMESTAMP;
    default:
        _ASSERT(FALSE);
        return -1;
    }
}

ADODB::DataType CTypeDet::GetSQL_ADODB_Type(DATA_TYPE type)
{
    switch (type)
    {
    case DATA_TYPE_1D: return ADODB::DataTinyInt;
    case DATA_TYPE_2D: return ADODB::DataSmallInt;
    case DATA_TYPE_4D: return ADODB::DataInt;
    case DATA_TYPE_8D: return ADODB::DataBigInt;
    case DATA_TYPE_4F: return ADODB::DataFloat;
    case DATA_TYPE_STRING: return ADODB::DataVarChar;
	case DATA_TYPE_WIDE_STRING: return ADODB::DataVarWChar;
    case DATA_TYPE_TIMESTAMP: return ADODB::DataDateTime;
    default:
        _ASSERT(FALSE);
        return ADODB::DataTinyInt;
    }
}

DATA_TYPE CTypeDet::GetTypeFromSQLType(SHORT sql_type)
{
    switch (sql_type)
    {
    case SQL_FLOAT:
        _ASSERT(FALSE);
        return DATA_TYPE_8F;
    case SQL_REAL:
        return DATA_TYPE_4F;
    case SQL_SMALLINT:
        return DATA_TYPE_2D;
    case SQL_INTEGER:
        return DATA_TYPE_4D;
    case SQL_BIGINT:
        return DATA_TYPE_8D;
    case SQL_TINYINT:
    case SQL_BIT:
        return DATA_TYPE_1D;
	case SQL_TYPE_TIMESTAMP:
    case SQL_TIMESTAMP:
        return DATA_TYPE_TIMESTAMP;
    case SQL_VARCHAR:
    case SQL_CHAR:
		return DATA_TYPE_STRING;
	case -8:
    case -9:
        return DATA_TYPE_WIDE_STRING;
    default:
        _ASSERT(FALSE);
        return DATA_TYPE_UNKNOWN;
    }
}

/////////////////////////////////
// for SQL 'smalldatetime' data type
// use 0x%x
/////////////////////////////////
DWORD GetCurSQLSmallDateTime()
{
    time_t cur_time = 0;
    ::time(&cur_time);
    
    long lSecDiff = 0;
    _get_timezone(&lSecDiff);				// UTC time --> local time

    cur_time -= lSecDiff;
    cur_time /= 60;							// 분단위로 변환
    cur_time += ADDER_FOR_SQL_SMALLTIME_MIN;// 1900/1/1 ~ 1970/1/1 사이에 해당하는 날짜를 분단위로 바꾸어 더하고...

    WORD wDate = WORD(cur_time / DAY_IN_MIN);		// 1900/1/1 부터 경과한 날짜수
    WORD wMinSinceMidNight = WORD(cur_time % DAY_IN_MIN);

    // 0x%x <-- 리턴되는 값을 요렇게 binary 형태로 해서 db에 update나 insert 해야만 정상적으로 들어간다아...
    // 그냥 %d 요렇게 정수로만 하면 상위 2바이트만 들어간다구...
    return MAKELONG(wMinSinceMidNight, wDate);
}

/////////////////////////////////
// for SQL 'datetime' data type
// use 0x%I64x
/////////////////////////////////
LONGLONG GetCurSQLDateTime()
{
    time_t cur_time = 0;
    ::time(&cur_time);

    //cur_time -= ::_timezone;				// UTC time --> local time

    long lSecDiff = 0;
    ::_get_timezone(&lSecDiff);				// UTC time --> local time

    cur_time -= lSecDiff;
    cur_time += ADDER_FOR_SQL_SMALLTIME_SEC;// 1900/1/1 ~ 1970/1/1 사이에 해당하는 날짜를 초단위로 바꾸어 더하고...
    
    DWORD dwDate = (DWORD)(cur_time / DAY_IN_SEC);	// 1900/1/1 부터 경과한 날짜수
    DWORD dwMilliSecSinceMidNight = (cur_time % DAY_IN_SEC) * 1000;

    dwMilliSecSinceMidNight = DWORD(dwMilliSecSinceMidNight / 3.333333f);	// millisec은 맞는데... resolution이 '3.33밀리초' 란다...
    //dwMilliSecSinceMidNight /= 3.333333f;	// millisec은 맞는데... resolution이 '3.33밀리초' 란다...
    
    // 0x%I64x <-- 리턴되는 값을 요렇게 binary 형태로 해서 db에 update나 insert 해야만 정상적으로 들어간다아...
    // 그냥 %I64d 요렇게 정수로만 하면 상위 4바이트만 들어간다구...
    return MAKE_LONGLONG(dwMilliSecSinceMidNight , dwDate);
}

BS_IMPLEMENT_DYNAMIC( CDBRecStream, CDBRecord );
