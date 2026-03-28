#include "stdafx.h"
#include "adodb.h"
#include <oledberr.h>

#ifdef _WIN64
#import "C:\Program Files\Common Files\System\ado\msado15.dll" no_namespace raw_interfaces_only
#else
#import "libid:2A75196C-D9EB-4129-B803-931327F72D5C" no_namespace raw_interfaces_only
#endif // _WIN64

ADODB* ADODB::co_init_ = nullptr;
DWORD ADODB::failover_count_ = 0;

IMPLEMENT_OBJECT_POOL(ADODB::OutputParameter, ChunkAllocatorMT)

ADODB::ADODB() : connection_(nullptr), command_(nullptr), command_type_(adCmdUnspecified), recordset_(nullptr), parameters_(nullptr), fields_(nullptr), reconnect_count_(0), retry_count_(0), sign_value_(0)
{
}

ADODB::~ADODB()
{
	Disconnect();

	if (co_init_ == this)
	{
		CoUninitialize();
		co_init_ = nullptr;
	}
}

bool ADODB::Connect(const wchar_t* ip, unsigned short port, const wchar_t* db, const wchar_t* user, const wchar_t* password, unsigned long timeout)
{
	std::wostringstream connection_stream;
	connection_stream << "Provider=SQLOLEDB;Data Source=" << ip << "," << port << ";Initial Catalog=" << db << ";User Id=" << user << ";Password=" << password;
	std::wstring connection_string(connection_stream.str());
	return Connect(connection_string.c_str(), timeout);
}

bool ADODB::Connect(const wchar_t* connection_string, unsigned long timeout)
{
	HRESULT hr = S_FALSE;
	unsigned long error_line = 0;

	if (co_init_ == nullptr)
	{
		hr = CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_SPEED_OVER_MEMORY);
		if (FAILED(hr))
		{
			error_line = __LINE__;
			goto FAILED;
		}

		co_init_ = this;
	}

	if (!Disconnect())
	{
		error_line = __LINE__;
		goto FAILED;
	}

	hr = CoCreateInstance(__uuidof(Connection), NULL, CLSCTX_INPROC, __uuidof(_Connection), reinterpret_cast<void**>(&connection_));
	if (FAILED(hr))
	{
		error_line = __LINE__;
		goto FAILED;
	}

	if (timeout) {
		hr = connection_->put_ConnectionTimeout(timeout);
		if (FAILED(hr))
		{
			error_line = __LINE__;
			goto FAILED;
		}
	}

	hr = connection_->put_ConnectionString(_bstr_t(connection_string));
	if (FAILED(hr))
	{
		error_line = __LINE__;
		goto FAILED;
	}

	hr = connection_->Open(_bstr_t(""), _bstr_t(""), _bstr_t(""), adConnectUnspecified);
	if (FAILED(hr))
	{
		error_line = __LINE__;
		goto FAILED;
	}

	if (command_ == nullptr)
	{
		hr = CoCreateInstance(__uuidof(Command), NULL, CLSCTX_INPROC, __uuidof(_Command), reinterpret_cast<void**>(&command_));
		if (FAILED(hr))
		{
			error_line = __LINE__;
			goto FAILED;
		}
	}

	hr = command_->putref_ActiveConnection(connection_);
	if (FAILED(hr))
	{
		error_line = __LINE__;
		goto FAILED;
	}

	hr = command_->get_Parameters(&parameters_);
	if (FAILED(hr))
	{
		error_line = __LINE__;
		goto FAILED;
	}

	connection_string_ = connection_string;
	parameters_->Release();

	return true;

FAILED:
	ReportError(hr, __FUNCTIONW__, error_line);
	return false;
}

bool ADODB::Connect(unsigned long timeout)
{
	HRESULT hr = S_FALSE;
	unsigned long error_line = 0;

	if (connection_string_.empty())
	{
		error_line = __LINE__;
		goto FAILED;
	}

	return Connect(connection_string_.c_str(), timeout);

FAILED:
	ReportError(hr, __FUNCTIONW__, error_line);
	return false;
}

bool ADODB::Disconnect()
{
	HRESULT hr = S_FALSE;
	unsigned long error_line = 0;	

	if (connection_ != nullptr)
	{		
		ReleaseRecordset();

		if (command_ != nullptr)
		{
			ResetCommand();
			command_->Release();
			command_ = nullptr;
		}

		if (IsConnected() == true)
		{
			hr = connection_->Close();
			if (FAILED(hr))
			{
				error_line = __LINE__;
				goto FAILED;
			}
		}

		connection_->Release();
		connection_ = nullptr;
	}
	
	return true;

FAILED:
	ReportError(hr, __FUNCTIONW__, error_line);
	return false;
}

