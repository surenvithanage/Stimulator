#ifndef PARALLAX_RESULT_H
#define PARALLAX_RESULT_H

#include <chrono>
#include <string>
#include <curl/curl.h>

namespace Parallax
{
	/**
	 * Result of downloading a file using libparallax. Contains statistics.
	 */
	struct ParallaxResult
	{
		/**
		 * Duration across all requests.
		 */
		std::chrono::nanoseconds duration;

		/**
		 * Value of the Content-Type header, i.e. the MIME type of the download.
		 */
		std::string contentType;

		/**
		 * Value of the Content-Length header, i.e. the size of the download, in bytes.
		 */
		curl_off_t contentLength;
	};
}

#endif
