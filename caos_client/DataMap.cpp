#include <fstream>
#include <cstring>
#include <algorithm>
#include "DataMap.hpp"
#include "File.hpp"
#include "Log.hpp"

DataMap::DataMap(size_t blockCount):
blockCount(static_cast<int>(blockCount)){
	// file format: [lengths: positions, llv, vfpositions]
	//              [maps: [positions: bid, posvectlength, posvect]
	//                     [llv: bid, time]
	//                     [vfpositions: bid, vfposvectlength, vfposvect]]

	std::fstream file;

	file.open("csw.map", std::ios::in | std::ios::binary);

	if (!file.good()) {
		std::vector<size_t> initPos(0);
		for(int i = 0; i < blockCount+1; i++){
				initPos.push_back(i-1);
				positions.insert({i-1, {}});
		}
		positions[-1] = initPos;

		for(int i = 0; i < blockCount+1; i++){
				latestLocalVersions.insert({i-1, 0});
		}

		for(int i = 0; i < blockCount+1; i++){
				vfpositions.insert({i-1, {}});
		}
	}
	else{

		size_t length = File::GetLength(file);
		size_t num = length/sizeof(byte_t);
		std::vector<byte_t> mapBytes(num);

		file.read((char *) mapBytes.data(), num*sizeof(byte_t));
		file.close();

		int64_t posSize, llvSize, vfposSize;
		int64_t offset = 0;

		memcpy(&posSize, mapBytes.data()+offset, sizeof(int64_t));
		offset += sizeof(int64_t);
		memcpy(&llvSize, mapBytes.data()+offset, sizeof(int64_t));
		offset += sizeof(int64_t);
		memcpy(&vfposSize, mapBytes.data()+offset, sizeof(int64_t));
		offset += sizeof(int64_t);

		while(posSize > 0){
				int64_t tempBid;
				memcpy(&tempBid, mapBytes.data()+offset, sizeof(int64_t));
				offset += sizeof(int64_t);
				posSize -= sizeof(int64_t);

				int64_t vectLen;
				memcpy(&vectLen, mapBytes.data()+offset, sizeof(int64_t));
				offset += sizeof(int64_t);
				posSize -= sizeof(int64_t);

				std::vector<size_t> vect(vectLen);
				memcpy(vect.data(), mapBytes.data()+offset, sizeof(size_t)*vectLen);
				offset += sizeof(size_t)*vectLen;
				posSize -= sizeof(size_t)*vectLen;

				positions.insert({{tempBid,vect}});
		}

		while(llvSize > 0){
				int64_t tempBid;
				memcpy(&tempBid, mapBytes.data()+offset, sizeof(int64_t));
				offset += sizeof(int64_t);
				llvSize -= sizeof(int64_t);

				time_t time;
				memcpy(&time, mapBytes.data()+offset, sizeof(time_t));
				offset += sizeof(time_t);
				llvSize -= sizeof(time_t);

				latestLocalVersions.insert({{tempBid, time}});
		}

		while(vfposSize > 0){
				int64_t tempBid;
				memcpy(&tempBid, mapBytes.data()+offset, sizeof(int64_t));
				offset += sizeof(int64_t);
				vfposSize -= sizeof(int64_t);

				int64_t vectLen;
				memcpy(&vectLen, mapBytes.data()+offset, sizeof(int64_t));
				offset += sizeof(int64_t);
				vfposSize -= sizeof(int64_t);

				std::vector<size_t> vect(vectLen);
				memcpy(vect.data(), mapBytes.data()+offset, sizeof(size_t)*vectLen);
				offset += sizeof(size_t)*vectLen;
				vfposSize -= sizeof(size_t)*vectLen;

				vfpositions.insert({{tempBid,vect}});
		}
	}
}

DataMap::~DataMap(){}

std::vector<size_t> DataMap::Positions(int64_t bid){
	if(positions.find(bid) == positions.end())
		return {};
	else
		return positions.find(bid)->second;
}

