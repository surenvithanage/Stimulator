#include <iostream>
#include <fstream>
#include <ctime>
#include <stdio.h>
#include <mpich/mpi.h>
#define MAX 825

using namespace std;


MPI_Status status;

double A[MAX][MAX];
double B[MAX][MAX];
double C[MAX][MAX];
int thread_number = omp_get_max_threads();

main(int argc, char **argv)
{
	//Initializing values
	int numtasks;
	int taskid;
	int numworkers;
	int source;
	int dest;
	int rows;
	int offset;
	int N;

	//Initializing MPI
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &taskid);
	MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

	numworkers = numtasks - 1;

	/* MASTER PROGRAM*/
	if (taskid == 0)
	{
		cout << "Please Enter the value of the square Matrices " << endl;
		cin >> N;

		//Randomly Generating Numbers for the Matrices
		for (int i = 0; i < N; i++)
		{
			for (int j = 0; j < N; j++)
			{
				A[i][j] = rand() % 15;
			}
		}

		for (int i = 0; i < N; i++)
		{
			for (int j = 0; j < N; j++)
			{
				B[i][j] = rand() % 15;
			}
		}

		for (int i = 0; i < N; i++)
		{
			for (int j = 0; j < N; j++)
			{
				C[i][j] = 0;
			}
		}

		//Doing the Multiplication
		clock_t ProcessTime;
		ProcessTime = clock();

		/* Semd matrix data to the worker tasks*/
		rows = N / numworkers;
		offset = 0;

		for (dest = 1; dest <= numworkers; dest++)
		{
			MPI_Send(&offset, 1, MPI_INT, dest, 1, MPI_COMM_WORLD);
			MPI_Send(&rows, 1, MPI_INT, dest, 1, MPI_COMM_WORLD);
			MPI_Send(&A[offset][0], rows*N, MPI_DOUBLE, dest, 1, MPI_COMM_WORLD);
			MPI_Send(&B, N*N, MPI_DOUBLE, dest, 1, MPI_COMM_WORLD);
			offset = offset + rows;
		}

		/* Wait for results from all worker tasks*/
		for (int i = 1; i <= numworkers; i++)
		{
			source = i;
			MPI_Recv(&offset, 1, MPI_INT, source, 2, MPI_COMM_WORLD, &status);
			MPI_Recv(&rows, 1, MPI_INT, source, 2, MPI_COMM_WORLD, &status);
			MPI_Recv(&C[offset][0], rows*N, MPI_DOUBLE, source, 2, MPI_COMM_WORLD, &status);
		}

		float elapsedTime = (float)ProcessTime / CLOCKS_PER_SEC;
		cout << "Elapsed Time for Process:" << elapsedTime << " Seconds";


		//Saving To File
		ofstream myFile("Matrice Multiplication.txt");
		if (myFile.is_open())
		{

			myFile << "Elapsed Time for Process:" << elapsedTime << "Seconds";
			myFile.close();
		}
		else cout << "Unable to Open File";
		int x;
		cin >> x;

	}

	/* WORKER PROGRAM*/
	if (taskid > 0)
	{
		source = 0;
		MPI_Recv(&offset, 1, MPI_INT, source, 1, MPI_COMM_WORLD, &status);
		MPI_Recv(&rows, 1, MPI_INT, source, 1, MPI_COMM_WORLD, &status);
		MPI_Recv(&A, rows*N, MPI_DOUBLE, source, 1, MPI_COMM_WORLD, &status);
		MPI_Recv(&B, N*N, MPI_DOUBLE, source, 1, MPI_COMM_WORLD, &status);

	#pragma omp parrallel shared (A,B,C,N)
		{
			#pragma omp for
			{
				for (int i = 0; i < rows; i++)
				{
					for (int j = 0; j < N; j++)
					{
						for (int k = 0; k < N; k++)
						{
							C[i][j] += A[i][k] * B[k][j];
						}
					}

				}
			}
		}

		MPI_Send(&offset, 1, MPI_INT, 0, 2, MPI_COMM_WORLD);
		MPI_Send(&rows, 1, MPI_INT, 0, 2, MPI_COMM_WORLD);
		MPI_Send(&C, rows*N, MPI_DOUBLE, 0, 2, MPI_COMM_WORLD);
	}
	MPI_Finalize();

}