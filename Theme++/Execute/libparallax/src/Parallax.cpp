#include "Parallax.h"

#include <iostream>
#include <fstream>
#include <string>
#include <atomic>
#include <chrono>
#include <string.h>
#include <curl/curl.h>
#include <omp.h>
#include <unistd.h>

#include "util.h"

namespace chrono = std::chrono;

namespace Parallax
{
	struct RequestWork
	{
		curl_off_t start;
		curl_off_t end;

		FILE* file;

		long totalBytes;
		std::atomic_long* completedBytes;
		ProgressCallback callback;

		CURL* curl;
		CURLcode curlE = CURLcode::CURLE_OK;
	};

	size_t writeFunction(char *ptr, size_t size, size_t nmemb, void *userdata)
	{
		RequestWork* work = (RequestWork*)userdata;

		work->completedBytes->fetch_add(size * nmemb);

		if (work->callback != nullptr)
		{
			work->callback((work->completedBytes->load() * 1.0) / work->totalBytes);
		}

		return fwrite(ptr, size, nmemb, work->file);
	}

	ParallaxResult download(std::string url, std::string filePath, unsigned int nThreads, ProgressCallback callback)
	{
		// Create root cURL instance.
		CURL* rootCurl = curl_easy_init();
		CURLcode curlE;

		if (rootCurl == nullptr)
		{
			throw ParallaxException(ParallaxExceptionCode::FAILED_INIT, "Failed to initialize cURL");
		}

		#define CLEANUP()\
		{\
			curl_easy_cleanup(rootCurl);\
		}

		curl_easy_setopt(rootCurl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(rootCurl, CURLOPT_FAILONERROR, true);

		// Start stopwatch.
		auto startTime = chrono::high_resolution_clock::now();

		// Perform HEAD request.
		curl_easy_setopt(rootCurl, CURLOPT_NOBODY, true);

		curlE = curl_easy_perform(rootCurl);

		if (!(curlE == CURLcode::CURLE_OK || curlE == CURLcode::CURLE_PARTIAL_FILE))
		{
			CLEANUP();
			throw ParallaxException(
				ParallaxExceptionCode::FAILED_HEAD, std::string("Failed HEAD request with code ").append(std::to_string(curlE)),
				ParallaxExceptionDataType::CURL_CODE, new CURLcode { curlE }
			);
		}

		// Retrieve Content-Length.
		curl_off_t contentLength;
		curlE = curl_easy_getinfo(rootCurl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &contentLength);

		if (!(curlE == CURLcode::CURLE_OK))
		{
			CLEANUP();
			throw ParallaxException(
				ParallaxExceptionCode::FAILED_CONTENT_LENGTH, std::string("Failed to retrieve Content-Length with code ").append(std::to_string(curlE)),
				ParallaxExceptionDataType::CURL_CODE, new CURLcode { curlE }
			);
		}

		// Retrieve Content-Type.
		char* _contentType = nullptr;
		curl_easy_getinfo(rootCurl, CURLINFO_CONTENT_TYPE, &_contentType);
		std::string contentType((_contentType == nullptr) ? "" : _contentType);

		// Perform GET_0 request.
		{
			// Open output file for writing.
			FILE* file = fopen(filePath.c_str(), "w");
			if (file == nullptr)
			{
				throw ParallaxException(
					ParallaxExceptionCode::FAILED_IO, std::string("Failed to open file ").append(filePath).append(" for writing"),
					ParallaxExceptionDataType::STRING, new std::string(filePath)
				);
			}

			#undef CLEANUP
			#define CLEANUP()\
			{\
				fclose(file);\
				curl_easy_cleanup(rootCurl);\
			}

			std::atomic_long completedBytes { 0 };
			RequestWork get0RequestWork { 0, 0, file, contentLength, &completedBytes, callback, rootCurl };

			// Request a 0-byte range. If the server doesn't support range requests, the entire file will be served to
			// the output file with 200 response. If the server does support range requests, 0 or 1 byte(s) will be
			// served with a 206 response.
			curl_easy_setopt(rootCurl, CURLOPT_RANGE, "0-0");

			curl_easy_setopt(rootCurl, CURLOPT_HTTPGET, true);
			curl_easy_setopt(rootCurl, CURLOPT_WRITEDATA, &get0RequestWork);
			curl_easy_setopt(rootCurl, CURLOPT_WRITEFUNCTION, writeFunction);

			curlE = curl_easy_perform(rootCurl);
			if (curlE != CURLcode::CURLE_OK)
			{
				CLEANUP();
				throw ParallaxException(
					ParallaxExceptionCode::FAILED_GET_0, std::string("Failed GET_0 request with code ").append(std::to_string(curlE)),
					ParallaxExceptionDataType::CURL_CODE, new CURLcode { curlE }
				);
			}

			long responseCode;
			curl_easy_getinfo(rootCurl, CURLINFO_RESPONSE_CODE, &responseCode);

			if (responseCode == 206)
			{
				fclose(file);
				// The server does support range quests; continue with parallel download.
			}
			else if (responseCode == 200)
			{
				// The server doesn't suppport range requests; the file has been downloaded. Cleanup & quit.
				CLEANUP();
				return { chrono::duration_cast<chrono::nanoseconds>(chrono::high_resolution_clock::now() - startTime), contentType, contentLength };
			}
			else
			{
				CLEANUP();
				throw ParallaxException(
					ParallaxExceptionCode::FAILED_GET_0, std::string("Failed GET_0 request with status ").append(std::to_string(curlE)),
					ParallaxExceptionDataType::LONG, new long { responseCode }
				);
			}
		}

		#undef CLEANUP
		#define CLEANUP()\
		{\
			curl_easy_cleanup(rootCurl);\
		}

		// Make temp directory to download parts to.
		// TODO: Replace with /tmp.
		char* tempDir = mkdtemp(strdup("parallaxXXXXXX"));
		if (tempDir == nullptr)
		{
			CLEANUP();
			throw ParallaxException(
				ParallaxExceptionCode::FAILED_IO, std::string("Failed to create temporary directory ").append(tempDir),
				ParallaxExceptionDataType::STRING, new std::string(tempDir)
			);
		}

		std::atomic_long completedBytes { 0 };
		curl_off_t bytesPerThread = contentLength / nThreads;
		RequestWork* getNRequestWork = new RequestWork[nThreads];

		#undef CLEANUP
		#define CLEANUP()\
		{\
			delete[] getNRequestWork;\
			deleteDir(tempDir);\
			curl_easy_cleanup(rootCurl);\
		}

		for (unsigned int i = 0; i < nThreads; i++)
		{
			curl_off_t nBytes =  (i < nThreads - 1) ? bytesPerThread : (contentLength - (bytesPerThread * i));
			curl_off_t start = bytesPerThread * i;
			curl_off_t end = start + nBytes;

			// Create thread-local cURL instance.
			CURL* curl = curl_easy_init();
			if (curl == nullptr)
			{
				CLEANUP();
				throw ParallaxException(ParallaxExceptionCode::FAILED_INIT, "Failed to initialize cURL");
			}

			// Open temporary file for writing.
			std::string fp = std::string(tempDir).append("/").append(std::to_string(i));
			FILE* file = fopen(fp.c_str(), "w");
			if (file == nullptr)
			{
				curl_easy_cleanup(curl);
				CLEANUP();
				throw ParallaxException(
					ParallaxExceptionCode::FAILED_IO, std::string("Failed to open temporary file ").append(fp).append(" for writing"),
					ParallaxExceptionDataType::STRING, new std::string(fp)
				);
			}

			// Setup thread-local cURL to download part.
			curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
			curl_easy_setopt(curl, CURLOPT_FAILONERROR, true);

			curl_easy_setopt(curl, CURLOPT_HTTPGET, true);
			curl_easy_setopt(curl, CURLOPT_RANGE, std::to_string(start).append("-").append(std::to_string(end)).c_str());

			getNRequestWork[i] = { start, end, file, contentLength, &completedBytes, callback, curl };

			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &getNRequestWork[i]);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunction);
		}

