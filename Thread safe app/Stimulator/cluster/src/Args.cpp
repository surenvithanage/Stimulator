#include "Args.h"

Args parseArgs(int argc, char** argv)
{
	Args a;

	char c;
	while ((c = getopt(argc, argv, "i:k:m:c:hv")) != -1)
	{
		a.isParsed = true;

		switch (c)
		{
			case 'k': { a.k = atoi(optarg); break; }
			case 'i': { a.inputFile = optarg; break; }
			case 'm': { a.membershipOutputFile = optarg; break; }
			case 'c': { a.centroidOutputFile = optarg; break; }
			case 'h': { a.showHelp = true; break; }
			case 'v': { a.verbose = true; break; }
			case '?': { a.hasError = true; break; }
		}
	}

	return a;
}
