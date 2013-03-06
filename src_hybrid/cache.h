#ifndef CACHE_H
#define CACHE_H

#include <sys/types.h>

class Cache {
  private:

	// Data storage
	int** tags;
	bool** valid;
	bool** dirty;
	int** lru;

	u_int32_t _blockSize;
	u_int32_t _assoc;
	u_int32_t _numSets;
	bool _randReplacement;
	bool _writeAlloc;
	bool _writeThrough;

  public:
	Cache(u_int32_t numSets, u_int32_t assoc, u_int32_t blockSize, bool randRep, bool writeAlloc, bool writeThrough); 

	~Cache(); 
	
	void reset(); 

	// check to see if it is in the cache (doesn't modify anything)
	bool check(u_int32_t addr, bool load); 

	u_int32_t getTag(u_int32_t addr); 

	u_int32_t getIndex(u_int32_t addr); 

	// accesses cache, returns true if we have a hit, false otherwise, don't change any state if modify == false
	bool access(u_int32_t addr, bool load); 
};

#endif
