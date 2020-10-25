#include "Args.h"

Args parseArgs(int argc, char** argv)
{
	Args a;

	char c;
	while ((c = getopt(argc, argv, "n:t:hvm")) != -1)
	{
		a.isParsed = true;

		switch (c)
		{
			case 'n': { a.n = atoi(optarg); break; }
			case 't': { a.threadLimit = atoi(optarg); break; }
			case 'h': { a.showHelp = true; break; }
			case 'v': { a.isVerbose = true; break; }
			case 'm': { a.showMatrices = true; break; }
			case '?': { a.hasError = true; break; }
		}
	}

	return a;
}
