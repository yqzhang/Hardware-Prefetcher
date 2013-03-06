#ifndef PREFETCHER_H
#define PREFETCHER_H

#define MAX_STATE_COUNT 400
#define MAX_REQUEST_COUNT 32
#define NULL_STATE 0xFFFF
#define L1_PREFETCH_DEGREE 24
#define L2_PREFETCH_DEGREE 12
#define L1_CACHE_BLOCK 16
#define L2_CACHE_BLOCK 32

#define STATE_INIT 0
#define STATE_STEADY 1
#define STATE_TRANSIENT 2
#define STATE_NO_PRED 3

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#include <sys/types.h>
#include <stdio.h>

#include "cache.h"

struct Request;

struct State {
  u_int32_t pc;     // PC of the state
  u_int32_t addr;   // Last access address
  u_int16_t state;  // Access counter
  int16_t offset;   // Access offset
  u_int16_t ahead;  // Accesses prefetched ahead
  u_int16_t next;   // Next state for LRU
};

class Prefetcher {
  private:
  // History state table - Size: sizeof(State) * MAX_STATE_COUNT
  u_int32_t stateCount;
  u_int32_t stateHead;
  State historyState[MAX_STATE_COUNT];

  // Local request queue
  u_int32_t frontRequest;
  u_int32_t rearRequest;
  u_int32_t localRequest[MAX_REQUEST_COUNT];

  // History state table operations
  void initHistoryState();
  bool ifEmptyHistoryState();
  bool ifFullHistoryState();
  void insertHistoryState(u_int32_t, u_int32_t, u_int16_t, u_int16_t, u_int16_t);
  u_int16_t queryHistoryState(u_int32_t);
  u_int16_t getSecondLRUState();

  void printHistoryState();

  // Local request queue operations
  void initLocalRequest();
  bool ifEmptyLocalRequest();
  bool ifFullLocalRequest();
  bool enLocalRequest(u_int32_t);
  u_int32_t deLocalRequest();
  u_int32_t getFrontLocalRequest();
  bool ifAlreadyInQueue(u_int32_t);

  void prefetchRequest(u_int32_t, int16_t, u_int32_t);

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
	void cpuRequest(Request req, Cache *l1, Cache *l2); 
};

#endif
