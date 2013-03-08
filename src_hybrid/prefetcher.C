/**
 * Name:  Yunqi Zhang
 * PID:   A53030068
 * Email: yqzhang@ucsd.edu
 */

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
  u_int16_t index = queryHistoryState(req.pc);

  // Not in the history state table
  if (index == NULL_STATE) {
    insertHistoryState(req.pc, req.addr, 1, DEFAULT_OFFSET, DEFAULT_AHEAD, STATE_INIT);
    sequentialPrefetch(req.addr, DEFAULT_OFFSET, DEFAULT_AHEAD, req.HitL1);
  }
  // Already in the history state table
  else {
    int16_t offset = (int64_t)req.addr - (int64_t)historyState[index].addr;
      
    switch (historyState[index].state) {
    case STATE_INIT:
      if (offset == historyState[index].offset) {
        historyState[index].state = STATE_STEADY;
        historyState[index].addr  = req.addr;
        historyState[index].count ++;
        historyState[index].ahead --;
        stridePrefetch(req.addr, offset, historyState[index].count, historyState[index].ahead, req.HitL1);
      }
      else {
        historyState[index].state = STATE_TRANS;
        historyState[index].addr  = req.addr;
        historyState[index].count = 1;
        historyState[index].offset= offset;
        historyState[index].ahead = DEFAULT_AHEAD;
        sequentialPrefetch(req.addr, NOT_HIT_OFFSET, NOT_HIT_AHEAD, req.HitL1);
      }
      break;
    case STATE_TRANS:
      if (offset == historyState[index].offset) {
        historyState[index].state = STATE_STEADY;
        historyState[index].addr  = req.addr;
        historyState[index].count = 2;
        historyState[index].ahead --;
        stridePrefetch(req.addr, offset, historyState[index].count, historyState[index].ahead, req.HitL1);
      }
      else {
        historyState[index].state = STATE_NO_PRE;
        historyState[index].addr  = req.addr;
        historyState[index].count = 1;
        historyState[index].offset= offset;
        historyState[index].ahead = DEFAULT_AHEAD;
        sequentialPrefetch(req.addr, NOT_HIT_OFFSET, NOT_HIT_AHEAD, req.HitL1);
      }
      break;
    case STATE_STEADY:
      if (offset == historyState[index].offset) {
        historyState[index].state = STATE_STEADY;
        historyState[index].addr  = req.addr;
        historyState[index].count += 4;
        historyState[index].ahead --;
        stridePrefetch(req.addr, offset, historyState[index].count, historyState[index].ahead, req.HitL1);
      }
      else {
        historyState[index].state = STATE_TRANS;
        historyState[index].addr  = req.addr;
        historyState[index].offset= offset;
        historyState[index].count = 1;
        historyState[index].ahead = DEFAULT_AHEAD;
        sequentialPrefetch(req.addr, NOT_HIT_OFFSET, NOT_HIT_AHEAD, req.HitL1);
      }
      break;
    case STATE_NO_PRE:
      if (offset == historyState[index].offset) {
        historyState[index].state = STATE_TRANS;
        historyState[index].addr  = req.addr;
        historyState[index].count = 2;
        historyState[index].ahead --;
        sequentialPrefetch(req.addr, offset, historyState[index].ahead, req.HitL1);
      }
      else {
        historyState[index].addr  = req.addr;
        historyState[index].count = 1;
        historyState[index].offset= offset;
        historyState[index].ahead = DEFAULT_AHEAD;
        sequentialPrefetch(req.addr, NOT_HIT_OFFSET, NOT_HIT_AHEAD, req.HitL1);
      }
      break;
    default:
      printf("An error occurred in the finite state machine T_T\n");
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
      temp.state  = historyState[index].state;

      historyState[index].pc      = historyState[next].pc;
      historyState[index].addr    = historyState[next].addr;
      historyState[index].count   = historyState[next].count;
      historyState[index].offset  = historyState[next].offset;
      historyState[index].ahead   = historyState[next].ahead;
      historyState[index].state   = historyState[next].state;
      historyState[index].next    = historyState[next].next;

      historyState[next].pc     = temp.pc;
      historyState[next].addr   = temp.addr;
      historyState[next].count  = temp.count;
      historyState[next].offset = temp.offset;
      historyState[next].ahead  = temp.ahead;
      historyState[next].state  = temp.state;
      historyState[next].next   = stateHead;
      stateHead = next;
    }
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

void Prefetcher::insertHistoryState(u_int32_t pc, u_int32_t addr, u_int16_t count, int16_t offset, u_int16_t ahead, u_int8_t state) {
  if (!ifFullHistoryState()) {
    // Insert state when table is not full
    historyState[stateCount].pc     = pc;
    historyState[stateCount].addr   = addr;
    historyState[stateCount].count  = count;
    historyState[stateCount].offset = offset;
    historyState[stateCount].ahead  = ahead;
    historyState[stateCount].next   = stateHead;
    historyState[stateCount].state  = state;
    
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
    historyState[replace].state   = state;

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

bool Prefetcher::enLocalRequest(u_int32_t addr, bool HitL1) {
  if (ifAlreadyInQueue(addr, HitL1)) {
    return false;
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

bool Prefetcher::ifAlreadyInQueue(u_int32_t addr, bool HitL1) {
  u_int32_t i;
  for (i = frontRequest; i % MAX_REQUEST_COUNT < rearRequest; i++) {
    if (HitL1) {
      if (localRequest[i % MAX_REQUEST_COUNT] / L1_CACHE_BLOCK == addr / L1_CACHE_BLOCK) {
        return true;
      }
    }
    else {
      if (localRequest[i % MAX_REQUEST_COUNT] / L2_CACHE_BLOCK == addr / L2_CACHE_BLOCK) {
        return true;
      }
    }
  }
  return false;
}

void Prefetcher::sequentialPrefetch(u_int32_t addr, int16_t offset, u_int16_t ahead, bool HitL1) {
  int16_t i;
  if (offset != 0) {
    for (i = -1; i <= ahead; i++) {
      u_int32_t prefetch;
      prefetch = (int64_t)addr + (int64_t)i * (int64_t)offset;
      if (HitL1) {
        if (prefetch / L1_CACHE_BLOCK == addr / L1_CACHE_BLOCK) continue;
      }
      else {
        if (prefetch / L2_CACHE_BLOCK == addr / L2_CACHE_BLOCK) continue;
      }
      enLocalRequest(prefetch, false);
    }  
  }
}

void Prefetcher::stridePrefetch(u_int32_t addr, int16_t offset, u_int16_t count, u_int16_t &ahead, bool HitL1) {
  if (offset != 0) {
    while (ahead < MIN(count, MAX_PREFETCH_DEGREE)) {
      ahead++;
      enLocalRequest((int64_t)addr + (int64_t)ahead * (int64_t)offset, HitL1);
    }
  }
}
