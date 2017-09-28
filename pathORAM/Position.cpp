#include "Position.hpp"
#include "File.hpp"

#include <fstream>

bool PositionHelper::Load(std::string filename, int *position, size_t size)
{
	std::fstream file;
	file.open(filename, std::fstream::in | std::ios::binary);	
	
	if (!file.good()) {
		return false;
	}
	
	// Is the file the correct length?
	size_t length = File::GetLength(file);

	if (length != size*sizeof (int)) {
		file.close();
		return false;
	}
	
	// Read in the file data
	file.read((char *) position, size*sizeof (int));
	
	file.close();
	return true;
}

void PositionHelper::Save(std::string filename, int *position, size_t size)
{
	std::fstream file;
	file.open(filename, std::ios::out | std::ios::trunc | std::ios::binary);
	
	// Dump array to file
	file.write((char *) position, size*sizeof (int));
	
	file.close();
}
