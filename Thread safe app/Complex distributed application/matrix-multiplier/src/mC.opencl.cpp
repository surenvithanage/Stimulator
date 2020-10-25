#ifdef MULTIPLY_MODE_OPENCL

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <linux/limits.h>
#include <unistd.h>
#include <CL/cl.hpp>

#include "mC.h"

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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void calculate_mC_rows(
	int mpiRank, int mpiSize, int* counts, Args* args,
	int* mA_rows, Matrix* mB,
	int* mC_rows
)
{
#pragma GCC diagnostic pop

	cl_int err = CL_SUCCESS;

	// Get OpenCL platform.
	cl::Platform platform = cl::Platform::get(&err);
	if (err == CL_DEVICE_NOT_FOUND || err == CL_PLATFORM_NOT_FOUND_KHR)
	{
		std::cerr << mpiRank << ": No OpenCPL platform " << err << std::endl;
		return;
	}

	// Select first available GPU or CPU device.
	std::vector<cl::Device> gpuDevices;
	platform.getDevices(CL_DEVICE_TYPE_GPU, &gpuDevices);

	std::vector<cl::Device> cpuDevices;
	platform.getDevices(CL_DEVICE_TYPE_CPU, &cpuDevices);

	if (gpuDevices.size() + cpuDevices.size() == 0)
	{
		std::cerr << mpiRank << ": No OpenCL devices" << std::endl;
		return;
	}

	cl::Device device = (gpuDevices.size() > 0 ? gpuDevices : cpuDevices).front();

	// Log selected platform & device.
	if (args->isVerbose > 0)
	{
		std::string platformName;
		platform.getInfo(CL_PLATFORM_NAME, &platformName);
		std::string deviceName;
		device.getInfo(CL_DEVICE_NAME, &deviceName);
		std::cout << mpiRank << ": Using device \"" << deviceName << "\" of OpenCL platform \"" << platformName << '"' << std::endl;
	}

	// Create OpenCL context.
	cl::Context ctx(device, nullptr, nullptr, nullptr, &err);
	if (err != CL_SUCCESS)
	{
		std::cerr << mpiRank << ": Failed to create OpenCL context with error " << err << std::endl;
		return;
	}

	// Create OpenCL command queue.
	cl::CommandQueue q(ctx, device, 0, &err);
	if (err != CL_SUCCESS)
	{
		std::cerr << mpiRank << ": Failed to create OpenCL command queue with error " << err << std::endl;
	}

	// Build OpenCL program.
	std::string clSourcePath = getExecutablePath().append("/mC.opencl.cl");
	std::fstream clSourceFile;
	clSourceFile.open(clSourcePath);
	if (!clSourceFile)
	{
		std::cerr << mpiRank << ": Failed to open the OpenCL source at " << clSourcePath << std::endl;
		return;
	}
	std::ostringstream clSourceStream;
	clSourceStream << clSourceFile.rdbuf();
	std::string clSource = clSourceStream.str();

	cl::Program program(ctx, clSource, true, &err);
	if (err != CL_SUCCESS && mpiRank == 0)
	{
		std::cerr << ": Failed to build OpenCL program with error " << err << std::endl;

		if (err == CL_BUILD_PROGRAM_FAILURE)
		{
			std::string buildLog;
			program.getBuildInfo(device, CL_PROGRAM_BUILD_LOG, &buildLog);
			std::cerr << mpiRank << buildLog << std::endl;
		}
	}

	// Get element multiplication kernel, setup kernel memory.
	cl::Kernel multiplyMatrix(program, "calculateMcElement");

	// n
	cl::Buffer nBuf(ctx, CL_MEM_READ_ONLY | CL_MEM_HOST_NO_ACCESS | CL_MEM_USE_HOST_PTR, sizeof(int), &args->n);
	multiplyMatrix.setArg(0, nBuf);

	// Matrix B.
	cl::Buffer mBBuf(ctx, CL_MEM_READ_ONLY | CL_MEM_HOST_NO_ACCESS | CL_MEM_USE_HOST_PTR, sizeof(int) * args->n * args->n, mB->arr);
	multiplyMatrix.setArg(1, mBBuf);

	// Rows of matrix A.
	cl::Buffer mARowsBuf(ctx, CL_MEM_READ_ONLY | CL_MEM_HOST_NO_ACCESS | CL_MEM_USE_HOST_PTR, sizeof(int) * counts[mpiRank], mA_rows);
	multiplyMatrix.setArg(2, mARowsBuf);

	// (Resultant) rows of matrix C.
	cl::Buffer mCRowsBuf(ctx, CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY, sizeof(int) * counts[mpiRank]);
	multiplyMatrix.setArg(3, mCRowsBuf);

	// Execute kernel per element of matrix C (i.e. row of A * column of B) that must be calculated.
	q.enqueueNDRangeKernel(multiplyMatrix, 0, cl::NDRange(args->n, counts[mpiRank] / args->n));

	// Read results to host memory.
	q.enqueueReadBuffer(mCRowsBuf, CL_BLOCKING, 0, sizeof(int) * counts[mpiRank], mC_rows);

	// Await kernel completion.
	cl::finish();
}

#endif