bool ADODB::SetCommand(CommandType type, const wchar_t* sql)
{
	HRESULT hr = S_FALSE;
	unsigned long error_line = 0;

	reconnect_count_ = 0;
	retry_count_ = 0;
	
	if (ResetCommand() == false)
	{
		error_line = __LINE__;
		goto FAILED;
	}

	hr = command_->put_CommandText(_bstr_t(sql));
	if (FAILED(hr))
	{
		error_line = __LINE__;
		goto FAILED;
	}
	
	switch (type)
	{
	case CommandText:
		command_type_ = adCmdText;
		break;
	case CommandSP:
		command_type_ = adCmdStoredProc;
		break;
	}

	return true;

FAILED:
	ReportError(hr, __FUNCTIONW__, error_line);
	return false;
}

bool ADODB::SetParameter(_variant_t column, ParameterDirection direction, DataType type, void* buffer, std::size_t size, std::size_t* actual_size)
{	
	HRESULT hr = S_FALSE;
	_variant_t variant;
	std::size_t buffer_size = size;
	std::size_t data_size = (actual_size) ? *actual_size : size;
	DataTypeEnum adtype = adEmpty;
	_Parameter* parameter = nullptr;
	unsigned long error_line = 0;

	if (command_ == NULL)
	{
		error_line = __LINE__;
		goto FAILED;
	}

	if (parameters_ == NULL)
	{
		error_line = __LINE__;
		goto FAILED;
	}

	if (direction != ParameterInput && buffer == nullptr)
	{
		error_line = __LINE__;
		goto FAILED;
	}

	switch (type) {
	case DataTinyInt:
		adtype = adTinyInt;
		if (buffer != NULL) {
			variant = *static_cast<BYTE*>(buffer);
		} else {
			variant.ChangeType(VT_NULL);
		}
		break;
	case DataSmallInt:
		adtype = adSmallInt;
		if (buffer != NULL) {
			variant = *static_cast<short*>(buffer);
		} else {
			variant.ChangeType(VT_NULL);
		}
		break;
	case DataInt:
		adtype = adInteger;
		if (buffer != NULL) {
			variant = *static_cast<int*>(buffer);
		} else {
			variant.ChangeType(VT_NULL);
		}
		break;
	case DataBigInt:
		adtype = adBigInt;
		if (buffer != NULL) {
			variant = *static_cast<__int64*>(buffer);
		} else {
			variant.ChangeType(VT_NULL);
		}
		break;
	case DataFloat:
		adtype = adSingle;
		if (buffer != NULL) {
			variant = *static_cast<float*>(buffer);
		} else {
			variant.ChangeType(VT_NULL);
		}
		break;
	case DataDouble:
		adtype = adDouble;
		if (buffer != NULL) {
			variant = *static_cast<double*>(buffer);
		} else {
			variant.ChangeType(VT_NULL);
		}
		break;
	case DataChar:
		adtype = adChar;
		variant = static_cast<const char*>(buffer);
		if (buffer != NULL) {
			variant = static_cast<const char*>(buffer);
		} else {
			variant.ChangeType(VT_NULL);
		}
		break;
	case DataVarChar:
		adtype = adVarChar;
		variant = static_cast<const char*>(buffer);
		if (buffer != NULL) {
			variant = static_cast<const char*>(buffer);
		} else {
			variant.ChangeType(VT_NULL);
		}
		break;
	case DataWChar:
		adtype = adWChar;
		variant = static_cast<const wchar_t*>(buffer);
		if (buffer != NULL) {
			variant = static_cast<const wchar_t*>(buffer);
			buffer_size *= sizeof(wchar_t);
		} else {
			variant.ChangeType(VT_NULL);
		}
		break;
	case DataVarWChar:
		adtype = adVarWChar;
		if (buffer != NULL) {
			variant = static_cast<const wchar_t*>(buffer);
			buffer_size *= sizeof(wchar_t);
		} else {
			variant.ChangeType(VT_NULL);
		}
		break;
	case DataLongVarWChar:
		adtype = adLongVarWChar;
		if (buffer != NULL) {
			variant = static_cast<const wchar_t*>(buffer);
			buffer_size *= sizeof(wchar_t);
		} else {
			variant.ChangeType(VT_NULL);
		}
		break;
	case DataSmallDateTime:
	case DataDateTime:
		adtype = adDBTimeStamp;
		if (buffer != NULL) {
			TIMESTAMP_STRUCT* ts = static_cast<TIMESTAMP_STRUCT*>(buffer);
			SYSTEMTIME system_time;
			system_time.wYear = ts->year;
			system_time.wMonth = ts->month;
			system_time.wDayOfWeek = 0;
			system_time.wDay = ts->day;
			system_time.wHour = ts->hour;
			system_time.wMinute = ts->minute;
			system_time.wSecond = ts->second;
			double time = 0;
			SystemTimeToVariantTime(&system_time, &time);
			variant = time;				
		} else {
			variant.ChangeType(VT_NULL);
			buffer_size = sizeof(DATE);
		}
		break;
	case DataTimeStamp:
		data_size = 8;
	case DataBinary:
		adtype = adBinary;
	case DataVarBinary:
		{
			if (adtype == adEmpty)
				adtype = adVarBinary;

			if (data_size)
			{
				SAFEARRAYBOUND bound;
				bound.lLbound = 0;

				if (direction == ParameterInput)
					bound.cElements = static_cast<ULONG>(data_size);
				else
					bound.cElements = static_cast<ULONG>(buffer_size);

				variant.parray = SafeArrayCreate(VT_UI1, 1, &bound);

				void* data = nullptr;
				hr = SafeArrayAccessData(variant.parray, &data);
				if (FAILED(hr))
				{
					error_line = __LINE__;
					goto FAILED;
				}

				memcpy(data, buffer, data_size);
				variant.vt = VT_ARRAY | VT_UI1;
			}
			else
			{
				variant.ChangeType(VT_NULL);
			}
		}
		break;
	default:
		error_line = __LINE__;
		goto FAILED;
	}

	column.ChangeType(VT_BSTR);

	hr = command_->CreateParameter(column.bstrVal, adtype, static_cast<ParameterDirectionEnum>(direction + 1),
		(direction == ParameterInput && variant.vt != VT_NULL) ? static_cast<long>(data_size) : static_cast<long>(buffer_size), variant, &parameter);

	if (FAILED(hr))
	{
		error_line = __LINE__;
		goto FAILED;
	}

	hr = parameters_->Append(parameter);
	if (FAILED(hr))
	{
		error_line = __LINE__;
		goto FAILED;
	}
	parameter->Release();

	if (direction != ParameterInput)
	{
		OutputParameter* output_parameter = new OutputParameter;
		output_parameter->column = column;
		output_parameter->type = type;
		output_parameter->buffer = buffer;
		output_parameter->buffer_size = buffer_size;
		output_parameter->actual_size = actual_size;
		output_parameters_.push_back(output_parameter);
	}

	return true;

FAILED:
	ReportError(hr, __FUNCTIONW__, error_line);
	return false;
}

