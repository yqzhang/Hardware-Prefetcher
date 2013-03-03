#include "prefetcher.h"
#include "mem-sim.h"

Prefetcher::Prefetcher() {
  initLocalRequest();
  initHistoryState();
}

bool Prefetcher::hasRequest(u_int32_t cycle) {
	return false;
}

Request Prefetcher::getRequest(u_int32_t cycle) {
	Request req;
	return req;
}

void Prefetcher::completeRequest(u_int32_t cycle) {
  return;
}

void Prefetcher::cpuRequest(Request req) {
  return ;
}

// History state table operations
void initHistoryState() {
  stateCount = stateHead = 0;
}

bool ifEmptyHistoryState() {
  return (stateCount == 0);
}

bool ifFullHistoryState() {
  return (stateCount == MAX_STATE_COUNT);
}

void insertHistoryState(u_int32_t pc, u_int32_t addr, u_int16_t count, u_int16_t offset, u_int16_t ahead) {
  if (!ifFullHistoryState()) {
    historyState[stateCount].pc     = pc;
    historyState[stateCount].addr   = addr;
    historyState[stateCount].count  = count;
    historyState[stateCount].offset = offset;
    historyState[stateCount].ahead  = ahead;
    historyState[stateCount].next   = stateHead;
    
    stateHead = stateCount;
  }
  else {
    // TODO
  }
}

// Local request queue operations
void Prefetcher::initLocalRequest() {
  frontRequest = rearRequest = 0;
}

bool ifEmptyLocalRequest() {
  return (frontRequest == rearRequest);
}

bool ifFullLocalRequest() {
  return (frontRequest == (rearRequest + 1) % MAX_REQUEST_COUNT);
}

bool enLocalRequest(u_int32_t addr) {
  if (ifFullLocalRequest()) {
    return false;
  }
  else {
    localRequest[rearRequest] = addr;
    rearRequest = (rearRequest + 1) % MAX_REQUEST_COUNT;
    return true;
  }
}

u_int32_t deLocalRequest() {
  if (ifEmptyLocalRequest()) {
    return 0;
  }
  else {
    u_int32_t addr = localRequest[frontRequest];
    frontRequest = (frontRequest + 1) % MAX_REQUEST_COUNT;
    return addr;
  }
}

u_int32_t getFrontLocalRequest() {
  return localRequest[frontRequest];
}
