#ifndef CSWCaosCLIENT_MAP_H
#define CSWCaosCLIENT_MAP_H

#include <unordered_map>
#include "CaosWClient.hpp"

using BlockMap = std::unordered_map<int64_t, std::vector<size_t>>;
using VersionMap = std::unordered_map<int64_t, time_t>;

class DataMap {
	int blockCount;
	BlockMap positions;
	VersionMap latestLocalVersions;
	BlockMap vfpositions;

public:
	DataMap(size_t blockCount);
	~DataMap();

	std::vector<size_t> Positions(int64_t bid);
	void AddPosition(int64_t bid, size_t pos);
	void RemovePosition(int64_t bid, size_t pos);
	void SetPositions(int64_t bid, std::vector<size_t> poss);

	std::vector<size_t> VFPositions(int64_t bid);
	void AddVFPosition(int64_t bid, size_t pos);
	void RemoveVFPosition(int64_t bid, size_t pos);
	void SetVFPositions(int64_t bid, std::vector<size_t> poss);

	time_t LatestLocalVersion(int64_t bid);
	void SetLocalVersion(int64_t bid, time_t time);

	size_t GetFreePosition();
	size_t GetRandBlockPosition(int64_t bid);
	size_t GetRandPosition(int64_t bid);

	int64_t GetMapSize();

	void SaveMap();
};

#endif //CSWCaosCLIENT_MAP_H
