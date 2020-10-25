#include "util.h"

#include <string.h>
#include <unistd.h>
#include <dirent.h>

void deleteDir(std::string dirPath)
{
	DIR* dir = opendir(dirPath.c_str());
	if (dir == nullptr) return;

	// Iterate over each item in the directory.
	dirent* entry;
	while ((entry = readdir(dir)))
	{
		// Ignore . and ..
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

		std::string entryPath = std::string(dirPath).append("/").append(entry->d_name);

		// If the item is another directory, recursively delete its contents, then `rmdir` it.
		if (entry->d_type == DT_DIR)
		{
			deleteDir(entryPath);
			rmdir(entryPath.c_str());
		}
		// Otherwise, `unlink` the file.
		else
		{
			unlink(entryPath.c_str());
		}
	}

	// Unlink the directory.
	rmdir(dirPath.c_str());
}

void deleteDir(const char* dirPath)
{
	deleteDir(std::string(dirPath));
}
