#include "cache.h"
#include <stdlib.h>
#include <math.h>

Cache::Cache(u_int32_t numSets, u_int32_t assoc, u_int32_t blockSize, bool randRep, bool writeAlloc, bool writeThrough) {
	_numSets = numSets;
	_blockSize = blockSize;
	_assoc = assoc;
	_randReplacement = randRep;
	_writeAlloc = writeAlloc;
	_writeThrough = writeThrough;

	//printf("numsets %u, blocksize %u, assoc %u\n",_numSets,_blockSize,_assoc);

	tags = (int**)malloc(_numSets * sizeof(int *));
	valid = (bool**)malloc(_numSets * sizeof(bool *));
	dirty = (bool**)malloc(_numSets * sizeof(bool *));
	lru = (int**)malloc(_numSets * sizeof(int *));

	for(int i = 0; i < _numSets; ++i) {
		tags[i] = (int*)malloc(_assoc * sizeof(int));
		valid[i] = (bool*)malloc(_assoc * sizeof(bool));
		dirty[i] = (bool*)malloc(_assoc * sizeof(bool));
		lru[i] = (int*)malloc(_assoc * sizeof(int));
	}

	// init data
	reset();
}

Cache::~Cache() {
	for(int i = 0; i < _numSets; ++i) {
		free(tags[i]);
		free(valid[i]);
		free(dirty[i]);
		free(lru[i]);
	}
	free(tags);
	free(valid);
	free(dirty);
	free(lru);
}

void Cache::reset() {
	for(int i = 0; i < _numSets; ++i) {
		for(int j = 0; j < _assoc; ++j) {
			tags[i][j] = lru[i][j] = 0;
			dirty[i][j] = false;
			valid[i][j] = false;
		}
	}

	// reseed the random generator
	srand(100);
}

// check to see if it is in the cache (doesn't modify anything)
bool Cache::check(u_int32_t addr, bool load) {
	bool isHit = false;

	int b = (int)log2((double)_blockSize);
	int s = (int)log2((double)_numSets);
	int t = 32 - b - s;

	int index = ((addr >> b) << (32 - s)) >> (32 - s);
	int tag = addr >> (b + s);

	for(int i = 0; i < _assoc; ++i) {
		if(tag == tags[index][i] && valid[index][i]) { // found it
			isHit = true;
			break; // no need to search more
		}
	}

	return isHit;
}

u_int32_t Cache::getTag(u_int32_t addr) {
	int b = (int)log2((double)_blockSize);
	int s = (int)log2((double)_numSets);
	int t = 32 - b - s;

	u_int32_t tag = addr >> (b + s);
	return tag;
}

u_int32_t Cache::getIndex(u_int32_t addr) {
	int b = (int)log2((double)_blockSize);
	int s = (int)log2((double)_numSets);

	u_int32_t index = ((addr >> b) << (32 - s)) >> (32 - s);
	return index;
}

// accesses cache, returns true if we have a hit, false otherwise, don't change any state if modify == false
bool Cache::access(u_int32_t addr, bool load) {
	bool isHit = false;

	int b = (int)log2((double)_blockSize);
	int s = (int)log2((double)_numSets);
	int t = 32 - b - s;

	//printf("b %d, s %d, t %d\n",b,s,t);

	int index = ((addr >> b) << (32 - s)) >> (32 - s);
	int tag = addr >> (b + s);

	if(_numSets == 1) index = 0;

	//printf("index %d\n",index);

	bool inCache = false;
	bool foundEmpty;

	// first check to see if the data is already in there
	for(int i = 0; i < _assoc; ++i) {
		if(tag == tags[index][i] && valid[index][i]) { // found it
			isHit = true;

			if(!load)
				dirty[index][i] = true; // its dirty now that we stored to it

			// update LRU information (no need to do this for DM)
			if(_assoc > 1 && !_randReplacement) {
				// for all that had smaller LRU numbers, add 1 to them
				for(int j = 0; j < _assoc; ++j) {
					if(lru[index][j] < lru[index][i] && valid[index][j]) 
						lru[index][j]++;
				}

				lru[index][i] = 0;
			}

			inCache = true;
			break; // no need to search more
		}
	}

	// if it wasn't in cache, bring it in if it was a load or if we are doing write allocate
	if(!inCache && (load || _writeAlloc)) {
		foundEmpty = false;

		// first search to see if there is an open spot at that index
		for(int i = 0; i < _assoc; ++i) {
			if(!valid[index][i]) { // found an open spot... all is good with the world
				tags[index][i] = tag;
				valid[index][i] = true;
				if(!load) dirty[index][i] = true;

				// need to update LRU bits if we are using that policy
				if(_assoc > 1 && !_randReplacement) {
					// for all that had smaller LRU numbers, add 1 to them
					for(int j = 0; j < _assoc; ++j) {
						if(valid[index][j])  // since we found an open slot, we can just increase all the others without worrying
							lru[index][j]++;
					}

					lru[index][i] = 0;
				}

				foundEmpty = true;
				break;
			}
		}

		// if we couldn't find an empty spot, we need to kick out something old
		if(!foundEmpty) {
			// if DM, just put it in the only slot we have...
			if(_assoc == 1) {
				tags[index][0] = tag;
				valid[index][0] = true;

				//if(dirty[index][0]) evicted = true;

				if(load) dirty[index][0] = false;
				else dirty[index][0] = true;
			}

			// just kick out a random guy if we specified random replacement policy
			else if(_randReplacement) {
				int temp = rand() % _assoc;
				tags[index][temp] = tag;
				valid[index][temp] = true;

				//if(dirty[index][temp]) evicted = true;

				if(load) dirty[index][temp] = false;
				else dirty[index][temp] = true;
			}

			// find LRU and kick him out
			else {
				for(int i = 0; i < _assoc; ++i) {
					lru[index][i]++;
					if(lru[index][i] == _assoc) { // this is the LRU
						tags[index][i] = tag;
						valid[index][i] = true;

						//if(dirty[index][i]) evicted = true;

						if(load) dirty[index][i] = false;
						else dirty[index][i] = true;

						lru[index][i] = 0;
					}
				}
			}
		}
	}

	return isHit;
}
