#include "StdAfx.h"
#include "JsonValue.h"

JSONValue::JSONValue() : cursor_(nullptr)
{
}

JSONValue::JSONValue(const char* value) : Json::Value(value), cursor_(nullptr)
{
}

JSONValue::JSONValue(const wchar_t* value) : Json::Value(CW2AEX<STRING_CONVERT_BUFFER_SIZE>(value)), cursor_(nullptr)
{
}

JSONValue::JSONValue(const std::string& value) : Json::Value(value.c_str()), cursor_(nullptr)
{
}

JSONValue::JSONValue(const std::wstring& value) : Json::Value(CW2AEX<STRING_CONVERT_BUFFER_SIZE>(value.c_str())), cursor_(nullptr)
{
}

JSONValue::JSONValue(DWORD value) : Json::Value(static_cast<Json::UInt>(value)), cursor_(nullptr)
{
}

const Json::Value& JSONValue::operator[](const char* key) const
{
	return Json::Value::operator[](key);
}

JSONValue& JSONValue::operator[](const char* key)
{
	cursor_ = &Json::Value::operator[](key);
	return *this;
}

JSONValue& JSONValue::operator=(JSONValue other)
{
	if (cursor_)
	{
		cursor_->swap(other);
		cursor_ = nullptr;
	}

	return *this;
}

void JSONValue::Serialize(std::wstring& text)
{
	Json::FastWriter writer;
	text = CA2WEX<1024>(writer.write(*this).c_str());
}
