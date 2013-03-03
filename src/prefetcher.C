#include "prefetcher.h"
#include "mem-sim.h"

Prefetcher::Prefetcher() {
  stateCount = 0;
  stateHead = 0;
  frontRequest = 0;
  rearRequest = 0;
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
