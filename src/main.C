/*
 *
 * File: main.C
 * Author: Saturnino Garcia (sat@cs)
 * Description: Cache simulator for 2 level cache
 *
 */

#include "cache.h"
#include "CPU.h"
#include "mem-sim.h"
#include "memQueue.h"
#include "prefetcher.h"

#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[]) {
	if(argc != 2) {
		printf("Usage: %s [trace file]\n",argv[0]);
		return 1;
	}

	int hitTimeL1 = 1; 
	int accessTimeL2 = 20; 
	int accessTimeMem = 50; 

	int lineSizeL1 = 16;
	int assocL1 = 2; 
	int totalSizeL1 = 32; 

	int lineSizeL2 = 32; 
	int assocL2 = 8; 
	int totalSizeL2 = 256; 

	int numSetsL1, numSetsL2;


	u_int32_t addr, cycles;
	u_int32_t runtime = 0; // number of cycles in runtime
	u_int32_t nonmemrt = 0; // number of cycles in runtime spent with non-mem instructions

	FILE *fp; // to write out the final stats

	// calc number of sets (if assoc == 0 then it's fully assoc so there is only 1 set)
	if(assocL1 != 0) numSetsL1 = totalSizeL1 * 1024 / (assocL1 * lineSizeL1);
	else {
		numSetsL1 = 1;
		assocL1 = totalSizeL1 * 1024 / lineSizeL1;
	}

	if(assocL2 != 0) numSetsL2 = totalSizeL2 * 1024 / (assocL2 * lineSizeL2);
	else {
		numSetsL2 = 1;
		assocL2 = totalSizeL2 * 1024 / lineSizeL2;
	}

	// D-cache is write through with no-write-alloc, LRU replacement
	Cache DCache(numSetsL1,assocL1,lineSizeL1,false,false,true);

	// L2 cache is writeback with write-alloc, LRU replacement
	Cache L2Cache(numSetsL2,assocL2,lineSizeL2,false,true,false);

	CPU cpu(argv[1]);

	Prefetcher pf;

	memQueue writeBuffer(10,&DCache,accessTimeL2,true,true,'a');
	memQueue queueL2(20,&DCache,accessTimeL2,true,false,'b');
	memQueue queueMem(10,&L2Cache,accessTimeMem,false,false,'c');


	// statistical stuff
	u_int32_t nRequestsL2 = 0; // number of requests sent out to L2 (both CPU and prefetcher requests)
	u_int32_t memCycles = 0; // number of cycles that main memory is being accessed
	u_int32_t memQsize = 0; // used for calculating average queue length

	u_int32_t curr_cycle = 1;
	Request req;
	bool isHit;

	while(!cpu.isDone()) {

		isHit = false;

		cpuState cpu_status = cpu.getStatus(curr_cycle);

//		printf("%u: %u\n",curr_cycle,cpu_status);

		if(cpu_status == READY) { // request is ready
				
			req = cpu.issueRequest(curr_cycle);

			// check for L1 hit
			isHit = DCache.check(req.addr,req.load);
			cpu.hitL1(isHit);
			req.HitL1 = isHit;

			// notify the prefetcher of what just happened with this memory op
			pf.cpuRequest(req);

			if(isHit) {
				DCache.access(req.addr,req.load);
				cpu.completeRequest(curr_cycle);
			}
			else if(req.load) {
				nRequestsL2++;
				if(queueL2.add(req,curr_cycle)) cpu.setStatus(WAITING); // CPU is now "waiting" for response from L2/mem
				else cpu.setStatus(STALLED_L2); // no room in l2 queue so we are "stalled" on this request
			}
			else { 
				nRequestsL2++;

				if (writeBuffer.add(req,curr_cycle)) cpu.completeRequest(curr_cycle); 
				else { // need to stall for an entry in the write buffer to open up
					cpu.setStatus(STALLED_WB);
				}
			}
		}
		// PF can do some work if we are just waiting or idle OR if we had a hit in the D-cache so the D-to-L2 bus isn't needed
		else if(cpu_status == WAITING || cpu_status == IDLE || cpu_status == STALLED_WB || isHit) { // either waiting for lower mem levels or idle so PF can do something
			if(pf.hasRequest(curr_cycle)) { 
				nRequestsL2++;

				req = pf.getRequest(curr_cycle);
				req.fromCPU = false;
				req.load = true;
				if(queueL2.add(req,curr_cycle)) pf.completeRequest(curr_cycle); // if added to queue then the request is "complete"
			}

			if(cpu_status == STALLED_WB) { // attempt to put it in the write buffer
				req = cpu.getRequest(); // get the request we want

				if (writeBuffer.add(req,curr_cycle)) cpu.completeRequest(curr_cycle); // if added, we can move on
			}
		}
		else if(cpu_status == STALLED_L2) { // stalled b/c of L2 queue so let us just try this right away
			req = cpu.getRequest();
			if(queueL2.add(req,curr_cycle)) cpu.setStatus(WAITING); // l2 queue is free now so we can go into waiting state
		}

		// service the L2 queue
		if(queueL2.frontReady(curr_cycle)) { // check to see if the front element in the queue is ready
			//printf("servicing the l2 queue on cycle %u\n",curr_cycle);
			req = queueL2.getFront();

			isHit = L2Cache.check(req.addr,req.load);
			cpu.loadHitL2(isHit);

			if(isHit) {
				DCache.access(req.addr,req.load); // update D cache
				if(req.fromCPU) cpu.completeRequest(curr_cycle); // this request was from the CPU so update state to show we are done
				queueL2.remove(); // remove this request from the queue
			}
			else {
				if(queueMem.add(req,curr_cycle)) queueL2.remove(); // succesfully added to memory queue so we can remove it from L2 queue
			}
		}

		// service the memory queue
		if(queueMem.frontReady(curr_cycle)) {
			//printf("servicing the mem queue on cycle %u\n",curr_cycle);
			req = queueMem.getFront();
			queueMem.remove();

			// update both L2 and D cache
			L2Cache.access(req.addr,req.load);
			if(req.load) DCache.access(req.addr,req.load); // only update if this is a load

			if(req.fromCPU && req.load) cpu.completeRequest(curr_cycle);
		}

		// check to see if we are utilizing memory BW during this cycle
		if(queueMem.getSize() > 0) memCycles++;

		// used to find the average size of the memory queue
		memQsize += queueMem.getSize();

		// service the write buffer
		if(writeBuffer.frontReady(curr_cycle)) {
			req = writeBuffer.getFront();

			isHit = L2Cache.check(req.addr,req.load);
			cpu.storeHitL2(isHit);

			if(isHit) { // store hit in L2 so just save it and we are done
				L2Cache.access(req.addr,req.load);
				writeBuffer.remove();
			}
			else { // L2 is write-allocate so we need to load data from memory first
				if(queueMem.add(req,curr_cycle)) writeBuffer.remove(); // we can keep adding to the queue because we check for duplicates as part of add()
			}
		}

		curr_cycle++; // next cycle
	}

	curr_cycle--; // just for stats sake

	double avgMemQ = (double)memQsize / (double)curr_cycle;
	double L2BW = (double)nRequestsL2 / (double)curr_cycle;
	double memBW = (double)memCycles / (double)curr_cycle;

	/*
	fprintf("total run time: %u\n",curr_cycle);
	fprintf("D-cache total hit rate: %f\n",cpu.getHitRateL1());
	fprintf("L2 cache total hit rate: %f\n",cpu.getHitRateL2());
	fprintf("AMAT: %f\n",cpu.getAMAT());
	fprintf("Average Memory Queue Size: %f\n",avgMemQ);
	fprintf("L2 BW Utilization: %f\n",L2BW);
	fprintf("Memory BW Utilization: %f\n",memBW);
	*/


	// create output file name based on trace file name
	char* outfile = (char *)malloc(sizeof(char)*(strlen(argv[1])+5));
	strcpy(outfile,argv[1]);
	strcat(outfile,".out");

	fp = fopen(outfile,"w"); // open outfile for writing

	free(outfile);

	fprintf(fp,"%u\n",curr_cycle);
	fprintf(fp,"%.4f\n",cpu.getHitRateL1());
	fprintf(fp,"%.4f\n",cpu.getHitRateL2());
	fprintf(fp,"%.4f\n",cpu.getAMAT());
	fprintf(fp,"%.4f\n",avgMemQ);
	fprintf(fp,"%.4f\n",L2BW);
	fprintf(fp,"%.4f\n",memBW);

	fclose(fp);

	return 0;
}

