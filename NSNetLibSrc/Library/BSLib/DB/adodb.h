#ifndef BSLIB_ADODB_H_
#define BSLIB_ADODB_H_

#include <cstddef>
#include <functional>
#include <comutil.h>
#include <sqltypes.h>

struct _Connection;
struct _Command;
struct _Recordset;
struct Parameters;
struct Fields;

class ADODB
{
public:
	enum CommandType
	{
		CommandText,
		CommandSP
	};

	enum ParameterDirection
	{
		ParameterInput,
		ParameterOutput,
		ParameterInputOutput,
		ParameterReturnValue
	};

	enum CursorLocation
	{
		CursorServer,
		CursorClient,
	};

	enum DataType
	{
		DataTinyInt,
		DataSmallInt,
		DataInt,
		DataBigInt,
		DataFloat,
		DataDouble,
		DataChar,
		DataVarChar,
		DataWChar,
		DataVarWChar,
		DataLongVarWChar,
		DataDateTime,
		DataSmallDateTime,
		DataTimeStamp,
		DataBinary,
		DataVarBinary,
	};

	static const size_t NULL_DATA = 0;

	typedef std::function<void(const ADODB& owner, HRESULT result, const wchar_t* desc, const wchar_t* func, unsigned long line, const wchar_t* sql)> ErrorReporter;

	ADODB();
	virtual ~ADODB();

	bool Connect(const wchar_t* ip, unsigned short port, const wchar_t* db, const wchar_t* user, const wchar_t* password, unsigned long timeout = 0);
	bool Connect(const wchar_t* connection_string, unsigned long timeout = 0);
	bool Connect(unsigned long timeout = 0);
	bool Disconnect();

	bool SetCommand(CommandType type, const wchar_t* sql);
	bool SetParameter(_variant_t column, ParameterDirection direction, DataType type, void* buffer, std::size_t size, std::size_t* actual_size = nullptr);
	bool ExecuteCommand();

	bool GetRowCount(unsigned long& count);
	bool GetRowSize(std::size_t& size);

	bool GetColumnCount(short& count);
	bool GetColumnType(short column, DataType& type);
	bool GetColumnType(const wchar_t* column, DataType& type);
	bool GetColumnActualSize(short column, std::size_t& size);
	bool GetColumnActualSize(const wchar_t* column, std::size_t& size);
	bool GetColumnDefinedSize(short column, std::size_t& size);
	bool GetColumnDefinedSize(const wchar_t* column, std::size_t& size);
	bool GetColumnData(short column, DataType type, void* buffer, std::size_t buffer_size, std::size_t* actual_size = nullptr);
	bool GetColumnData(const wchar_t* column, DataType type, void* buffer, std::size_t buffer_size, std::size_t* actual_size = nullptr);

	bool SetCursorLocation(CursorLocation location);
	bool SetCommandTimeout(unsigned long timeout = 30);

	bool IsRowEnd();
	bool MoveNextRow();

	bool IsConnected();

	void set_error_reporter(const ErrorReporter& error_reporter) { error_reporter_ = error_reporter; }

	DWORD reconnect_count() const { return reconnect_count_; }
	DWORD retry_count() const { return retry_count_; }

	static DWORD failover_count() { return failover_count_; }

private:
	struct OutputParameter
	{
		_variant_t column;
		DataType type;
		void* buffer;
		std::size_t buffer_size;
		std::size_t* actual_size;

		DECLARE_OBJECT_POOL(OutputParameter, ChunkAllocatorMT)
	};

	bool ResetCommand();
	bool ReleaseRecordset();
	bool ResetConnection();

	bool GetColumnType(const _variant_t& column, DataType& type);
	bool GetColumnActualSize(const _variant_t& column, std::size_t& size);
	bool GetColumnDefinedSize(const _variant_t& column, std::size_t& size);
	bool GetColumnData(const _variant_t& column, DataType type, void* buffer, std::size_t buffer_size, std::size_t* actual_size);

	bool GetParameterData(const _variant_t& column, DataType type, void* buffer, std::size_t buffer_size, std::size_t* actual_size);
	bool GetVariantData(_variant_t& variant, DataType type, void* buffer, std::size_t buffer_size, std::size_t* actual_size);

	void ReportError(HRESULT result, const wchar_t* func, unsigned long error_line);

	static ADODB* co_init_;
	static DWORD failover_count_;

	std::wstring connection_string_;
	_Connection* connection_;
	_Command* command_;
	int command_type_;
	_Recordset* recordset_;
	Parameters* parameters_;
	Fields* fields_;
	ErrorReporter error_reporter_;
	typedef std::list<OutputParameter*> OUTPUT_PARAMETERS;
	OUTPUT_PARAMETERS output_parameters_;
	TIMESTAMP_STRUCT timestamp_;	
	DWORD reconnect_count_;
	DWORD retry_count_;
	LONGLONG sign_value_;
};

#endif // BSLIB_ADODB_H_