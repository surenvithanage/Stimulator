/**
 * Returns the absolute value of the specified `double`.
 */
double absd(double d)
{
	return (d < 0) ? -d : d;
}

/**
 * Computes the index of the element within `centroids` (of length `_k`) to which the element at `global_id(0)` of `arr`
 * is closest, into `memberships`.
 */
kernel void computeLocalMemberships(
	global int* _k,
	global double* arr,
	global double* centroids,
	global int* memberships
) {
	int n = get_global_id(0);
	int k = *_k;

	int minIdx = 0;
	double minDiff = absd(centroids[0] - arr[n]);

	for (int i = 0; i < k; i++)
	{
		if (absd(centroids[i] - arr[n]) < minDiff)
			minIdx = i;
	}

	memberships[n] = minIdx;
}

/**
 * Recomputes the element at `global_id(0)` of `centroids`, according to the `memberships` of elements of `arr` (both of
 * length `_n`).
 */
kernel void recomputeCentroids(
	global int* _n,
	global double* arr,
	global double* centroids,
	global int* memberships
) {
	int k = get_global_id(0);
	int n = *_n;

	double acc = 0;
	int count = 0;

	for (int i = 0; i < n; i++)
	{
		if (k == memberships[i])
		{
			acc += arr[i];
			++count;
		}
	}

	centroids[k] = (count == 0) ? 0 : (acc / count);
}