		// Download parts in parallel, close respective files.
		#pragma omp parallel for
		for (unsigned int i = 0; i < nThreads; i++)
		{
			getNRequestWork[i].curlE = curl_easy_perform(getNRequestWork[i].curl);
			fclose(getNRequestWork[i].file);
			curl_easy_cleanup(getNRequestWork[i].curl);
		}

		// Ensure no part download failed.
		for (unsigned int i = 0; i < nThreads; i++)
		{
			if (getNRequestWork[i].curlE != CURLcode::CURLE_OK)
			{
				CLEANUP();
				throw ParallaxException(
					ParallaxExceptionCode::FAILED_GET_N,
					std::string("Failed GET_N request of range ").append(std::to_string(getNRequestWork[i].start)).append("-")
						.append(std::to_string(getNRequestWork[i].end)).append(" with code ").append(std::to_string(getNRequestWork[i].curlE)),
					ParallaxExceptionDataType::CURL_CODE, new CURLcode { curlE }
				);
			}
		}

		delete[] getNRequestWork;

		#undef CLEANUP
		#define CLEANUP()\
		{\
			deleteDir(tempDir);\
			curl_easy_cleanup(rootCurl);\
		}

		// Merge downloaded parts into output file.
		std::ofstream of(filePath, std::ios_base::binary);
		if (!of.is_open())
		{
			CLEANUP();
			throw ParallaxException(
				ParallaxExceptionCode::FAILED_IO, std::string("Failed to open file ").append(filePath).append(" for writing"),
				ParallaxExceptionDataType::STRING, new std::string(filePath)
			);
		}

		#undef CLEANUP
		#define CLEANUP()\
		{\
			of.close();\
			deleteDir(tempDir);\
			curl_easy_cleanup(rootCurl);\
		}

		for (unsigned int i = 0; i < nThreads; i++)
		{
			std::string fp = std::string(tempDir).append("/").append(std::to_string(i));
			std::ifstream f(fp, std::ios_base::binary);

			if (!f.is_open())
			{
				throw ParallaxException(
					ParallaxExceptionCode::FAILED_IO, std::string("Failed to open file ").append(filePath).append(" for reading"),
					ParallaxExceptionDataType::STRING, new std::string(fp)
				);
			}

			// Binary-concatenate part to output file, delete part afterwards.
			of << f.rdbuf();
			f.close();
			unlink(fp.c_str());
		}

		CLEANUP();
		return { chrono::duration_cast<chrono::nanoseconds>(chrono::high_resolution_clock::now() - startTime), contentType, contentLength };
	}
}
