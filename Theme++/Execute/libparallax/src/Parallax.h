#ifndef PARALLAX_H
#define PARALLAX_H

#include <string>
#include <functional>

#include "ParallaxException.h"
#include "ParallaxResult.h"

namespace Parallax
{
	/**
	 * Function to be invoked when `libparallax` receives data being downloaded, invoked with a `double` in the range
	 * [0,1] representing the percentage progress.
	 */
	typedef std::function<void(double)> ProgressCallback;

	/**
	 * Simultaneously downloads using `nThreads` threads, a file from the specified `url` to the specified `filePath`.
	 */
	ParallaxResult download(std::string url, std::string filePath, unsigned int nThreads, ProgressCallback callback = nullptr);
}

#endif
