#ifndef FILELOCK_H
#define FILELOCK_H

#include <string>

class FileLock{
public:
	~FileLock() = default;

	bool AquireFileLock(std::string filename);
	void ReleaseFileLock(std::string filename);
};

#endif