void DataMap::AddPosition(int64_t bid, size_t pos){
	Log::Write(Log::DEBUG, "Attempting to add pos %i to bid %i", pos, bid);
	if(positions.find(bid) != positions.end()){
		if(std::find(positions.find(bid)->second.begin(), positions.find(bid)->second.end(), pos) == positions.find(bid)->second.end())
		{
				Log::Write(Log::DEBUG, "successfully added");
				positions.find(bid)->second.push_back(pos);
				RemovePosition(-1, pos);
		}
	}
	else{
		Log::Write(Log::DEBUG, "successfully added complete new entry");
		positions.insert({bid, {pos}});
		RemovePosition(-1, pos);
	}

}

void DataMap::RemovePosition(int64_t bid, size_t pos){
	if(positions.find(bid) != positions.end()){
		positions.find(bid)->second.erase(std::remove(positions.find(bid)->second.begin(),
																	positions.find(bid)->second.end(), pos),
													positions.find(bid)->second.end());
		if(bid != -1)
				positions.find(-1)->second.push_back(pos);
	}

}

void DataMap::SetPositions(int64_t bid, std::vector<size_t> poss){

	for(int i = 0; i < (int)positions.find(bid)->second.size(); i++)
		AddPosition(-1, positions.find(bid)->second.at(i));

	if(positions.find(bid) != positions.end())
		positions.find(bid)->second = poss;
	else
		positions.insert({bid, poss});

	for(int i = 0; i < (int)poss.size(); i++)
		RemovePosition(-1, poss.at(i));
}

std::vector<size_t> DataMap::VFPositions(int64_t bid){
	if(vfpositions.find(bid) == vfpositions.end())
		return {};
	else
		return vfpositions.find(bid)->second;
}

void DataMap::AddVFPosition(int64_t bid, size_t pos){
	Log::Write(Log::DEBUG, "Add VF of bid %i pos %i to size %i", bid, pos, (int)VFPositions(bid).size());
	if(bid < blockCount){
		if(vfpositions.find(bid) == vfpositions.end())
				vfpositions[bid] = {pos};
		else if(std::find(vfpositions.find(bid)->second.begin(), vfpositions.find(bid)->second.end(), pos) == vfpositions.find(bid)->second.end())
				vfpositions.find(bid)->second.push_back(pos);
	}
}

void DataMap::RemoveVFPosition(int64_t bid, size_t pos){
	Log::Write(Log::DEBUG, "REM VF of bid %i pos %i to size %i", bid, pos, (int)VFPositions(bid).size());
	if(vfpositions.find(bid) != vfpositions.end()){
		vfpositions.find(bid)->second.erase(std::remove(vfpositions.find(bid)->second.begin(),
																		vfpositions.find(bid)->second.end(), pos),
														vfpositions.find(bid)->second.end());
	}

}

void DataMap::SetVFPositions(int64_t bid, std::vector<size_t> poss){
	Log::Write(Log::DEBUG, "SET VF of %i to size %i", bid, (int)poss.size());
	for(int i = 0; i < poss.size(); i++)
		Log::Write(Log::DEBUG, "vf pos: %i", poss.at(i));
	if(bid < blockCount){
		if(vfpositions.find(bid) == vfpositions.end())
				vfpositions[bid] = poss;
		else
				vfpositions.find(bid)->second = poss;
	}
}

time_t DataMap::LatestLocalVersion(int64_t bid){
	if(latestLocalVersions.find(bid) == latestLocalVersions.end())
		return 0;
	else
		return latestLocalVersions.find(bid)->second;
}

void DataMap::SetLocalVersion(int64_t bid, time_t time){
	if(bid < blockCount){
		if(latestLocalVersions.find(bid) == latestLocalVersions.end())
				latestLocalVersions[bid] = time;
		else
				latestLocalVersions.find(bid)->second = time;
	}
}

size_t DataMap::GetFreePosition(){
	for (;;) {
		size_t randPos = AES::GetRandom(blockCount);
		int64_t bid;

		int ibc = static_cast<int>(blockCount);
		for(int i=0; i < ibc; i++){
				std::vector<size_t> blockPositions = Positions(i-1);
				if(std::find(blockPositions.begin(), blockPositions.end(),randPos)!=blockPositions.end()) {
					bid = i-1;
					break;
				}
		}

		if (bid == -1) {
				return randPos;
		}

		std::vector<size_t> poss = Positions(bid);

		// If the block is duplicated
		if (poss.size() > 1) {
				// Remove the position from maps
				RemovePosition(bid, randPos);
				std::remove(poss.begin(), poss.end(), randPos);
				return randPos;
		}
	}
}

