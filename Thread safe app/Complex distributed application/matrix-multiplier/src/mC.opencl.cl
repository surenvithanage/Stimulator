/**
 * Calculates the element at a single row and column of C.
 *
 * This kernel gets executed in the context of an MPI process (see matrix-multiplier.cpp).
 * Continuing the example of the calculate_mC_rows function (see mC.h),
 *
 * P starts a 2x3 NDRangeKernel to calculate elements of C that correspond to a..f.
 * To calculate f,
 *
 *            (c = 2)
 *     A =       v
 *         a  b  c
 *         d  e [f] < (r = 1)
 *         g  h  i
 *
 *     mACoffset = d
 */
kernel void calculateMcElement(
	global int* _n,
	global int* mB,
	global int* mA_rows,
	global int* mC_rows
) {
	int n = *_n;
	int c = get_global_id(0);
	int r = get_global_id(1);

	int mACoffset = (r * n);

	mC_rows[mACoffset + c] = 0;

	for (int i = 0; i < n; i++)
		mC_rows[mACoffset + c] += mA_rows[mACoffset + i] * mB[c + (n * i)];
}
