#ifdef CLUSTER_MODE_SERIAL

#include <fstream>

#include "kmeans.h"
#include "util.h"

KMeansResult kmeans(Args args)
{
	// Retrieve numbers from input file
	std::vector<double> arr;

	{
		std::ifstream f(args.inputFile);
		if (!f.is_open())
		{
			return { -9 };
		}
		double d;
		while (f >> d)
			arr.push_back(d);
	}

	int n = arr.size();

	if (n == 0)
	{
		std::cerr << "No values to cluster." << std::endl;
		return { -3 };
	}

	if (args.k <= 0)
	{
		std::cerr << "K must be positive." << std::endl;
		return { -4 };
	}

	if (args.k > n)
	{
		std::cerr << "K must be less than the number of values to cluster." << std::endl;
		return { -5 };
	}

	std::cout << "n = " << n << std::endl;
	std::cout << "k = " << args.k << std::endl;

	if (args.verbose)
	{
		std::cout << "arr = ";
		printArr(n, arr.data());
		std::cout << '\n' << std::endl;
	}

	int* newMemberships = nullptr;
	int* oldMemberships = nullptr;

	// Calculate initial centroids randomly.
	double* centroids = new double[n];
	for (int i = 0; i < args.k; i++)
	{
		while (true)
		{
			centroids[i] = rand() % n;
			bool occurs = false;

			for (int j = 0; !occurs && j < i; j++)
				occurs = centroids[j] == centroids[i];

			if (!occurs)
				break;
		}
	}

	for (int i = 0; i < args.k; i++)
		centroids[i] = arr[(int)centroids[i]];

	do
	{
		// Initialize oldMemberships or newMemberships if uninitialized (during the first 2 iteration), or copy
		// newMemberships to oldMemberships (during subsequent iterations).
		bool populateOld = false;

		if (oldMemberships == nullptr)
		{
			oldMemberships = new int[n];
			populateOld = true;
		}
		else if (newMemberships == nullptr)
		{
			newMemberships = new int[n];
		}
		else
		{
			for (int i = 0;  i < n; i++)
				oldMemberships[i] = newMemberships[i];
		}

		// Calculate differences with current centroids
		double** diffs = new double*[args.k];
		for (int i = 0; i < args.k; i++)
		{
			diffs[i] = new double[n];

			for (int j = 0; j < n; j++)
				diffs[i][j] = centroids[i] - arr[j];
		}

		// Populate memberships according to differences
		for (int i = 0; i < n; i++)
		{
			int min = 0;
			for (int j = 1; j < args.k; j++)
			{
				if (abs(diffs[j][i]) < abs(diffs[min][i]))
					min = j;
			}
			(populateOld ? oldMemberships : newMemberships)[i] = min;
		}

		// Recalculate centroids
		for (int i = 0; i < args.k; i++)
		{
			double acc = 0;
			int count = 0;

			for (int j = 0; j < n; j++)
			{
				if (i == (populateOld ? oldMemberships : newMemberships)[j])
				{
					acc += arr[j];
					++count;
				}
			}

			centroids[i] = (count == 0) ? 0 : acc / count;
		}

		// Output iteration data
		if (args.verbose)
		{
			std::cout << "centroids = ";
			printArr(args.k, centroids);
			std::cout << std::endl;

			std::cout << "diffs =\n";
			for (int i = 0; i < args.k; i++)
			{
				std::cout << "  [" << i << "] " << centroids[i] << " -> ";
				printArr(n, diffs[i]);
				std::cout << std::endl;
			}

			std::cout << "memberships = ";
			printArr(n, populateOld ? oldMemberships : newMemberships);

			std::cout << '\n' << std::endl;
		}

		// Deallocate differences
		for (int i = 0; i < args.k; i++)
			delete[] diffs[i];
		delete[] diffs;
	}
	while (oldMemberships == nullptr || newMemberships ==  nullptr || !arraysEqual(n, oldMemberships, newMemberships));

	return { 0, true, n, newMemberships, centroids };
}

#endif
