#ifdef CLUSTER_MODE_MPI_OPENCL

#include <fstream>
#include <sstream>
#include <math.h>
#include <linux/limits.h>
#include <unistd.h>
#include <mpich/mpi.h>
#include <CL/cl.hpp>

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

/**
 * Retrieves the directory name of the current _Linux_ executable.
 * Based off https://stackoverflow.com/a/5525712/2466716.
 */
std::string getExecutablePath()
{
	char buf[PATH_MAX];
	ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
	if (len != -1) buf[len] = '\0';
	std::string str(buf);
	return str.substr(0, str.rfind('/'));
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

	// Create OpenCL program
	cl_int err = CL_SUCCESS;

	// Get OpenCL platform.
	cl::Platform platform = cl::Platform::get(&err);
	if (err == CL_DEVICE_NOT_FOUND || err == CL_PLATFORM_NOT_FOUND_KHR)
	{
		log() << "No OpenCPL platform " << err << std::endl;
		return { -3, isRoot };
	}

	// Select first available GPU or CPU device.
	std::vector<cl::Device> gpuDevices;
	platform.getDevices(CL_DEVICE_TYPE_GPU, &gpuDevices);

	std::vector<cl::Device> cpuDevices;
	platform.getDevices(CL_DEVICE_TYPE_CPU, &cpuDevices);

	if (gpuDevices.size() + cpuDevices.size() == 0)
	{
		log() << "No OpenCL devices" << std::endl;
		return { -4, isRoot };
	}

	cl::Device device = (gpuDevices.size() > 0 ? gpuDevices : cpuDevices).front();

	// Log selected platform & device.
	if (args.verbose)
	{
		std::string platformName;
		platform.getInfo(CL_PLATFORM_NAME, &platformName);
		std::string deviceName;
		device.getInfo(CL_DEVICE_NAME, &deviceName);
		log() << "Using device \"" << deviceName << "\" of OpenCL platform \"" << platformName << '"' << std::endl;
	}

	// Create OpenCL context.
	cl::Context ctx(device, nullptr, nullptr, nullptr, &err);
	if (err != CL_SUCCESS)
	{
		log() << "Failed to create OpenCL context with error " << err << std::endl;
		return { -5, isRoot };
	}

	// Create OpenCL command queue.
	cl::CommandQueue q(ctx, device, 0, &err);
	if (err != CL_SUCCESS)
	{
		log() << "Failed to create OpenCL command queue with error " << err << std::endl;
	}

	// Build OpenCL program.
	std::string clSourcePath = getExecutablePath().append("/kmeans.mpi.opencl.cl");
	std::fstream clSourceFile;
	clSourceFile.open(clSourcePath);
	if (!clSourceFile)
	{
		log() << "Failed to open the OpenCL source at " << clSourcePath << std::endl;
		return { -8, isRoot };
	}
	std::ostringstream clSourceStream;
	clSourceStream << clSourceFile.rdbuf();
	std::string clSource = clSourceStream.str();

	cl::Program program(ctx, clSource, true, &err);
	if (err != CL_SUCCESS && isRoot)
	{
		std::cerr << ": Failed to build OpenCL program with error " << err << std::endl;

		if (err == CL_BUILD_PROGRAM_FAILURE)
		{
			std::string buildLog;
			program.getBuildInfo(device, CL_PROGRAM_BUILD_LOG, &buildLog);
			std::cout << buildLog << std::endl;
		}
	}

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

	// Retrieve OpenCL kernels, create buffers & set args that are loop invariant.

	cl::Kernel computeLocalMemberships(program, "computeLocalMemberships");

	cl::Buffer kBuf(ctx, CL_MEM_READ_ONLY | CL_MEM_HOST_READ_ONLY | CL_MEM_USE_HOST_PTR, sizeof(int), &args.k);
	computeLocalMemberships.setArg(0, kBuf);

	cl::Buffer arrBuf(ctx, CL_MEM_READ_ONLY | CL_MEM_HOST_READ_ONLY | CL_MEM_USE_HOST_PTR, sizeof(double) * counts[mpiRank], arr);
	computeLocalMemberships.setArg(1, arrBuf);

	cl::Kernel recomputeCentroids(program, "recomputeCentroids");

	cl::Buffer nBuf(ctx, CL_MEM_READ_ONLY | CL_MEM_HOST_READ_ONLY | CL_MEM_USE_HOST_PTR, sizeof(int), &n);
	recomputeCentroids.setArg(0, nBuf);

	cl::Buffer rootArrBuf(ctx, CL_MEM_READ_ONLY | CL_MEM_HOST_READ_ONLY | CL_MEM_USE_HOST_PTR, sizeof(double) * n, rootArr);
	recomputeCentroids.setArg(1, rootArrBuf);

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
		cl::Buffer centroidsBuf(ctx, CL_MEM_READ_ONLY | CL_MEM_HOST_READ_ONLY | CL_MEM_USE_HOST_PTR, sizeof(double) * args.k, centroids);
		computeLocalMemberships.setArg(2, centroidsBuf);

		cl::Buffer membershipsBuf(ctx, CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY, sizeof(int) * counts[mpiRank]);
		computeLocalMemberships.setArg(3, membershipsBuf);

		int* memberships = new int[counts[mpiRank]];
		q.enqueueNDRangeKernel(computeLocalMemberships, cl::NDRange(0), cl::NDRange(counts[mpiRank]));
		q.enqueueReadBuffer(membershipsBuf, CL_BLOCKING, 0, sizeof(int) * counts[mpiRank], memberships);
		cl::finish();

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

		delete[] memberships;

		// Recompute centroids
		if (isRoot)
		{
			cl::Buffer rootCentroidsBuf(ctx, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, sizeof(double) * args.k, centroids);
			recomputeCentroids.setArg(2, rootCentroidsBuf);

			cl::Buffer rootMembershipsBuf(ctx, CL_MEM_READ_ONLY | CL_MEM_HOST_READ_ONLY | CL_MEM_USE_HOST_PTR, sizeof(int) * n, rootPopulateOld ? rootOldMemberships : rootNewMemberships);
			recomputeCentroids.setArg(3, rootMembershipsBuf);

			q.enqueueNDRangeKernel(recomputeCentroids, cl::NDRange(0), cl::NDRange(args.k));
			q.enqueueReadBuffer(rootCentroidsBuf, CL_BLOCKING, 0, args.k, centroids);
			cl::finish();

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
