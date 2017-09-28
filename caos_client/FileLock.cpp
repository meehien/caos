#include "FileLock.hpp"
#include "Log.hpp"

#include <iosfwd>
#include <string>
#include <iostream>
#include <fstream>

bool FileLock::AquireFileLock(std::string filename){
	std::ifstream infile(filename+".lock");
	if(!infile.good()){
		std::fstream file;
		file.open(filename+".lock", std::ios::out | std::ios::trunc | std::ios::binary);

		file.write("", 0);
		file.close();
		return true;
	}
	return false;
}

void FileLock::ReleaseFileLock(std::string filename){
	if(std::remove((filename+".lock").c_str()) != 0)
		Log::Write(Log::WARNING, "Unable to release lock on %s", filename.c_str());
}