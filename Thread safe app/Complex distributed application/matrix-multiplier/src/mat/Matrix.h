#ifndef MATRIX_H
#define MATRIX_H

#include <initializer_list>
#include <ostream>

/**
 * Represents a matrix of integers.
 */
class Matrix
{
public:

	/**
	 * Underlying array that stores the elements of the matrix.
	 */
	int* arr;

	/**
	 * Number of rows.
	 */
	int rows;

	/**
	 * Number of columns.
	 */
	int cols;

	/**
	 * Creates a matrix of the specified number of rows & columns, optionally seeding it with values of the specified
	 * array.
	 */
	Matrix(int _rows, int _cols, int* _arr = nullptr);

	/**
	 * Creates a square matrix of the specified size, optionally seeding it with values of the specified array.
	 */
	Matrix(int size, int* _arr = nullptr);

	/**
	 * Creates a matrix of the specified number of rows & columns, seeding it with the specified values.
	 */
	Matrix(int _rows, int _cols, std::initializer_list<int> initializer);

	/**
	 * Creates a square matrix of the specified size, seeding it with the specified values.
	 */
	Matrix(int size, std::initializer_list<int> initializer);

	/**
	 * Releases resources acquired by the matrix.
	 */
	~Matrix();

	/**
	 * Gets the value at the specified row & column of the matrix.
	 */
	int get(int r, int c);

	/**
	 * Sets the value at the specified row & column of the matrix.
	 */
	void set(int r, int c, int value);

	/**
	 * Seeds the matrix with the specified array.
	 */
	void seed(int* _arr);

private:

	/**
	 * Prints the matrix in TSV format to the specified output stream.
	 */
	friend std::ostream& operator<<(std::ostream& stream, Matrix& mat);
};

#endif