int64_t DataMap::GetMapSize(){
	int64_t size = 0;
	int ibc = static_cast<int>(blockCount);
	for(int i=0; i < ibc; i++){
		if(0 < (int)Positions(i).size())
				size++;
	}
	return size;
}

size_t DataMap::GetRandBlockPosition(int64_t bid){
	std::vector<size_t> blockPositions = Positions(bid);
	if (blockPositions.size() == 0){
		return GetFreePosition();
	}

	size_t randPos = AES::GetRandom(blockPositions.size());
	size_t pos = blockPositions[randPos];
	Log::Write(Log::DEBUG,"access block %zu (had %zu positions) from pos %zu ",bid,blockPositions.size(),pos);
	return pos;
}

size_t DataMap::GetRandPosition(int64_t bid){
	std::vector<size_t> blockPositions = Positions(bid);
	size_t randPos = AES::GetRandom(blockPositions.size());
	size_t pos = blockPositions[randPos];
	return pos;
}

void DataMap::SaveMap(){
// file format: [lengths: positions, llv, vfpositions]
//              [maps: [positions: bid, posvectlength, posvect]
//                     [llv: bid, time]
//                     [vfpositions: bid, vfposvectlength, vfposvect]]


	int64_t posSize, llvSize, vfposSize;

	posSize = 0;
	for(auto block : positions){
		posSize += 2*sizeof(int64_t);
		posSize += sizeof(size_t)*block.second.size();
	}

	llvSize = latestLocalVersions.size()*(sizeof(int64_t)+sizeof(time_t));

	vfposSize = 0;
	for(auto block : vfpositions){
		vfposSize += 2*sizeof(int64_t);
		vfposSize += sizeof(size_t)*block.second.size();
	}

	std::vector<byte_t> mapBytes(sizeof(int64_t)*3 + posSize + llvSize + vfposSize);
	int64_t offset = 0;

	memcpy(mapBytes.data()+offset, &posSize, sizeof(int64_t));
	offset += sizeof(int64_t);
	memcpy(mapBytes.data()+offset, &llvSize, sizeof(int64_t));
	offset += sizeof(int64_t);
	memcpy(mapBytes.data()+offset, &vfposSize, sizeof(int64_t));
	offset += sizeof(int64_t);

	for(auto block : positions){
		memcpy(mapBytes.data()+offset, &block.first, sizeof(int64_t));
		offset += sizeof(int64_t);
		auto size = block.second.size();
		memcpy(mapBytes.data()+offset, &size, sizeof(int64_t));
		offset += sizeof(int64_t);
		memcpy(mapBytes.data()+offset, block.second.data(), sizeof(size_t)*block.second.size());
		offset += sizeof(size_t)*block.second.size();
	}

	for(auto block : latestLocalVersions){
		memcpy(mapBytes.data()+offset, &block.first, sizeof(int64_t));
		offset += sizeof(int64_t);
		memcpy(mapBytes.data()+offset, &block.second, sizeof(time_t));
		offset += sizeof(time_t);
	}

	for(auto block : vfpositions){
		memcpy(mapBytes.data()+offset, &block.first, sizeof(int64_t));
		offset += sizeof(int64_t);
		auto size = block.second.size();
		memcpy(mapBytes.data()+offset, &size, sizeof(int64_t));
		offset += sizeof(int64_t);
		memcpy(mapBytes.data()+offset, block.second.data(), sizeof(size_t)*block.second.size());
		offset += sizeof(size_t)*block.second.size();
	}

	std::fstream file;

	file.open("csw.map", std::ios::out | std::ios::trunc | std::ios::binary);

	file.write((char *) mapBytes.data(), mapBytes.size()*sizeof(byte_t));
	file.close();

}