bool ADODB::ExecuteCommand()
{
	HRESULT hr = S_FALSE;
	unsigned long error_line = 0;
	CursorLocationEnum current_cursor_location = adUseNone;
	CursorLocationEnum selected_cursor_location = adUseServer;
	
	retry_count_ = 0;

	if (ReleaseRecordset() == false)
	{
		error_line = __LINE__;
		goto FAILED;
	}

	hr = connection_->get_CursorLocation(&current_cursor_location);
	if (FAILED(hr))
	{
		error_line = __LINE__;
		goto FAILED;
	}

	if (output_parameters_.empty() == false)
		selected_cursor_location = adUseClient;

	if (current_cursor_location != selected_cursor_location)
	{
		hr = connection_->put_CursorLocation(selected_cursor_location);
		if (FAILED(hr))
		{
			error_line = __LINE__;
			goto FAILED;
		}
	}

	do
	{
		hr = CoCreateInstance(__uuidof(Recordset), NULL, CLSCTX_INPROC, __uuidof(_Recordset), reinterpret_cast<void**>(&recordset_));
		if (FAILED(hr))
		{
			error_line = __LINE__;
			goto FAILED;
		}

		hr = recordset_->putref_Source(command_);
		if (FAILED(hr))
		{
			error_line = __LINE__;
			goto FAILED;
		}

		hr = recordset_->Open(vtMissing, vtMissing, adOpenForwardOnly, adLockReadOnly, command_type_);
		if (FAILED(hr))
		{
			switch (hr)
			{
			case DB_E_ABORTLIMITREACHED:
			case E_FAIL:
				{
					ReportError(hr, __FUNCTIONW__, __LINE__);

					if (ReleaseRecordset() == false)
					{
						error_line = __LINE__;
						goto FAILED;
					}

					if (ResetConnection() == false)
					{
						error_line = __LINE__;
						goto FAILED;
					}

					if (failover_count_ == 0 || reconnect_count_ < failover_count_)
					{
						++reconnect_count_;
					}
					else
					{
						error_line = __LINE__;
						goto FAILED;
					}
				}
				break;
			default:
				error_line = __LINE__;
				goto FAILED;
			}
		}
	}
	while (FAILED(hr));

	hr = recordset_->get_Fields(&fields_);
	if (FAILED(hr))
	{
		error_line = __LINE__;
		goto FAILED;
	}

	fields_->Release();

	for each(OutputParameter* output_parameter in output_parameters_)
	{
		if (!GetParameterData(output_parameter->column, output_parameter->type, output_parameter->buffer, output_parameter->buffer_size, output_parameter->actual_size))
		{
			error_line = __LINE__;
			goto FAILED;
		}

		delete output_parameter;
	}
	output_parameters_.clear();

	return true;

FAILED:
	ReportError(hr, __FUNCTIONW__, error_line);
	return false;
}

