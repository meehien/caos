#include "MapCheck.hpp"
#include "FileStore.hpp"
#include "Crypto.hpp"
#include "TestStore.hpp"
#include <algorithm>

MapCheck::MapCheck(BlockStore *store, std::string fileName, size_t blockCount, size_t blockSize, bytes<Key> key):
store(store), fileName(fileName), blockCount(blockCount), blockSize(blockSize), key(key){}

void MapCheck::Check() {

	Log::Write(Log::INFO, "Loading map...");
	DataMap *map = new DataMap(blockCount, fileName);
	DataMap *unmapped = new DataMap(blockCount, "");
	knownPoss = new std::vector<size_t>();
	std::vector<int64_t> *bids = new std::vector<int64_t>();
	std::vector<int64_t> *newBids = new std::vector<int64_t>();
	changed = 0;

	int64_t bidCount = map->GetMapSize();
	Log::Write(Log::INFO, "%i blocks found", bidCount);

	Log::Write(Log::INFO, "Checking positions...\n");
	for(int i = 0; i <= (int)blockCount; i++){
		if((int)map->Positions(i).size() > 0)
				bids->push_back(i);
	}

	Log::Write(Log::INFO,"======== Beginning map validity test ========");
	double mapValidity = 0;
	for(int i = 0; i < bidCount; i++)
		mapValidity += Validity(bids->at(i), map);

	Log::Write(Log::INFO,"======== Beginning map knowledge test ========");

	for(int i = 0; i <= (int)blockCount; i++){
		if(std::find(knownPoss->begin(), knownPoss->end(),i)==knownPoss->end()){
				Block b = Block(AES::Decrypt(key, store->Read(i), blockSize));
				if(b.bid != -1){
					unmapped->AddPosition(b.bid, i);
					if(std::find(newBids->begin(), newBids->end(),b.bid)==newBids->end())
						newBids->push_back(b.bid);
				}
		}
	}

	int found = 0;
	std::sort(newBids->begin(), newBids->end());
	for(int i = 0; i < (int)newBids->size(); i++)
		found += ListUnmapped(newBids->at(i), unmapped);

	if((int)newBids->size() == 0)
		Log::Write(Log::INFO,"No unknown positions found\n");

	/* very slow so disabled
	Log::Write(Log::INFO,"=============== Hit rate test ===============");
	double attempts = 0;
	for(int i = 0; i < bidCount; i++)
		attempts += HitRate(bids->at(i));
	*/

	Log::Write(Log::INFO,"================== Results ==================");
	Log::Write(Log::INFO, "Map contains %i blocks in %i positions", (int)bids->size(), (int)knownPoss->size());
	Log::Write(Log::INFO, "Overall map validity: %lf%%", (mapValidity/(double)bidCount)*100);
	Log::Write(Log::INFO, "Overall map knowledge of store: %lf%%", ((knownPoss->size()-changed)/(double)(found+knownPoss->size()))*100);
	//Log::Write(Log::INFO, "Overall average requests per block hit: %lf", attempts/(double)bidCount);

	delete map;
	delete unmapped;
	delete knownPoss;
	delete bids;
	delete newBids;
}

double MapCheck::Validity(int64_t bid, DataMap *map) {
	Log::Write(Log::INFO, "Accessing bid %i", bid);
	int64_t posCount = (int) map->Positions(bid).size();
	Log::Write(Log::INFO, "Map lists %i positions, %i verified", posCount, (int)map->VFPositions(bid).size());

	int poss = 0;
	for(int i = 0; i < posCount; i++){
		knownPoss->push_back(map->Positions(bid)[i]);
		Block b = Block(AES::Decrypt(key, store->Read(map->Positions(bid)[i]), blockSize));

		if(b.chg == 0)
				Log::Write(Log::DEBUG, "pos: %i bid: %i known by: %i times: %i, %i\t\tbytes: %i %i %i %i %i...",
							map->Positions(bid)[i], b.bid, b.cns, b.chg, map->LatestLocalVersion(bid), b.data[0],b.data[1],
							b.data[2],b.data[3],b.data[4]);
		else
				Log::Write(Log::DEBUG, "pos: %i bid: %i known by: %i times: %i, %i\tbytes: %i %i %i %i %i...",
							map->Positions(bid)[i], b.bid, b.cns, b.chg, map->LatestLocalVersion(bid), b.data[0],b.data[1],
							b.data[2],b.data[3],b.data[4]);
		if(b.bid == bid && b.chg == map->LatestLocalVersion(bid))
				poss++;
	}

	changed += (posCount - poss);
	double validity = poss/(double)posCount;

	if (0 < validity)
		Log::Write(Log::INFO, "%i positions (%lf%%) remain valid\n", poss, validity*100);
	else
		Log::Write(Log::WARNING, "%i positions (%lf%%) remain valid\n", poss, validity*100);

	return validity;
}

