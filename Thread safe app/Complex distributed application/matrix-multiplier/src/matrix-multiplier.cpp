#include <iostream>
#include <chrono>
#include <math.h>
#include <string.h>
#include "mpi.h"

#include "Args.h"
#include "mat/Matrix.h"
#include "mC.h"

namespace chrono = std::chrono;

/**
 * Returns a square matrix initialized with `n` random values in the range [0, 20).
 */
Matrix* get_random_square_matrix(int n)
{
	int* arr = new int[n * n];
	for (int i = 0; i < (n * n); i++)
		arr[i] = rand() % 20;

	Matrix* m = new Matrix(n, arr);
	delete[] arr;
	return m;
}

/**
 * Prints the specified array `arr` of the specified length `n` to `std::cout` in the form `[a b c d]`.
 */
void print_arr(int* arr, int n)
{
	std::cout << "[";
	for (int i = 0; i < n; i++)
	{
		std::cout << arr[i];
		if (i < n - 1)
			std::cout << ' ';
	}
	std::cout << "]";
}

int main(int argc, char** argv)
{
	// Parse CLI args, show usage info, etc.
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
		std::cout << "  " << progName << " -n N [-t T] [-v]\n";
		std::cout << "  " << progName << " -h\n";

		std::cout << "\nArguments:\n";
		std::cout << "  -n N      : Size of the matrix.\n";
		std::cout << "  -t T      : Maximum number of threads. Defaults to & assumed unlimited if zero.\n";
		std::cout << "  -v        : Logs detailed information about the calculation.\n";
		std::cout << "  -m        : Shows the matrices.\n";
		std::cout << "  -h        : Shows this help message.\n";

		std::cout << std::endl;
		return args.showHelp ? 0 : -1;
	}

	if (args.n <= 0)
	{
		std::cerr << "N must be positive." << std::endl;
		return -3;
	}

	if (args.threadLimit < 0)
	{
		std::cerr << "T can't be negative." << std::endl;
		return -4;
	}

	// Seed RNG.
	srand(time(nullptr));

	// Initialize MPI, retrieve rank & size.
	MPI_Init(&argc, &argv);

	int mpiRank, mpiSize;
	MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
	MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);

	bool isRoot = (mpiRank == 0);

	// Create & output matrices A & B.
	Matrix* mA = isRoot ? get_random_square_matrix(args.n) : nullptr;
	Matrix* mB = isRoot ? get_random_square_matrix(args.n) : new Matrix(args.n);

	if (isRoot)
	{
		if (args.showMatrices)
		{
			std::cout << "\nA =\n" << *mA << '\n';
			std::cout << "\nB =\n" << *mB << std::endl;
		}
		else
		{
			std::cout << "A = [...]\n";
			std::cout << "B = [...]" << std::endl;
		}

	}

	auto mCStart = chrono::high_resolution_clock::now();

	// Broadcast matrix B.
	MPI_Bcast(
		mB->arr, args.n * args.n, MPI_INT,
		0, MPI_COMM_WORLD
	);

	// Calculate counts & displacements for scattering & gathering.
	int maxRowsPerProcess = std::ceil((float)args.n / mpiSize);
	int* counts = new int[mpiSize];
	int* displacements = new int[mpiSize];

	{
		int r = args.n;
		for (int i = 0; i < mpiSize; i++)
		{
			if (r > maxRowsPerProcess)
			{
				r -= maxRowsPerProcess;
				counts[i] = maxRowsPerProcess;
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

			counts[i] *= args.n;
			displacements[i] = (i == 0) ? 0 : displacements[i - 1] + counts[i - 1];
		}
	}

	if (isRoot && args.isVerbose)
	{
		std::cout << "\nmaxRowsPerProcess = " << maxRowsPerProcess << '\n';

		std::cout << "counts = ";
		print_arr(counts, mpiSize);
		std::cout << '\n';

		std::cout << "displacements = ";
		print_arr(displacements, mpiSize);
		std::cout << std::endl;
	}

	// Scatter matrix A.
	int* mA_rows = new int[counts[mpiRank]];
	MPI_Scatterv(
		isRoot ? mA->arr : nullptr, counts, displacements, MPI_INT,
		mA_rows, counts[mpiRank], MPI_INT,
		0, MPI_COMM_WORLD
	);

	// Calculate rows of matrix C.
	int* mC_rows = new int[counts[mpiRank]];

	calculate_mC_rows(
		mpiRank, mpiSize, counts, &args,
		mA_rows, mB,
		mC_rows
	);

	// Gather calculations, assemble and output matrix C along with multiplication time.
	Matrix* mC = isRoot ? new Matrix(args.n) : nullptr;

	MPI_Gatherv(
		mC_rows, counts[mpiRank], MPI_INT,
		isRoot ? mC->arr : nullptr, counts, displacements, MPI_INT,
		0, MPI_COMM_WORLD
	);

	if (isRoot)
	{
		auto mCDurationNs = chrono::duration_cast<chrono::nanoseconds>(chrono::high_resolution_clock::now() - mCStart).count();

		if (args.showMatrices)
			std::cout << "\nC =\n" << *mC << "\n\n";
		else
			std::cout << "C = [...]\n";


		std::cout << "Multiplication took " << mCDurationNs << " ns" << " (" << (mCDurationNs / 1e9f) << " s)" << std::endl;
	}

	// Finalize MPI, return.
	MPI_Finalize();
	return 0;
}
