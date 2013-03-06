#include "prefetcher.h"
#include "mem-sim.h"

Prefetcher::Prefetcher() {
  initLocalRequest();
  initHistoryState();
}

bool Prefetcher::hasRequest(u_int32_t cycle) {
	return (!ifEmptyLocalRequest());
}

Request Prefetcher::getRequest(u_int32_t cycle) {
	Request req;
  req.addr = getFrontLocalRequest();
	return req;
}

void Prefetcher::completeRequest(u_int32_t cycle) {
  deLocalRequest();
}

void Prefetcher::cpuRequest(Request req, Cache *l1, Cache *l2) {
  //printf("req pc: %u, req addr: %u\n", req.pc, req.addr);
  //printHistoryState();
  //getchar();

    enLocalRequest(req.addr + 32);
    enLocalRequest(req.addr + 64);
    enLocalRequest(req.addr + 96);
    return ;
}

// History state table operations
void Prefetcher::initHistoryState() {
  stateCount = 0;
  stateHead = NULL_STATE;
}

bool Prefetcher::ifEmptyHistoryState() {
  return (stateCount == 0);
}

bool Prefetcher::ifFullHistoryState() {
  return (stateCount == MAX_STATE_COUNT);
}

void Prefetcher::insertHistoryState(u_int32_t pc, u_int32_t addr, u_int16_t state, u_int16_t offset, u_int16_t ahead) {
  if (!ifFullHistoryState()) {
    // Insert state when table is not full
    historyState[stateCount].pc     = pc;
    historyState[stateCount].addr   = addr;
    historyState[stateCount].state  = state;
    historyState[stateCount].offset = offset;
    historyState[stateCount].ahead  = ahead;
    historyState[stateCount].next   = stateHead;
    
    stateHead = stateCount;
    stateCount++;
  }
  else {
    // LRU eviction in history state table
    u_int32_t index = getSecondLRUState();
    u_int32_t replace = historyState[index].next;
    historyState[index].next = NULL_STATE;

    historyState[replace].pc      = pc;
    historyState[replace].addr    = addr;
    historyState[replace].state   = state;
    historyState[replace].offset  = offset;
    historyState[replace].ahead   = ahead;
    historyState[replace].next    = stateHead;

    stateHead = replace;
  }
}

u_int16_t Prefetcher::queryHistoryState(u_int32_t pc) {
  u_int16_t i;
  for (i = stateHead; i != NULL_STATE; i = historyState[i].next ) {
    if (historyState[i].pc == pc) {
      return i;
    }
  }
  return NULL_STATE;
}

u_int16_t Prefetcher::getSecondLRUState() {
  u_int16_t i;
  for (i = 0; i < MAX_STATE_COUNT; i++) {
    if (historyState[i].next == NULL_STATE) {
      continue;
    }
    else if (historyState[historyState[i].next].next == NULL_STATE) {
      return i;
    }
  }
  printf("Error -- from Prefetcher::getSecondLRUState()");
}

void Prefetcher::printHistoryState() {
  u_int32_t i, j;
  for (i = stateHead, j = 0; i != NULL_STATE && j < 32; i = historyState[i].next, j++) {
    printf("pc: %u, addr: %u state: %u, offset: %d, ahead: %u\n", historyState[i].pc, historyState[i].addr, historyState[i].state, historyState[i].offset, historyState[i].ahead);
  }
}

// Local request queue operations
void Prefetcher::initLocalRequest() {
  frontRequest = rearRequest = 0;
}

bool Prefetcher::ifEmptyLocalRequest() {
  return (frontRequest == rearRequest);
}

bool Prefetcher::ifFullLocalRequest() {
  return (frontRequest == (rearRequest + 1) % MAX_REQUEST_COUNT);
}

bool Prefetcher::enLocalRequest(u_int32_t addr) {
  if (ifAlreadyInQueue(addr)) {
    return true;
  }


  if (ifFullLocalRequest()) {
    return false;
  }
  else {
    localRequest[rearRequest] = addr;
    rearRequest = (rearRequest + 1) % MAX_REQUEST_COUNT;
    return true;
  }
}

u_int32_t Prefetcher::deLocalRequest() {
  if (ifEmptyLocalRequest()) {
    return 0;
  }
  else {
    u_int32_t addr = localRequest[frontRequest];
    frontRequest = (frontRequest + 1) % MAX_REQUEST_COUNT;
    return addr;
  }
}

u_int32_t Prefetcher::getFrontLocalRequest() {
  return localRequest[frontRequest];
}

bool Prefetcher::ifAlreadyInQueue(u_int32_t addr) {
  u_int32_t i;
  for (i = frontRequest; i % MAX_REQUEST_COUNT < rearRequest; i++) {
    if (localRequest[i % MAX_REQUEST_COUNT] / 16 == addr / 16) {
      return true;
    }
  }
  return false;
}

void Prefetcher::prefetchRequest(u_int32_t addr, int16_t offset, u_int32_t degree) {
  u_int32_t i, prefetch;
  for (i = 0, prefetch = addr + offset; i < degree; i++, prefetch += offset) {
    if(addr / 32 != prefetch / 32 && !ifAlreadyInQueue(prefetch)) {
      //printf("%u offset: %d\n", prefetch, offset);
      //getchar();
      enLocalRequest(prefetch);
    }
  }
}
