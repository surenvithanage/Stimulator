#include "Matrix.h"

Matrix::Matrix(int _rows, int _cols, int* _arr): rows(_rows), cols(_cols)
{
	arr = new int[_rows * _cols];
	seed(_arr);
}

Matrix::Matrix(int size, int* _arr): rows(size), cols(size)
{
	arr = new int[size * size];
	seed(_arr);
}

Matrix::Matrix(int _rows, int _cols, std::initializer_list<int> initializer): rows(_rows), cols(_cols)
{
	arr = new int[_rows * _cols];
	seed((int*)initializer.begin());
}

Matrix::Matrix(int size, std::initializer_list<int> initializer): rows(size), cols(size)
{
	arr = new int[size * size];
	seed((int*)initializer.begin());
}

Matrix::~Matrix()
{
	delete[] arr;
}

int Matrix::get(int r, int c)
{
	int i = (r * cols) + c;
	return arr[i];
}

void Matrix::set(int r, int c, int value)
{
	int i = (r * cols) + c;
	arr[i] = value;
}

void Matrix::seed(int* _arr)
{
	for (int i = 0; i < (rows * cols); i++)
		arr[i] = (_arr == nullptr) ? 0 : _arr[i];
}

std::ostream& operator<<(std::ostream& stream, Matrix& mat)
{
	for (int i = 0; i < (mat.rows * mat.cols); i++)
	{
		if (i % mat.cols == 0)
		{
			if (i > 0)
				stream << '\n';
		}
		else
		{
			stream << '\t';
		}

		stream << mat.arr[i];
	}
	return stream;
}
