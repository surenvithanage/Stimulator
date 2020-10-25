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
	 * Value of 'k' to use for k-means.
	 */
	int k = -1;

	/**
	 * Whether details of the computation must be logged.
	 */
	bool verbose = false;

	/**
	 * File from which values to cluster are read.
	 */
	char* inputFile = nullptr;

	/**
	 * File to which the computed memberships must be written.
	 */
	char* membershipOutputFile = nullptr;

	/**
	 * File to which the computed centroids must be written.
	 */
	char* centroidOutputFile = nullptr;

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