bool ADODB::GetRowCount(unsigned long& count)
{
	HRESULT hr = S_FALSE;
	unsigned long error_line = 0;
	CursorLocationEnum current_cursor_location = adUseNone;

	if (recordset_ == NULL)
	{
		error_line = __LINE__;
		goto FAILED;
	}

	hr = connection_->get_CursorLocation(&current_cursor_location);
	if (FAILED(hr))
	{
		error_line = __LINE__;
		goto FAILED;
	}

	if (current_cursor_location == adUseClient && IsRowEnd() == false)
	{
		for (count = 0; SUCCEEDED(recordset_->MoveNext()); ++count);

		if (count > 0)
		{
			hr = recordset_->MoveFirst();
			if (FAILED(hr))
			{
				error_line = __LINE__;
				goto FAILED;
			}
		}
	}

	return true;

FAILED:
	ReportError(hr, __FUNCTIONW__, error_line);
	return false;
}

bool ADODB::GetRowSize(std::size_t& size)
{
	HRESULT hr = S_FALSE;
	unsigned long error_line = 0;
	short column_count = 0;
	long actual_size = 0;

	if (fields_ == NULL)
	{
		error_line = __LINE__;
		goto FAILED;
	}

	if (!GetColumnCount(column_count))
	{
		error_line = __LINE__;
		goto FAILED;
	}

	for (short count = 0; count < column_count; ++count)
	{
		Field* field = NULL;
		_variant_t column_variant(static_cast<short>(count));
		hr = fields_->get_Item(column_variant, &field);
		if (FAILED(hr))
		{
			error_line = __LINE__;
			goto FAILED;
		}

		field->Release();

		hr = field->get_ActualSize(&actual_size);
		if (FAILED(hr))
		{
			error_line = __LINE__;
			goto FAILED;
		}

		size += actual_size;
	}

FAILED:
	ReportError(hr, __FUNCTIONW__, error_line);
	return false;
}

bool ADODB::GetColumnCount(short& count)
{
	HRESULT hr = S_FALSE;
	unsigned long error_line = 0;
	long temp_count = 0;

	if (fields_ == NULL)
	{
		error_line = __LINE__;
		goto FAILED;
	}

	hr = fields_->get_Count(&temp_count);
	if (FAILED(hr))
	{
		error_line = __LINE__;
		goto FAILED;
	}

	count = static_cast<short>(temp_count);

	return true;

FAILED:
	ReportError(hr, __FUNCTIONW__, error_line);
	return false;
}

bool ADODB::GetColumnType(short column, DataType& type)
{
	return GetColumnType(_variant_t(column), type);
}

bool ADODB::GetColumnType(const wchar_t* column, DataType& type)
{
	return GetColumnType(_variant_t(column), type);
}

bool ADODB::GetColumnActualSize(short column, std::size_t& size)
{
	return GetColumnActualSize(_variant_t(column), size);
}

bool ADODB::GetColumnActualSize(const wchar_t* column, std::size_t& size)
{
	return GetColumnActualSize(_variant_t(column), size);
}

bool ADODB::GetColumnDefinedSize(short column, std::size_t& size)
{
	return GetColumnDefinedSize(_variant_t(column), size);
}

bool ADODB::GetColumnDefinedSize(const wchar_t* column, std::size_t& size)
{
	return GetColumnDefinedSize(_variant_t(column), size);
}

bool ADODB::GetColumnData(short column, DataType type, void* buffer, std::size_t buffer_size, std::size_t* actual_size)
{
	return GetColumnData(_variant_t(column), type, buffer, buffer_size, actual_size);
}

