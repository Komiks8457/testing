#include "stdafx.h"
#include "sql_query.h"
#include <sstream>

#define SQL_QUERY_TIMEOUT 90

SQLQuery::SQLQuery(Type type) : type_(type), timeout_(SQL_QUERY_TIMEOUT) {}

SQLQuery::~SQLQuery()
{
	params_.clear();
}

void SQLQuery::BindParam(SQLParam* param)
{
	params_.push_back(param);
}

SQLDynamicQuery::SQLDynamicQuery() : SQLQuery(DYNAMIC)
{
}

SQLDynamicQuery::~SQLDynamicQuery()
{
	stmt_.clear();
	params_def_.clear();
}

bool SQLDynamicQuery::Execute(ADODB* adodb)
{
	if (!adodb->SetCommand(ADODB::CommandSP, L"sp_executesql"))
		return false;

	if (!adodb->SetCommandTimeout(timeout_))
		return false;

	short column = 0;
	if (!adodb->SetParameter(++column, ADODB::ParameterInput, ADODB::DataLongVarWChar, const_cast<wchar_t*>(stmt_.c_str()), stmt_.size() * sizeof(wchar_t)))
		return false;

	if (!params_.empty())
	{
		if (!adodb->SetParameter(++column, ADODB::ParameterInput, ADODB::DataLongVarWChar, const_cast<wchar_t*>(params_def_.c_str()), params_def_.size() * sizeof(wchar_t))) {
			return false;
		}

		for each (SQLParam* param in params_)
		{
			if (!adodb->SetParameter(++column, param->direction(), param->type(), (param->size() > 0) ? param->buffer() : nullptr, param->size(), param->actual_size()))
				return false;
		}
	}

	if (!adodb->ExecuteCommand()) {
		return false;
	}

	return true;
}

void SQLDynamicQuery::BindParam(const wchar_t* name, SQLParam* param)
{
	std::wstring definition;
	definition.append(L"@");
	definition.append(name);
	switch (param->type()) {
	case ADODB::DataTinyInt:
		definition.append(L" tinyint");
		break;
	case ADODB::DataSmallInt:
		definition.append(L" smallint");
		break;
	case ADODB::DataInt:
		definition.append(L" int");
		break;
	case ADODB::DataBigInt:
		definition.append(L" bigint");
		break;
	case ADODB::DataFloat:
		definition.append(L" float");
		break;
	case ADODB::DataDouble:
		definition.append(L" double");
		break;
	case ADODB::DataVarChar:
		{
			definition.append(L" varchar(");
			std::wostringstream size_stream;
			size_stream << param->size();
			definition.append(size_stream.str());
			definition.append(L")");
		}
		break;
	case ADODB::DataVarWChar:
		{
			definition.append(L" nvarchar(");
			std::wostringstream size_stream;
			size_stream << param->size();
			definition.append(size_stream.str());
			definition.append(L")");
		}
		break;
	case ADODB::DataDateTime:
		definition.append(L" datetime");
		break;
	case ADODB::DataSmallDateTime:
		definition.append(L" smalldatetime");
		break;
	case ADODB::DataBinary:
		{
			definition.append(L" binary(");
			std::wostringstream size_stream;
			size_stream << param->size();
			definition.append(size_stream.str());
			definition.append(L")");
		}
		break;
	case ADODB::DataVarBinary:
		{
			definition.append(L" varbinary(");
			std::wostringstream size_stream;
			if (param->size() <= 8000)
			{
				size_stream << param->size();
				definition.append(size_stream.str());
			}
			else
				definition.append(L"max");
			
			definition.append(L")");
		}
		break;
	default:
		ASSERT(FALSE);
	}

	if (param->direction() != ADODB::ParameterInput)
		definition.append(L" output");

	AddParamsDef(definition.c_str());
	params_.push_back(param);
}

void SQLDynamicQuery::AddStmt(const wchar_t* stmt)
{
	stmt_.append(stmt);
}

void SQLDynamicQuery::AddParamsDef(const wchar_t* params_def)
{
	if (!params_def_.empty()) {
		params_def_.append(L",");
	}
	params_def_.append(params_def);	
}

SQLProcedureQuery::SQLProcedureQuery(const wchar_t* name) : SQLQuery(PROCEDURE), name_(name)
{
}

SQLProcedureQuery::~SQLProcedureQuery()
{
	name_.clear();
}

void SQLProcedureQuery::BindVariableParam(SQLVariableParam* variable_param)
{
	params_.push_back(&variable_param->base_param_);
}

bool SQLProcedureQuery::Execute(ADODB* adodb)
{
	if (!adodb->SetCommand(ADODB::CommandSP, name_.c_str())) {
		return false;
	}

	if (!adodb->SetCommandTimeout(timeout_))
		return false;

	short column = 0;
	for each (SQLParam* param in params_) {
		if (!adodb->SetParameter(++column, param->direction(), param->type(),param->buffer(), param->size(), param->actual_size())) {
			return false;
		}
	}

	if (!adodb->ExecuteCommand()) {
		return false;
	}

	return true;
}
