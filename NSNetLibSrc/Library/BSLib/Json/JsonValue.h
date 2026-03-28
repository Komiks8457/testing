#pragma once

#define STRING_CONVERT_BUFFER_SIZE 1024

#pragma warning( disable : 4996) //jsoncpp issue workaround https://github.com/open-source-parsers/jsoncpp/issues/686

#include "windows.h"
#include <string>
#include "../../../ThirdPartyLib/jsoncpp-1.8.0/include/json/json.h"

#ifdef _WIN64
	#ifdef _DEBUG
		#pragma comment(lib, "lib_jsonx64d.lib")
	#else
		#pragma comment(lib, "lib_jsonx64.lib")
	#endif
#else
	#ifdef _DEBUG
		#pragma comment(lib, "lib_jsond.lib")
	#else
		#pragma comment(lib, "lib_json.lib")
	#endif
#endif 

class JSONValue : Json::Value
{
public:
	JSONValue();
	JSONValue(const char* value);
	JSONValue(const wchar_t* value);
	JSONValue(const std::string& value);
	JSONValue(const std::wstring& value);
	JSONValue(DWORD value);

	template<typename T>
	JSONValue(T value) : Json::Value(value), cursor_(nullptr)
	{
	}

	const Json::Value& operator[](const char* key) const;
	JSONValue& operator[](const char* key);
	JSONValue& operator=(JSONValue other);

	void Serialize(std::wstring& text);

private:
	Json::Value* cursor_;
};
