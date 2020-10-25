#include <iostream>
#include <fstream>
#include <string.h>
#include <vector>
#include <chrono>

#include "Args.h"
#include "util.h"
#include "kmeans.h"

namespace chrono = std::chrono;

int main(int argc, char** argv)
{
	// Seed RNG
	srand(time(nullptr));

	// Parse args
	char* progName = argv[0];
	if (progName[0] == '\0')
		progName = strdup("matrix-multiplier");

	Args args = parseArgs(argc, argv);

	if (args.hasError)
	{
		std::cout << "Run " << progName << " -h to view usage information." << std::endl;
		return -2;
	}

	if (!args.isParsed || args.showHelp)
	{
		std::cout << "Usage:\n";
		std::cout << "  " << progName << " -k K -i INPUT [-m MEMBERSHIP_OUTPUT] [-c CENTROID_OUTPUT] [-v]\n";
		std::cout << "  " << progName << " -h\n";

		std::cout << "\nArguments:\n";
		std::cout << "  -k K                 : Number of clusters to be computed.\n";
		std::cout << "  -i INPUT             : File from which values to cluster are read.\n";
		std::cout << "  -m MEMBERSHIP_OUTPUT : File to which computed memberships should be written.\n";
		std::cout << "  -c CENTROID_OUTPUT   : File to which computed centroids should be written.\n";
		std::cout << "  -v                   : Print (verbose) details during computation.\n";
		std::cout << "  -h                   : Shows this help message.\n";

		std::cout << std::endl;
		return args.showHelp ? 0 : -1;
	}

	if (args.inputFile == nullptr)
	{
		std::cerr << "No input file specified." << std::endl;
		return -8;
	}

	// Time & execute clustering
	auto tStart = chrono::high_resolution_clock::now();

	KMeansResult result = kmeans(args);

	// Terminate if non-zero return code, of non-root process (when distributed).
	if (result.returnCode != 0 || !result.isRoot)
		return result.returnCode;

	auto tDurationNs = chrono::duration_cast<chrono::nanoseconds>(chrono::high_resolution_clock::now() - tStart).count();

	// Write memberships
	if (args.membershipOutputFile != nullptr)
	{
		std::ofstream f(args.membershipOutputFile);
		if (!f.is_open())
		{
			std::cerr << "Failed to open " << args.membershipOutputFile << " for writing memberships" << std::endl;
			return -6;
		}

		for (int i = 0; i < result.n; i++)
			f << result.memberships[i] << '\n';

		f.close();

		std::cout << "Wrote memberships to " << args.membershipOutputFile << std::endl;
	}

	// Write centroids
	if (args.centroidOutputFile != nullptr)
	{
		std::ofstream f(args.centroidOutputFile);
		if (!f.is_open())
		{
			std::cerr << "Failed to open " << args.centroidOutputFile << " for writing centroids" << std::endl;
			return -7;
		}

		for (int i = 0; i < args.k; i++)
			f << result.centroids[i] << '\n';

		f.close();

		std::cout << "Wrote centroids to " << args.centroidOutputFile << std::endl;
	}

	// Output time
	std::cout << "Clustering took " << tDurationNs << " ns" << " (" << (tDurationNs / 1e9f) << " s)" << std::endl;
}