bool ADODB::GetColumnData(const wchar_t* column, DataType type, void* buffer, std::size_t buffer_size, std::size_t* actual_size)
{
	return GetColumnData(_variant_t(column), type, buffer, buffer_size, actual_size);
}

bool ADODB::SetCursorLocation(CursorLocation location)
{
	HRESULT hr = S_FALSE;
	unsigned long error_line = 0;

	if (connection_ == nullptr)
	{
		error_line = __LINE__;
		goto FAILED;
	}

	hr = connection_->put_CursorLocation(static_cast<CursorLocationEnum>(location + adUseServer));
	if (FAILED(hr))
	{
		error_line = __LINE__;
		goto FAILED;
	}

	return true;

FAILED:
	ReportError(hr, __FUNCTIONW__, error_line);
	return false;
}

bool ADODB::SetCommandTimeout(unsigned long timeout)
{
	HRESULT hr = S_FALSE;
	unsigned long error_line = 0;

	if (command_ == nullptr)
	{
		error_line = __LINE__;
		goto FAILED;
	}

	hr = command_->put_CommandTimeout(timeout);
	if (FAILED(hr))
	{
		error_line = __LINE__;
		goto FAILED;
	}

	return true;

FAILED:
	ReportError(hr, __FUNCTIONW__, error_line);
	return false;
}

bool ADODB::IsRowEnd()
{
	HRESULT hr = S_FALSE;
	unsigned long error_line = 0;
	VARIANT_BOOL eof = 0;

	if (recordset_ == NULL)
	{
		error_line = __LINE__;
		goto FAILED;
	}

	long state = 0;
	hr = recordset_->get_State(&state);
	if (FAILED(hr))
	{
		error_line = __LINE__;
		goto FAILED;
	}

	if (state)
	{
		hr = recordset_->get_EOF(&eof);
		if (FAILED(hr))
		{
			error_line = __LINE__;
			goto FAILED;
		}

		if (eof == 0)
			return false;
	}

	return true;

FAILED:
	ReportError(hr, __FUNCTIONW__, error_line);
	return false;
}

bool ADODB::MoveNextRow()
{
	HRESULT hr = S_FALSE;
	unsigned long error_line = 0;

	if (recordset_ == NULL)
	{
		error_line = __LINE__;
		goto FAILED;
	}

	hr = recordset_->MoveNext();
	if (FAILED(hr))
	{
		error_line = __LINE__;
		goto FAILED;
	}

	return true;

FAILED:
	ReportError(hr, __FUNCTIONW__, error_line);
	return false;
}

bool ADODB::IsConnected()
{
	HRESULT hr = S_FALSE;
	unsigned long error_line = 0;
	long state = 0;

	if (connection_ == nullptr)
	{
		error_line = __LINE__;
		goto FAILED;
	}

	hr = connection_->get_State(&state);
	if (FAILED(hr))
	{
		error_line = __LINE__;
		goto FAILED;
	}

	if (state == adStateClosed)
		return false;

	return true;

FAILED:
	ReportError(hr, __FUNCTIONW__, error_line);
	return false;
}

bool ADODB::ResetCommand()
{
	HRESULT hr = S_FALSE;
	unsigned long error_line = 0;
	long count = 0;

	if (parameters_ == nullptr)
	{
		error_line = __LINE__;
		goto FAILED;
	}

	hr = parameters_->get_Count(&count);
	if (FAILED(hr))
	{
		error_line = __LINE__;
		goto FAILED;
	}

	if (count > 0)
	{
		_Parameter* paramter = nullptr;
		VARIANT value;

		variant_t column(static_cast<short>(0));

		do
		{
			hr = parameters_->get_Item(column, &paramter);
			if (FAILED(hr))
			{
				error_line = __LINE__;
				goto FAILED;
			}

			paramter->Release();

			hr = paramter->get_Value(&value);
			if (FAILED(hr))
			{
				error_line = __LINE__;
				goto FAILED;
			}

			switch (value.vt)
			{
			case VT_BSTR:
				SysFreeString(value.bstrVal);
				break;
			case VT_ARRAY | VT_UI1:
				SafeArrayDestroy(value.parray);
				break;
			}

			hr = parameters_->Delete(column);			
			if (FAILED(hr))
			{
				error_line = __LINE__;
				goto FAILED;
			}

		} while (--count);
	}

	if (output_parameters_.empty() == false)
	{
		for each (auto output_parameter in output_parameters_)
			delete output_parameter;

		output_parameters_.clear();
	}

	return true;

FAILED:
	ReportError(hr, __FUNCTIONW__, error_line);
	return false;
}

