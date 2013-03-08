/**
 * Name:  Yunqi Zhang
 * PID:   A53030068
 * Email: yqzhang@ucsd.edu
 */

#ifndef PREFETCHER_H
#define PREFETCHER_H

#define MAX_STATE_COUNT 64
#define MAX_REQUEST_COUNT 64
#define NULL_STATE 0xFF
#define L1_CACHE_BLOCK 16
#define L2_CACHE_BLOCK 32

#define DEFAULT_OFFSET 32
#define DEFAULT_AHEAD  2

#define NOT_HIT_OFFSET 16
#define NOT_HIT_AHEAD  2

#define MAX_PREFETCH_DEGREE 16

#define STATE_INIT   0
#define STATE_TRANS  1
#define STATE_STEADY 2
#define STATE_NO_PRE 3

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define ABS(x)    (((x) < 0) ? -(x) : (x))

#include <sys/types.h>
#include <stdio.h>

struct Request;

struct State {
  u_int32_t pc;     // PC of the state      - 32 bit
  u_int32_t addr;   // Last access address  - 32 bit
  u_int16_t count;  // Access counter       - 16 bit
  int16_t offset;   // Access offset        - 16 bit
  u_int16_t ahead;  // Prefetching ahead    - 16 bit
  u_int8_t next;   // Next state for LRU   - 16 bit
  u_int8_t state;   // "state" of state     - 8  bit
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
  void insertHistoryState(u_int32_t, u_int32_t, u_int16_t, int16_t, u_int16_t, u_int8_t);
  u_int16_t queryHistoryState(u_int32_t);
  u_int16_t getSecondLRUState();

  // Local request queue operations
  void initLocalRequest();
  bool ifEmptyLocalRequest();
  bool ifFullLocalRequest();
  bool enLocalRequest(u_int32_t, bool);
  u_int32_t deLocalRequest();
  u_int32_t getFrontLocalRequest();
  bool ifAlreadyInQueue(u_int32_t, bool);

  // Prefetch algorithm
  void sequentialPrefetch(u_int32_t, int16_t, u_int16_t, bool);
  void stridePrefetch(u_int32_t, int16_t, u_int16_t, u_int16_t&, bool);

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
