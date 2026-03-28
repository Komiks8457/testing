#include "stdafx.h"
#include "sql_param.h"
#include "adodb.h"

SQLParam::SQLParam() : buffer_(NULL), size_(0), direction_(ADODB::ParameterInput), type_(ADODB::DataBigInt), actual_size_(nullptr)
{
}

SQLParam::SQLParam(ADODB::ParameterDirection direction, ADODB::DataType type, void* buffer, std::size_t size, std::size_t* actual_size) :
	direction_(direction), type_(type), buffer_(buffer), size_(size), actual_size_(actual_size)
{
}

SQLParam::~SQLParam()
{
}

SQLVariableParam::SQLVariableParam(std::size_t size) : std::wostream(&buffer_), buffer_(size), base_param_(ADODB::ParameterInput, ADODB::DataVarWChar, buffer_.front_, size)
{
}

SQLVariableParam::~SQLVariableParam()
{
}

void SQLVariableParam::Reset()
{
	buffer_.Clear();
}

bool SQLVariableParam::IsEmpty() const
{
	return (buffer_.GetSize() == 0);
}