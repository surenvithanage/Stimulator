#ifdef CLUSTER_MODE_MPI_SERIAL

#include <fstream>
#include <math.h>
#include <mpich/mpi.h>

#include "kmeans.h"
#include "util.h"

std::ostream& log()
{
	static int mpiRank = -1;
	if (mpiRank == -1)
		MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
	std::cout << mpiRank << ": ";
	return std::cout;
}

KMeansResult kmeans(Args args)
{
	MPI_Init(nullptr, nullptr);

	// Retrieve MPI rank & size
	int mpiRank;
	int mpiSize;
	MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
	MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);

	bool isRoot = (mpiRank == 0);

	// Read values at root node
	double* rootArr = nullptr;
	int n;
	if (isRoot)
	{
		std::vector<double> v;
		{
			std::ifstream f(args.inputFile);
			if (!f.is_open())
			{
				return { -9, isRoot };
			}
			double d;
			while (f >> d)
				v.push_back(d);
		}

		n = v.size();
		rootArr = new double[n];
		std::copy(v.begin(), v.end(), rootArr);

		std::cout << "n = " << n << std::endl;
		std::cout << "k = " << args.k << std::endl;

		if (args.verbose)
		{
			std::cout << "arr = ";
			printArr(n, rootArr);
			std::cout << '\n' << std::endl;
		}
	}

	// Broadcast number of values
	MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

	// Calculate counts & displacements
	int maxElementsPerProcess = std::ceil((float)n / mpiSize);
	int* counts = new int[mpiSize];
	int* displacements = new int[mpiSize];

	counts = new int[mpiSize];
	displacements = new int[mpiSize];
	{
		int r = n;
		for (int i = 0; i < mpiSize; i++)
		{
			if (r > maxElementsPerProcess)
			{
				r -= maxElementsPerProcess;
				counts[i] = maxElementsPerProcess;
			}
			else if (r < 0)
			{
				counts[i] = 0;
			}
			else
			{
				counts[i] = r;
				r = 0;
			}
			displacements[i] = (i == 0) ? 0 : displacements[i - 1] + counts[i - 1];
		}
	}

	// Scatter rootArr across nodes
	double* arr = new double[counts[mpiRank]];

	MPI_Scatterv(
		isRoot ? rootArr : nullptr, counts, displacements, MPI_DOUBLE,
		arr, counts[mpiRank], MPI_DOUBLE,
		0, MPI_COMM_WORLD
	);

	// Calculate initial centroids randomly on the root
	double* centroids = new double[args.k];

	if (isRoot)
	{
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
			centroids[i] = rootArr[(int)centroids[i]];
	}

	MPI_Request doneRequest;
	MPI_Irecv(nullptr, 0, MPI_INT, 0, 0xDEAD, MPI_COMM_WORLD, &doneRequest);

	int* rootNewMemberships = nullptr;
	int* rootOldMemberships = nullptr;

	do
	{
		// Broadcast centroids
		MPI_Request centroidsBcastRequest;
		MPI_Ibcast(centroids, args.k, MPI_DOUBLE, 0, MPI_COMM_WORLD, &centroidsBcastRequest);

		// Await completion of either doneRequest or centroidBcastRequest
		MPI_Request requests[] = { doneRequest, centroidsBcastRequest };
		int requestCompletedIdx = -1;
		MPI_Waitany(2, requests, &requestCompletedIdx, MPI_STATUS_IGNORE);

		if (requestCompletedIdx == 0)
			break;

		// Compute local memberships
		int* memberships = new int[counts[mpiRank]];

		double** diffs = new double*[args.k];
		for (int i = 0; i < args.k; i++)
		{
			diffs[i] = new double[counts[mpiRank]];

			for (int j = 0; j < counts[mpiRank]; j++)
				diffs[i][j] = centroids[i] - arr[j];
		}

		for (int i = 0; i < counts[mpiRank]; i++)
		{
			int min = 0;
			for (int j = 1; j < args.k; j++)
			{
				if (abs(diffs[j][i]) < abs(diffs[min][i]))
					min = j;
			}
			memberships[i] = min;
		}

		// Gather memberships on root
		bool rootPopulateOld = false;
		if (isRoot)
		{
			if (rootOldMemberships == nullptr)
			{
				rootOldMemberships = new int[n];
				rootPopulateOld = true;
			}
			else if (rootNewMemberships == nullptr)
			{
				rootNewMemberships = new int[n];
			}
			else
			{
				for (int i = 0;  i < n; i++)
					rootOldMemberships[i] = rootNewMemberships[i];
			}
		}

		MPI_Gatherv(
			memberships, counts[mpiRank], MPI_INT,
			isRoot ? (rootPopulateOld ? rootOldMemberships : rootNewMemberships) : nullptr, counts, displacements, MPI_INT,
			0, MPI_COMM_WORLD
		);

		for (int i = 0; i < args.k; i++)
			delete[] diffs[i];
		delete[] diffs;
		delete[] memberships;

		// Recompute centroids
		if (isRoot)
		{
			for (int i = 0; i < args.k; i++)
			{
				double acc = 0;
				int count = 0;

				for (int j = 0; j < n; j++)
				{
					if (i == (rootPopulateOld ? rootOldMemberships : rootNewMemberships)[j])
					{
						acc += rootArr[j];
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

				std::cout << "memberships = ";
				printArr(n, rootPopulateOld ? rootOldMemberships : rootNewMemberships);

				std::cout << '\n' << std::endl;
			}
		}
	}
	while (!isRoot || rootOldMemberships == nullptr || rootNewMemberships == nullptr || !arraysEqual(n, rootOldMemberships, rootNewMemberships));

	// Terminate involved processes
	if (isRoot)
	{
		for (int i = 1; i < mpiSize; i++)
			MPI_Send(nullptr, 0, MPI_INT, i, 0xDEAD, MPI_COMM_WORLD);
	}

	// Finalize & return
	MPI_Finalize();
	return { 0, isRoot, n, rootNewMemberships, centroids };
}

#endif