bool ADODB::ReleaseRecordset()
{
	HRESULT hr = S_FALSE;
	unsigned long error_line = 0;
	
	if (recordset_ != nullptr)
	{
		if (fields_ != nullptr)
		{
			long count = 0;

			hr = fields_->get_Count(&count);
			if (FAILED(hr))
			{
				error_line = __LINE__;
				goto FAILED;
			}

			if (count > 0)
			{
				hr = recordset_->Close();
				if (FAILED(hr))
				{
					error_line = __LINE__;
					goto FAILED;
				}
			}

			fields_ = nullptr;
		}

		recordset_->Release();
		recordset_ = nullptr;
	}

	return true;

FAILED:
	ReportError(hr, __FUNCTIONW__, error_line);
	return false;
}

bool ADODB::ResetConnection()
{
	HRESULT hr = S_FALSE;
	unsigned long error_line = 0;
	reconnect_count_ = 0;

	if (connection_ == nullptr)
	{
		error_line = __LINE__;
		goto FAILED;
	}

	if (IsConnected() == false)
	{
		error_line = __LINE__;
		goto FAILED;
	}

	hr = connection_->Close();
	if (FAILED(hr))
	{
		error_line = __LINE__;
		goto FAILED;
	}

	connection_->Release();
	connection_ = nullptr;	

	while (!Connect())
	{		
		if (failover_count_ == 0 || reconnect_count_ < failover_count_)
		{
			++reconnect_count_;
			Sleep(1000);
			continue;			
		}

		error_line = __LINE__;
		goto FAILED;
	}

	return true;

FAILED:
	ReportError(hr, __FUNCTIONW__, error_line);
	return false;
}

bool ADODB::GetColumnType(const _variant_t& column, DataType& type)
{
	HRESULT hr = S_FALSE;
	unsigned long error_line = 0;
	Field* field = nullptr;

	if (fields_ == NULL)
	{
		error_line = __LINE__;
		goto FAILED;
	}

	hr = fields_->get_Item(column, &field);
	if (FAILED(hr))
	{
		error_line = __LINE__;
		goto FAILED;
	}

	field->Release();

	DataTypeEnum adtype;
	hr = field->get_Type(&adtype);
	if (FAILED(hr))
	{
		error_line = __LINE__;
		goto FAILED;
	}

	switch (adtype) {
	case adTinyInt:
	case adUnsignedTinyInt:
		type = DataTinyInt;
		break;
	case adSmallInt:
	case adUnsignedSmallInt:
		type = DataSmallInt;
		break;
	case adInteger:
	case adUnsignedInt:
		type = DataInt;
		break;
	case adBigInt:
	case adUnsignedBigInt:
		type = DataBigInt;
		break;
	case adSingle:
		type = DataFloat;
		break;
	case adDouble:
		type = DataDouble;
		break;
	case adChar:
		type = DataChar;
		break;
	case adVarChar:
		type = DataVarChar;
		break;
	case adWChar:
		type = DataWChar;
		break;
	case adVarWChar:
		type = DataVarWChar;
		break;
	case adDBTimeStamp:
		type = DataTimeStamp;
		break;
	case adBinary:
		type = DataBinary;
		break;
	case adVarBinary:
		type = DataVarBinary;
		break;
	}

	return true;

FAILED:
	ReportError(hr, __FUNCTIONW__, error_line);
	return false;
}

bool ADODB::GetColumnActualSize(const _variant_t& column, std::size_t& size)
{
	HRESULT hr = S_FALSE;
	unsigned long error_line = 0;
	Field* field = nullptr;
	long temp_size = 0;

	if (fields_ == NULL)
	{
		error_line = __LINE__;
		goto FAILED;
	}

	hr = fields_->get_Item(column, &field);
	if (FAILED(hr))
	{
		error_line = __LINE__;
		goto FAILED;
	}

	field->Release();

	hr = field->get_ActualSize(&temp_size);
	if (FAILED(hr))
	{
		error_line = __LINE__;
		goto FAILED;
	}

	size = temp_size;

	return true;

FAILED:
	ReportError(hr, __FUNCTIONW__, error_line);
	return false;
}

bool ADODB::GetColumnDefinedSize(const _variant_t& column, std::size_t& size)
{
	HRESULT hr = S_FALSE;
	unsigned long error_line = 0;
	Field* field = nullptr;
	long temp_size = 0;

	if (fields_ == NULL)
	{
		error_line = __LINE__;
		goto FAILED;
	}

	hr = fields_->get_Item(column, &field);
	if (FAILED(hr))
	{
		error_line = __LINE__;
		goto FAILED;
	}

	field->Release();

	hr = field->get_DefinedSize(&temp_size);
	if (FAILED(hr))
	{
		error_line = __LINE__;
		goto FAILED;
	}

	size = temp_size;

	return true;

FAILED:
	ReportError(hr, __FUNCTIONW__, error_line);
	return false;
}