int MapCheck::ListUnmapped(int64_t bid, DataMap *map) {
	Log::Write(Log::INFO, "Unmapped positions found for bid %i", bid);
	int posCount = (int) map->Positions(bid).size();
	Log::Write(Log::INFO, "Store lists %i new positions", posCount);

	for(int i = 0; i < posCount; i++){
		Block b = Block(AES::Decrypt(key, store->Read(map->Positions(bid)[i]), blockSize));

		Log::Write(Log::DEBUG, "pos: %i bid: %i known by: %i time: %i\t\tbytes: %i %i %i %i %i...",
						map->Positions(bid)[i], b.bid, b.cns, b.chg, b.data[0],b.data[1],
						b.data[2],b.data[3],b.data[4]);
	}
	std::cout << std::endl;

	return posCount;
}

double MapCheck::HitRate(int64_t bid) {
	Block b;
	int totalAttempts = 0;
	int tests = 5;

	Log::Write(Log::INFO, "Testing hit rate of bid %i", bid);
	for(int j = 0; j < tests; j++) {
		b.bid = -2;
		int attempts = 0;
		TestStore *tStore = new TestStore(store);
		DataMap *map = new DataMap(blockCount, fileName);

		do {
				attempts++;

				size_t p1 = map->GetRandBlockPosition(bid);
				size_t p2 = AES::GetRandom(blockCount);

				int ibc = static_cast<int>(blockCount);
				int64_t bid2;
				for (int i = -1; i < ibc; i++) {
					std::vector<size_t> blockPositions = map->Positions(i);
					if (std::find(blockPositions.begin(), blockPositions.end(), p2) != blockPositions.end()) {
						bid2 = i;
						break;
					}
				}

				Block blk1, blk2, syncb1, syncb2, wblk1, wblk2;

				blk1 = Block(AES::Decrypt(key, tStore->Read(p1), blockSize));
				blk2 = Block(AES::Decrypt(key, tStore->Read(p2), blockSize));

				syncb1 = SyncPosition(bid, p1, blk1, map);
				syncb2 = SyncPosition(bid2, p2, blk2, map);

				b = syncb1;

				wblk1 = syncb1;

				wblk2 = DuplicateBlock(p2, syncb2, syncb1, map);

				tStore->Write(p1, AES::Encrypt(key, wblk1.toBytes()));
				tStore->Write(p2, AES::Encrypt(key, wblk2.toBytes()));

		} while (b.bid != bid);

		Log::Write(Log::DEBUG, "%i attempts to hit bid %i", attempts, bid);
		totalAttempts += attempts;

		delete tStore;
		delete map;
	}
	Log::Write(Log::INFO, "Average hit rate over %i tests on bid %i of %lf\n", tests, bid, totalAttempts/(double)tests);

	return totalAttempts/(double)tests;
}

Block MapCheck::SyncPosition(int64_t bid, size_t pos, Block block, DataMap *map){

	if(block.chg < map->LatestLocalVersion(block.bid)){
		map->RemovePosition(block.bid, pos);
		map->RemoveVFPosition(block.bid, pos);
		map->AddPosition(-1, pos);

		block.bid = -1;
		block.cns = 1;
		block.chg = time(nullptr);
	}
	else if(block.cns < NUM_CLIENTS){
		if(bid != block.bid){
				block.cns += 1;
				map->RemovePosition(bid, pos);
				map->RemoveVFPosition(bid, pos);

				if(block.chg > map->LatestLocalVersion(block.bid)){
					map->SetLocalVersion(block.bid, block.chg);
					map->SetPositions(block.bid, {pos});
					map->SetVFPositions(block.bid, {});

				}
				else if(block.chg == map->LatestLocalVersion(block.bid)){
					map->AddPosition(block.bid, pos);

				}
		}
		else if(block.chg > map->LatestLocalVersion(block.bid)){
				block.cns += 1;
				map->SetLocalVersion(block.bid, block.chg);
				map->SetPositions(block.bid, {pos});
				map->SetVFPositions(block.bid, {});

		}
	}

	if(block.cns == NUM_CLIENTS){
		map->AddVFPosition(block.bid, pos);
	}

	return block;
}

Block MapCheck::DuplicateBlock(size_t pos, Block block, Block orgBlock, DataMap *map){

	if(block.cns == NUM_CLIENTS && (int)map->VFPositions(block.bid).size() > NUM_CLIENTS){

		map->SetVFPositions(block.bid, {});
		map->RemovePosition(block.bid, pos);
		block = orgBlock;
		block.cns = 1;
		map->AddPosition(orgBlock.bid, pos);

	}
	else if(block.cns == 1 && (int)map->Positions(block.bid).size() > 1){

		map->RemovePosition(block.bid, pos);
		block = orgBlock;
		block.cns = 1;
		map->AddPosition(orgBlock.bid, pos);

	}

	return block;
}