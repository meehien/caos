#include "File.hpp"

void File::Read(std::fstream &file, byte_t *data, size_t len)
{
	file.read((char *) data, len);
}

void File::Write(std::fstream &file, byte_t *data, size_t len)
{
	file.write((char *) data, len);
}

size_t File::GetLength(std::fstream &file)
{
	file.seekg(0, file.end);
	size_t len = file.tellg();
	file.seekg(0, file.beg);
	
	return len;
}
