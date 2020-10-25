#ifndef PARALLAX_EXCEPTION_H
#define PARALLAX_EXCEPTION_H

#include <exception>
#include <string>
#include <curl/curl.h>

namespace Parallax
{
	/**
	 * Represents the cause of the `ParallaxException`.
	 */
	enum ParallaxExceptionCode
	{
		/**
		 * Failed to initialize a component (e.g. cURL).
		 */
		FAILED_INIT,

		/**
		 * Failed the HEAD request.
		 */
		FAILED_HEAD,

		/**
		 * Failed to retrieve value of the the Content-Length header from the HEAD request.
		 */
		FAILED_CONTENT_LENGTH,

		/**
		 * Failed to read/write a file.
		 */
		FAILED_IO,

		/**
		 * Failed the GET_0 request.
		 */
		FAILED_GET_0,

		/**
		 * Failed the GET_N request.
		 */
		FAILED_GET_N,
	};

	/**
	 * Represents the data type contained in the `data` field of the `ParallaxException`. If the type is not `NONE`,
	 * `data` can be safely casted to the corresponding type.
	 */
	enum ParallaxExceptionDataType
	{
		/**
		 * No data (`void*`).
		 */
		NONE,

		/**
		 * Integer (`int*`).
		 */
		INT,

		/**
		 * Long (`long*`).
		 */
		LONG,

		/**
		 * cURL code (`CURLcode*`).
		 */
		CURL_CODE,

		/**
		 * String `std::string*`.
		 */
		STRING,
	};

	/**
	 * Represents an exception thrown by `libparallax`.
	 */
	struct ParallaxException: public std::exception
	{
		ParallaxExceptionCode code;
		std::string message;

		ParallaxExceptionDataType dataType;
		void* data;

		/**
		 * Constructs an exception of the specified code and message.
		 */
		ParallaxException(ParallaxExceptionCode _code, std::string _message);

		/**
		 * Constructs an exception of the specified code and message, and containing the specified data of the specified
		 * type.
		 */
		ParallaxException(ParallaxExceptionCode _code, std::string _message, ParallaxExceptionDataType _dataType, void* _data);

		/**
		 * Releases `data`.
		 */
		~ParallaxException();

		/**
		 * Returns the description of the exception.
		 */
		const char* what();
	};
}

#endif
