#ifndef ARGS_H
#define ARGS_H

#include <stdlib.h>
#include <getopt.h>

/**
 * Represents CLI arguments passed to the application.
 */
struct Args
{
	/**
	 * Whether any arguments were parsed.
	 */
	bool isParsed = false;

	/**
	 * Whether any errors occured during argument parsing.
	 */
	bool hasError = false;

	/**
	 * Size of the matrices to be calculated.
	 */
	int n = 0;

	/**
	 * Thread limit imposed if applicable.
	 */
	int threadLimit = 0;

	/**
	 * Logs detailed information about the calculation.
	 */
	bool isVerbose = false;

	/**
	 * Shows the matrices.
	 */
	bool showMatrices = false;

	/**
	 * Shows a help message.
	 */
	bool showHelp = false;
};

/**
 * Uses GNU Getopt (https://www.gnu.org/software/libc/manual/html_node/Getopt.html) to parse the specified CLI arguments.
 * Writes argument-related errors to `std::cerr`.
 */
Args parseArgs(int argc, char** argv);

#endif
