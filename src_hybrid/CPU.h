#ifndef CPU_H
#define CPU_H

#include <stdio.h>
#include <sys/types.h>
#include "mem-sim.h"

class CPU {
  private:
	FILE* _trace; // trace file

	Request _currReq; // the request the CPU is currently working on

	cpuState _state; // state of the CPU

	u_int32_t _readyAt; // cycle that the next request will be ready

	bool _done; // true if we are done issuing requests

	u_int32_t nRequests; // number of memory requests
	u_int32_t hitsL1; // number of L1 hits
	u_int32_t hitsL2; // number of L2 hits (doesn't include accesses that had an L1 hit)
	u_int64_t totalAccessTime; // total time spent working on memory requests

  public:
	CPU(char* trace_file); 

	~CPU();

	// done only after we have read and completed the last request
	bool isDone(); 

	void readNextRequest(u_int32_t cycle);

	// "issues" request by setting time stamp
	Request issueRequest(u_int32_t cycle); 

	// gets the current request (doesn't change time stamp)
	Request getRequest(); 

	// completes the request by setting to CPU to idle then looking for the next memory access
	void completeRequest(u_int32_t cycle);

	void hitL1(bool isHit);

	void loadHitL2(bool isHit);
	void storeHitL2(bool isHit);

	// returns the current CPU state
	cpuState getStatus(u_int32_t cycle); 

	// update the CPU's status
	void setStatus(cpuState new_state);

	double getHitRateL1(); 
	double getHitRateL2();
	double getAMAT();
};

#endif
