#ifndef MEM_SIM_H
#define MEM_SIM_H

#include <sys/types.h>

enum cpuState {
	IDLE, 		// no requests at this time
	READY, 		// request ready
	WAITING, 	// waiting for request to be filled from L2 or mem
	STALLED_L2, // stalled waiting for D-to-L2 bus
	STALLED_WB 	// stalled waiting for write_buffer
};

struct Request {
	u_int32_t addr; // effective address of request (32 bits)
	u_int32_t pc; // PC of request (32 bits)
	bool load; // true if load, false if store
	bool fromCPU; // true if request originated in CPU (rather than prefetcher)

	u_int32_t issuedAt; // cycle that the request was issued
	bool HitL1; // if it was a hit in D-cache
	bool HitL2; // if it was a hit in L2 (meaningless if HitL1 == true)
};

#endif
