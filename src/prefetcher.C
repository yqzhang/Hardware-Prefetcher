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

void Prefetcher::cpuRequest(Request req) {
  if (req.HitL1) {
    u_int16_t index = queryHistoryState(req.pc);
    if (index == NULL_STATE) {
      insertHistoryState(req.pc, req.addr, 1, 0, 0);
      printf("weired\n");
    }
    else {
      // Second time
      if (historyState[index].time == 1) {
        historyState[index].offset = req.addr - historyState[index].addr;
        historyState[index].addr = req.addr;
        historyState[index].count++;
        historyState[index].ahead = (L1_CACHE_BLOCK - (req.addr % L1_CACHE_BLOCK)) / historyState[index].offset;
      }
      else {
        historyState[index].offset = req.addr - hitoryState[index].addr;
        historyState[index].addr = req.addr;
        historyState[index].count++;
        historyState[index].ahead--;
      }

      // Prefetch MIN(count, PREFETCH_DEGREE)
      while (historyState[index].ahead < MIN(historyState[index].count, PREFETCH_DEGREE)) {
        u_int32_t addr = historyState[index].addr + historyState[index].offset *
                       (historyState[index].ahead + 1);
        enLocalRequest(addr);
        historyState[index].ahead += (L1_CACHE_BLOCK - (addr % L1_CACHE_BLOCK)) / historyState[index].offset;
      }

      //Place to the most recent used head
      if (historyState[index].next == NULL_STATE) {
        u_int16_t prev = getSecondLRUState();

        historyState[prev].next = NULL_STATE;
        historyState[index].next = stateHead;
        stateHead = index;
      }
      else {
        u_int16_t next = historyState[index].next;

        State temp;
        temp.pc     = historyState[index].pc;
        temp.addr   = historyState[index].addr;
        temp.count  = historyState[index].count;
        temp.offset = historyState[index].offset;
        temp.ahead  = historyState[index].ahead;

        historyState[index].pc      = historyState[next].pc;
        historyState[index].addr    = historyState[next].addr;
        historyState[index].count   = historyState[next].count;
        historyState[index].offset  = historyState[next].offset;
        historyState[index].ahead   = historyState[next].ahead;
        historyState[index].next    = historyState[next].next;

        historyState[next].pc     = temp.pc;
        historyState[next].addr   = temp.addr;
        historyState[next].count  = temp.count;
        historyState[next].offset = temp.offset;
        historyState[next].ahead  = temp.ahead;
        historyState[next].next   = stateHead;
        stateHead = next;
      }
    }
  }
  else if (req.HitL2) {

  }
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

void Prefetcher::insertHistoryState(u_int32_t pc, u_int32_t addr, u_int16_t count, u_int16_t offset, u_int16_t ahead) {
  if (!ifFullHistoryState()) {
    // Insert state when table is not full
    historyState[stateCount].pc     = pc;
    historyState[stateCount].addr   = addr;
    historyState[stateCount].count  = count;
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
    historyState[replace].count   = count;
    historyState[replace].offset  = offset;
    historyState[replace].ahead   = ahead;
    historyState[replace].next    = stateHead;

    stateHead = replace;
  }
}

void updateHistoryState() {

}

u_int16_t queryHistoryState(u_int32_t pc) {
  u_int16_t i;
  for (i = stateHead; historyState[i].next != NULL_STATE; i = historyState[i].next ) {
    if (historyState[i].pc == pc) {
      return i;
    }
  }
  return NULL_STATE;
}

u_int32_t Prefetcher::getSecondLRUState() {
  u_int32_t i;
  for (i = 0; i < MAX_STATE_COUNT; i++) {
    if (historyState[historyState[i].next].next == NULL_STATE) {
      return i;
    }
  }
  printf("Error -- from Prefetcher::getSecondLRUState()")
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
    printf("Local Request full!\n");
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
