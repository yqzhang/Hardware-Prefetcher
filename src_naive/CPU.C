#include "CPU.h"

CPU::CPU(char* trace_file) {
	_trace = fopen(trace_file,"r");

	_state = IDLE;
	_done = false;
	_currReq.fromCPU = true;

	nRequests = hitsL1 = hitsL2 = 0;
	totalAccessTime = 0;

	readNextRequest(0); // perform the first read
}

CPU::~CPU() { fclose(_trace); }

// done only after we have read and completed the last request
bool CPU::isDone() { return _done; }

void CPU::readNextRequest(u_int32_t cycle) {
	char ld;
	u_int32_t pc, addr, cycles_since_last;

	if(fscanf(_trace, "%c %x %x %u\n",&ld,&pc,&addr,&cycles_since_last) == 4) {
		_currReq.addr = addr;
		if(ld == 'l') _currReq.load = true;
		else _currReq.load = false;

		_currReq.pc = pc;

		_readyAt = cycle + cycles_since_last + 1; // calculate when we will next be ready

		_currReq.HitL1 = false;
		_currReq.HitL2 = false;

		//printf("next memop ready at cycle %u\n",_readyAt);

		nRequests++;
	}
	else { _done = true; } // couldn't read anymore so we are done
}

// "issues" request by setting time stamp
Request CPU::issueRequest(u_int32_t cycle) { 
	_currReq.issuedAt = cycle;
	return _currReq; 
}

// gets the current request (doesn't change time stamp)
Request CPU::getRequest() { return _currReq; }

// completes the request by setting to CPU to idle then looking for the next memory access
void CPU::completeRequest(u_int32_t cycle) {
	_state = IDLE; // go to idle state

	totalAccessTime += cycle - _currReq.issuedAt + 1; // add the number of cycles needed to access

	// track the hit rate stats
	if(_currReq.HitL1) hitsL1++;
	else if(_currReq.HitL2) hitsL2++;

	readNextRequest(cycle); // read the next request
}

void CPU::hitL1(bool isHit) { _currReq.HitL1 = isHit; }

void CPU::loadHitL2(bool isHit) { _currReq.HitL2 = isHit; }
void CPU::storeHitL2(bool isHit) { if(isHit) hitsL2++; }

// returns the current CPU state
cpuState CPU::getStatus(u_int32_t cycle) { 
	if(_state == IDLE && cycle >= _readyAt) _state = READY; // see if we are now ready (from idle state)
	return _state; 
}

// update the CPU's status
void CPU::setStatus(cpuState new_state) { _state =  new_state; 
//if(_state == WAITING) 
//fprintf(stderr,"changed to waiting\n");
}

double CPU::getHitRateL1() { return (double)hitsL1 / (double)nRequests; }

double CPU::getHitRateL2() { 
	u_int32_t nRequestsL2 = nRequests - hitsL1; // anything that doesn't hit in L1 goes to L2
	return (double)hitsL2 / (double)nRequestsL2; 
}

double CPU::getAMAT() { return (double)totalAccessTime / (double)nRequests; }
