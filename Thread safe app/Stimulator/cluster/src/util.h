#ifndef UTIL_H
#define UTIL_H

/**
 * Returns whether the arrays `l` and `r`, each of length `n` are identical.
 */
template<typename T>
bool arraysEqual(int n, T* l, T* r)
{
	for (int i = 0; i < n; i++)
	{
		if (l[i] != r[i])
			return false;
	}
	return true;
}

/**
 * Prints the array `arr` of `n` elements to `std::cout` with `start` and `end` on either side, and each element
 * delimited by `separator`.
 */
template<typename T>
void printArr(int n, T* arr, const char* separator = ", ", const char* start = "[", const char* end = "]")
{
	if (start != nullptr)
		std::cout << start;

	for (int i = 0; i < n; i++)
	{
		if (i != 0)
			std::cout << separator;

		std::cout << arr[i];
	}

	if (end != nullptr)
		std::cout << end;
}

#endif
