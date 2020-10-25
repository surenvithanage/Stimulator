#include "ParallaxException.h"

namespace Parallax
{
	/**
	 * Maps `ParallaxExceptionCode`s to string representations of their names.
	 */
	const char* exceptionCodeToString(ParallaxExceptionCode code)
	{
		switch (code)
		{
			case ParallaxExceptionCode::FAILED_INIT: return "FAILED_INIT";
			case ParallaxExceptionCode::FAILED_HEAD: return "FAILED_HEAD";
			case ParallaxExceptionCode::FAILED_CONTENT_LENGTH: return "FAILED_CONTENT_LENGTH";
			case ParallaxExceptionCode::FAILED_IO: return "FAILED_IO";
			case ParallaxExceptionCode::FAILED_GET_0: return "FAILED_GET_0";
			case ParallaxExceptionCode::FAILED_GET_N: return "FAILED_GET_N";
			default: return "?";
		}
	}

	ParallaxException::ParallaxException(ParallaxExceptionCode _code, std::string _message):
		code(_code),
		message(_message.c_str()),
		dataType(ParallaxExceptionDataType::NONE),
		data(nullptr)
	{
	}

	ParallaxException::ParallaxException(ParallaxExceptionCode _code, std::string _message, ParallaxExceptionDataType _dataType, void* _data):
		code(_code),
		message(_message),
		dataType(_dataType),
		data(_data)
	{
	}

	ParallaxException::~ParallaxException()
	{
		if (data != nullptr)
		{
			switch (dataType)
			{
				case ParallaxExceptionDataType::NONE: { break; }
				case ParallaxExceptionDataType::INT: { delete (int*)data; break; }
				case ParallaxExceptionDataType::LONG: { delete (long*)data; break; }
				case ParallaxExceptionDataType::CURL_CODE: { delete (CURLcode*)data; break; }
				case ParallaxExceptionDataType::STRING: { delete (std::string*)data; break; }
			}
		}
	}

	const char* ParallaxException::what()
	{
		return std::string("ParallaxException (").append(exceptionCodeToString(code)).append("): ").append(message).c_str();
	}
}