bool ADODB::GetColumnData(const _variant_t& column, DataType type, void* buffer, std::size_t buffer_size, std::size_t* actual_size)
{
	HRESULT hr = S_FALSE;
	unsigned long error_line = 0;
	_variant_t variant(vtMissing);
	Field* field = nullptr;

	if (fields_ == NULL)
	{
		error_line = __LINE__;
		goto FAILED;
	}

	hr = fields_->get_Item(column, &field);
	if (FAILED(hr))
	{
		error_line = __LINE__;
		goto FAILED;
	}

	field->Release();

	hr = field->get_Value(&variant);
	if (FAILED(hr))
	{
		error_line = __LINE__;
		goto FAILED;
	}

	if (GetVariantData(variant, type, buffer, buffer_size, actual_size) == false)
	{
		error_line = __LINE__;
		goto FAILED;
	}

	return true;

FAILED:
	ReportError(hr, __FUNCTIONW__, error_line);
	return false;
}

bool ADODB::GetParameterData(const _variant_t& column, DataType type, void* buffer, std::size_t buffer_size, std::size_t* actual_size)
{
	HRESULT hr = S_FALSE;
	unsigned long error_line = 0;
	_variant_t variant(vtMissing);
	_Parameter* parameter = nullptr;

	if (parameters_ == NULL)
	{
		error_line = __LINE__;
		goto FAILED;
	}

	hr = parameters_->get_Item(column, &parameter);
	if (FAILED(hr))
	{
		error_line = __LINE__;
		goto FAILED;
	}

	parameter->Release();

	hr = parameter->get_Value(&variant);
	if (FAILED(hr))
	{
		error_line = __LINE__;
		goto FAILED;
	}

	if (!GetVariantData(variant, type, buffer, buffer_size, actual_size))
	{
		error_line = __LINE__;
		goto FAILED;
	}

	return true;

FAILED:
	ReportError(hr, __FUNCTIONW__, error_line);
	return false;
}

