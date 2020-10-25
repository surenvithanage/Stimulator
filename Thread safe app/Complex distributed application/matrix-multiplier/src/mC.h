#ifndef MC_H
#define MC_H

#include "Args.h"
#include "mat/Matrix.h"

/**
 * Calculates rows of matrix C.
 *
 * Gets executed in the context of an MPI process.
 * For example, to multiply two 3x3 matrices (n = 3; C = A x B) with 2 MPI nodes,
 * Consider the operation of the first MPI process P, which computes the first 2 rows of A.
 *
 *     A = [a b c]    B = j k l    C = [_ _ _]
 *         [d e f]        m n o        [_ _ _]
 *          g h i         p q r         _ _ _
 *
 * P invokes calculate_mC_rows with,
 *     mpiRank = 0
 *     mpiSize = 2
 *     counts  = [6 3]
 *     mB_rows = [j k l m n o p q r] (matrix B)
 *     mA_rows = [a b c d e f]       (first 2 rows of matrix A)
 *     mC_rows = [_ _ _ _ _ _]       (placeholders expected to be populated by calculate_mC_rows)
 */
void calculate_mC_rows(
	int mpiRank, int mpiSize, int* counts, Args* args,
	int* mA_rows, Matrix* mB,
	int* mC_rows
);

#endif
