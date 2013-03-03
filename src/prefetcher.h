#ifndef PREFETCHER_H
#define PREFETCHER_H

#define MAX_STATE_COUNT 256
#define MAX_REQUEST_COUNT 10

#include <sys/types.h>
#include <stdio.h>

struct Request;

struct State {
  u_int32_t pc;     // PC of the state
  u_int32_t addr;   // Last access address
  u_int16_t count;  // Access counter
  u_int16_t offset; // Access offset
  u_int16_t ahead;  // Accesses prefetched ahead
  u_int16_t next;   // Next state for LRU
};

class Prefetcher {
  private:
  // History state table - Size: sizeof(State) * MAX_STATE_COUNT = 4KB
  u_int32_t stateCount;
  u_int32_t stateHead;
  State historyState[MAX_STATE_COUNT];

  // Local request queue
  u_int32_t frontRequest;
  u_int32_t rearRequest;
  u_int32_t localRequest[MAX_REQUEST_COUNT];

  // History state table operations
  // TODO
  void initHistoryState();
  bool ifEmptyHistoryState();
  bool ifFullHistoryState();
  void insertHistoryState(u_int32_t, u_int32_t, u_int16_t, u_int16_t, u_int16_t);
  void updateHistoryState();
  State queryHistoryState(u_int32_t);

  // Local request queue operations
  void initLocalRequest();
  bool ifEmptyLocalRequest();
  bool ifFullLocalRequest();
  bool enLocalRequest(u_int32_t);
  u_int32_t deLocalRequest();
  u_int32_t getFrontLocalRequest();

  public:
  // Construction function
  Prefetcher();

	// should return true if a request is ready for this cycle
	bool hasRequest(u_int32_t cycle);

	// request a desired address be brought in
	Request getRequest(u_int32_t cycle);

	// this function is called whenever the last prefetcher request was successfully sent to the L2
	void completeRequest(u_int32_t cycle);

	/*
	 * This function is called whenever the CPU references memory.
	 * Note that only the addr, pc, load, issuedAt, and HitL1 should be considered valid data
	 */
	void cpuRequest(Request req); 
};

#endif