bool ADODB::GetVariantData(_variant_t& variant, DataType type, void* buffer, std::size_t buffer_size, size_t* actual_size)
{
	HRESULT hr = S_FALSE;
	unsigned long error_line = 0;

	if (variant.vt == VT_NULL) {
		if (actual_size != nullptr) {
			*actual_size = NULL_DATA;
		}
	} else {
		void* data = NULL;
		std::size_t size = 0;
		switch (variant.vt) {
		case VT_EMPTY:
			return true;
		case VT_I1:
			data = &variant.cVal;
			size = sizeof(variant.cVal);
			break;
		case VT_UI1:
			data = &variant.bVal;
			size = sizeof(variant.bVal);
			break;
		case VT_I2:
			data = &variant.iVal;
			size = sizeof(variant.iVal);
			break;
		case VT_UI2:
			data = &variant.uiVal;
			size = sizeof(variant.uiVal);
			break;
		case VT_I4:
			data = &variant.intVal;
			size = sizeof(variant.intVal);
			break;
		case VT_UI4:
			data = &variant.uintVal;
			size = sizeof(variant.uintVal);
			break;
		case VT_I8:
			data = &variant.llVal;
			size = sizeof(variant.llVal);
			break;
		case VT_UI8:
			data = &variant.ullVal;
			size = sizeof(variant.ullVal);
			break;
		case VT_R4:
			data = &variant.fltVal;
			size = sizeof(variant.fltVal);
			break;
		case VT_R8:
		case VT_DECIMAL:
			{
				if (variant.decVal.sign == 128)
				{
					sign_value_ = variant.decVal.Lo64 * -1;
					data = &sign_value_;
				}
				else
				{
					data = &variant.decVal.Lo64;
				}

				size = sizeof(variant.decVal.Lo64);
			}
			break;
		case VT_BSTR:
			data = variant.bstrVal;
			size = lstrlenW(variant.bstrVal) * sizeof(wchar_t);
			break;
		case VT_DATE:
			SYSTEMTIME systime;
			VariantTimeToSystemTime(variant.date, &systime);
			timestamp_.year = systime.wYear;
			timestamp_.month = systime.wMonth;
			timestamp_.day = systime.wDay;
			timestamp_.hour = systime.wHour;
			timestamp_.minute = systime.wMinute;
			timestamp_.second = systime.wSecond;
			timestamp_.fraction = 0;
			data = &timestamp_;
			size = sizeof(TIMESTAMP_STRUCT);
			break;
		case VT_ARRAY | VT_UI1:
			{
				HRESULT hr = SafeArrayAccessData(variant.parray, &data);
				if (FAILED(hr))
				{
					error_line = __LINE__;
					goto FAILED;
				}

				size = variant.parray->rgsabound->cElements;

				if (type == DataTimeStamp)
				{
					__int64 timestamp = 0;
					for (unsigned long offset = 0; --size; ++offset) {
						reinterpret_cast<BYTE*>(&timestamp)[offset] = reinterpret_cast<BYTE*>(data)[size];
					}

					tm time;
					errno_t error = localtime_s(&time, &timestamp);
					if (error != NO_ERROR)
					{
						error_line = __LINE__;
						goto FAILED;
					}

					timestamp_.year = static_cast<SQLSMALLINT>(time.tm_year + 1900);
					timestamp_.month = static_cast<SQLSMALLINT>(time.tm_mon + 1);
					timestamp_.day = static_cast<SQLSMALLINT>(time.tm_mday);
					timestamp_.hour = static_cast<SQLSMALLINT>(time.tm_hour);
					timestamp_.minute = static_cast<SQLSMALLINT>(time.tm_min);
					timestamp_.second = static_cast<SQLSMALLINT>(time.tm_sec);
					timestamp_.fraction = 0;

					data = &timestamp_;
					size = sizeof(TIMESTAMP_STRUCT);
				}
			}
			break;
		default:
			{
				error_line = __LINE__;
				goto FAILED;
			}
		}

		switch (type)
		{
		case DataChar:
		case DataVarChar:
			{
				CW2AEX<STRING_CONVERT_BUFFER_SIZE> converter(static_cast<wchar_t*>(data));
				const char* conversion_data = converter;
				if (conversion_data == nullptr)
				{
					error_line = __LINE__;
					goto FAILED;
				}

				size = lstrlenA(conversion_data);

				if (size > buffer_size)
					size = buffer_size;

				memcpy(buffer, conversion_data, size);
			}
			break;
		default:
			{
				if (size > buffer_size)
					size = buffer_size;

				memcpy(buffer, data, size);
			}
		}

		if (actual_size != NULL) {
			*actual_size = size;
		}
	}

	return true;

FAILED:
	ReportError(hr, __FUNCTIONW__, error_line);
	return false;
}

void ADODB::ReportError(HRESULT result, const wchar_t* func, unsigned long error_line)
{
	if (error_reporter_ != nullptr)
	{		
		BSTR command_text = nullptr;
		_bstr_t sql;

		if (command_ == nullptr)
			goto FINALIZE;

		if (FAILED(command_->get_CommandText(&command_text)))
			goto FINALIZE;

		if (command_text == nullptr)
			goto FINALIZE;

		sql.Attach(command_text);
		sql += L"{ ";

		if (parameters_ == nullptr)
			goto FINALIZE;

		long count = 0;
		if (FAILED(parameters_->get_Count(&count)))
			goto FINALIZE;

		if (count > 0)
		{
			_Parameter* paramter = nullptr;			
			short column = 0;

			while (count--)
			{
				if(FAILED(parameters_->get_Item(variant_t(column++), &paramter)))
					goto FINALIZE;

				if (paramter == nullptr)
					goto FINALIZE;

				paramter->Release();

				ParameterDirectionEnum paramter_direction;
				if (FAILED(paramter->get_Direction(&paramter_direction)))
					goto FINALIZE;

				switch (paramter_direction)
				{
				case adParamInput:
				case adParamInputOutput:
					{
						_variant_t value;
						if (FAILED(paramter->get_Value(&value)))
							goto FINALIZE;

						switch (value.vt)
						{
						case VT_BSTR:
							sql += L"'";
							sql += value.bstrVal;
							sql += L"'";
							break;
						case VT_NULL:
							sql += L"null";
							break;
						default:
							value.ChangeType(VT_BSTR);
							sql += value.bstrVal;
							break;
						}
					}
					break;
				default:
					sql += L"?";
					break;
				}

				if (count)
					sql += L", ";
			}
		}

		sql += L" }";

FINALIZE:
		IErrorInfo* info;
		GetErrorInfo(NULL, &info);		
				
		BSTR desc = nullptr;
		if (info)
			info->GetDescription(&desc);

		_com_error error = _com_error(result);
		error_reporter_(*this, result, (desc) ? desc : error.ErrorMessage(), func, error_line, sql);

		if (desc)
			SysFreeString(desc);
	}
}