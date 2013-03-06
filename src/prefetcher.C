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
  if (index == NULL_STATE) {
    insertHistoryState(req.pc, req.addr, 1, 0, 0);
    enLocalRequest(req.addr + 32);
    enLocalRequest(req.addr + 64);
  }
  else {
    // Second time
    if (historyState[index].count == 1) {
      historyState[index].offset = (int16_t)((int64_t)req.addr) - ((int64_t)historyState[index].addr);
      historyState[index].addr = req.addr;
      historyState[index].count++;
      if (req.HitL1) {
        if (historyState[index].offset > 0) {
          historyState[index].ahead = (L1_CACHE_BLOCK - (req.addr % L1_CACHE_BLOCK)) / historyState[index].offset;
        }
        else if (historyState[index].offset < 0) {
          historyState[index].ahead = (req.addr % L1_CACHE_BLOCK) / (- historyState[index].offset);
        }
        else {
          historyState[index].ahead = 0;
        }
      }
      else {
        if (historyState[index].offset > 0) {
          historyState[index].ahead = (L2_CACHE_BLOCK - (req.addr % L2_CACHE_BLOCK)) / historyState[index].offset;
        }
        else if (historyState[index].offset < 0) {
          historyState[index].ahead = (req.addr % L2_CACHE_BLOCK) / (- historyState[index].offset);
        }
        else {
          historyState[index].ahead = 0;
        }
      }
    }
    else {
      int16_t previous = historyState[index].offset;
      historyState[index].offset = (int64_t)req.addr - (int64_t)historyState[index].addr;
      historyState[index].addr = req.addr;
      if (previous != historyState[index].offset) {
        historyState[index].count = 1;
        enLocalRequest(req.addr + 32);
        enLocalRequest(req.addr + 64);
      }
      else {
        historyState[index].count++;
      }
      if (historyState[index].offset != 0) {
        historyState[index].ahead--;
      }
    }

    // Prefetch MIN(count - 1, PREFETCH_DEGREE)
    if (historyState[index].offset != 0) {
      u_int16_t prefetchDegree = (req.HitL1) ? (L1_PREFETCH_DEGREE) : (L2_PREFETCH_DEGREE);
      while (historyState[index].ahead < MIN(historyState[index].count - 1, prefetchDegree)) {
        u_int32_t addr = (int64_t)historyState[index].addr + (int64_t)(historyState[index].offset) *
                         ((int64_t)historyState[index].ahead + 1);
        enLocalRequest(addr);
        if (req.HitL1) {
          if (historyState[index].offset > 0) {
            historyState[index].ahead += (L1_CACHE_BLOCK - (addr % L1_CACHE_BLOCK)) / historyState[index].offset + 1;
          }
          else {
            historyState[index].ahead += (addr % L1_CACHE_BLOCK) / (- historyState[index].offset) + 1;
          }
        }
        else {
          if (historyState[index].offset > 0) {
            historyState[index].ahead += (L2_CACHE_BLOCK - (addr % L2_CACHE_BLOCK)) / historyState[index].offset + 1;
          }
          else {
            historyState[index].ahead += (addr % L1_CACHE_BLOCK) / (- historyState[index].offset) + 1;
          }
        }
      }
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

void Prefetcher::insertHistoryState(u_int32_t pc, u_int32_t addr, u_int16_t count, int16_t offset, u_int16_t ahead) {
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

bool Prefetcher::enLocalRequest(u_int32_t addr) {
  if (ifAlreadyInQueue(addr)) {
    //printf("full\n");
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

bool Prefetcher::ifAlreadyInQueue(u_int32_t addr) {
  u_int32_t i;
  for (i = frontRequest; i % MAX_REQUEST_COUNT < rearRequest; i++) {
    if (localRequest[i % MAX_REQUEST_COUNT] / 32 == addr / 32) {
      return true;
    }
  }
  return false;
}
