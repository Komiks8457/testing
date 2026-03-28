#ifndef BSLIB_SQL_PARAM_H_
#define BSLIB_SQL_PARAM_H_

#include "adodb.h"
#include <ostream>
#include <streambuf>

class SQLParam
{
public:
	SQLParam();
	SQLParam(ADODB::ParameterDirection direction, ADODB::DataType type, void* buffer, std::size_t size, std::size_t* actual_size = nullptr);
	virtual ~SQLParam();

	ADODB::ParameterDirection direction() const { return direction_; }
	ADODB::DataType type() const { return type_; }
	void* buffer() const { return buffer_; }
	std::size_t size() const { return size_; }
	std::size_t* actual_size() const { return actual_size_; }

	void set_direction(ADODB::ParameterDirection direction) { direction_ = direction; }
	void set_type(ADODB::DataType type) { type_ = type; }
	void set_buffer(void* buffer) { buffer_ = buffer; }
	void set_size(std::size_t size) { size_ = size; }

protected:
	ADODB::ParameterDirection direction_;
	ADODB::DataType type_;
	void* buffer_;
	std::size_t size_;
	std::size_t* actual_size_;
};

class SQLVariableParam : public std::wostream
{
public:
	SQLVariableParam(std::size_t size);
	~SQLVariableParam();

	void Reset();
	bool IsEmpty() const;

private:
	friend class SQLProcedureQuery;
	struct Buffer : std::wstreambuf
	{
		wchar_t* front_;
		wchar_t* back_;

		Buffer(std::size_t size) : front_(new wchar_t[size]), back_(front_ + (size - 1))
		{
			ZeroMemory(front_, size * sizeof(wchar_t));
			setp(front_, back_);
		}

		~Buffer()
		{
			delete [] front_;
		}

		void Clear()
		{
			std::size_t size = pptr() - pbase();
			if (size > 0)
			{
				ZeroMemory(front_, size * sizeof(wchar_t));
				setp(front_, back_);
			}
		}

		std::size_t GetSize() const { return (pptr() - pbase()); }
	};
	Buffer buffer_;
	SQLParam base_param_;
};

#endif // BSLIB_SQL_PARAM_H_
