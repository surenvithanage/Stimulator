#ifndef KMEANS_H
#define KMEANS_H

#include <iostream>
#include <vector>

#include "Args.h"

/**
 * Result of executing `kmeans`.
 */
struct KMeansResult
{
	/**
	 * Return code; if non-zero, the cluster program terminates or writes an error.
	 */
	int returnCode = 0;

	/**
	 * Whether this process is a root process (in the context of distribution). If false, the cluster program doesn't
	 * write results to stdout. This prevents results being written out by multiple nodes.
	 */
	bool isRoot = true;

	/**
	 * Number of values clustered.
	 */
	int n = -1;

	/**
	 * Array of memberships of each value.
	 */
	int* memberships = nullptr;

	/**
	 * Array of centroid values.
	 */
	double* centroids = nullptr;
};

/**
 * Executes the k-means clustering algorithm for `args`, on the values of `arr`, populating `newMemberships` and
 * `centroids` (both expected to be initialized) in the process.
 *
 * If distributing with MPI, the implementation is expected to init & finalize MPI processes.
 */
KMeansResult kmeans(Args args);

#endif
