#ifndef ARGS_H
#define ARGS_H

#include <string>

struct Args
{
	/**
	 * URL of the file to download.
	 */
	std::string url;

	/**
	 * Local path to which the file is saved.
	 */
	std::string output;

	/**
	 * Number of threads to use.
	 */
	int t = 3;

	/**
	 * Parses the specified CLI arguments into thie instance of `Args`. Exits the program with appropriate user feedback
	 * if a parsing error is encountered or help is requested.
	 */
	void parse(int argc, char** argv);
};

#endif
