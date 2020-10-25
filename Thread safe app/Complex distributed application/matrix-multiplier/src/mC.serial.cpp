#ifdef MULTIPLY_MODE_SERIAL

#include "mC.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void calculate_mC_rows(
	int mpiRank, int mpiSize, int* counts, Args* args,
	int* mA_rows, Matrix* mB,
	int* mC_rows
)
{
#pragma GCC diagnostic pop

	for (int x = 0; x < counts[mpiRank] / args->n; x++)
	{
		for (int y = 0; y < args->n; y++)
		{
			int mC_row_idx = y + (x * args->n);
			int acc = 0;

			for (int i = 0; i < args->n; i++)
			{
				acc += mA_rows[i + (x * args->n)] * mB->get(i, y);
			}

			mC_rows[mC_row_idx] = acc;
		}
	}
}

#endif
