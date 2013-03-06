#include "memQueue.h"
#include "mem-sim.h"
#include "cache.h"
#include <stdlib.h>
#include <stdio.h>

void memQueue::printQueue() {
	int i = _front;

	while(i != _rear) {
		printf("addr: %x, ready at: %u\n",_queue[i].addr,_readyTime[i]);

		i++;
		if(i == _capacity) i = 0;
	}
}

int memQueue::findDup(u_int32_t tag, u_int32_t index) {
	int dupLoc = -1;
	int i = _front;

	while(i != _rear) {
		if(_tags[i] == tag && _indexes[i] == index) dupLoc = i;

		i++;
		if(i == _capacity) i = 0;
	}

	return dupLoc;
}

memQueue::memQueue(u_int32_t capacity, Cache *c, u_int32_t latency, bool pipelined, bool write, char name) { 
	_id = name;
	_capacity = capacity; 
	_source = c;
	_latency = latency;
	_pipelined = pipelined;
	_write = write; 

	_front = 0;
	_rear = 0;
	_size = 0;

	_queue = (Request*)calloc(_capacity, sizeof(Request));
	_readyTime = (u_int32_t*)calloc(_capacity, sizeof(u_int32_t));
	_tags = (u_int32_t*)calloc(_capacity, sizeof(u_int32_t));
	_indexes = (u_int32_t*)calloc(_capacity, sizeof(u_int32_t));
	
}

memQueue::~memQueue() { 
	free(_queue); 
	free(_readyTime); 
	free(_tags); 
	free(_indexes); 
}

bool memQueue::add(Request req, u_int32_t cycle) {
	bool success = true;
	u_int32_t tag, index;

	if(_size == _capacity) success = false;
	else {
		// first check to see if this is a duplicate
		index = _source->getIndex(req.addr);
		tag = _source->getTag(req.addr);

		int dupLoc = findDup(tag,index); // see if there is a duplicate request (-1 = not found)

		if(dupLoc == -1||_queue[dupLoc].fromCPU) { // no duplicate found
			_queue[_rear] = req;
			_tags[_rear] = tag;
			_indexes[_rear] = index;

			// pipelined will be ready _latency cycles after entering the queue
			if(_pipelined || _size == 0) _readyTime[_rear] = cycle + _latency;

			else { // will be ready _latency cycles after the last item in the queue
				int i = _rear - 1;
				if(i < 0) i = _capacity - 1;

				_readyTime[_rear] = _latency + _readyTime[i];
			}

			//printf("adding to %c: readyTime is %u\n",_id,_readyTime[_rear]);

			_size++; // increase size
			_rear++; // move rear
			if(_rear == _capacity) _rear = 0;
		}
		else if(req.fromCPU) { // if our new request is from the CPU, we want to replace the old duplicate
			_queue[dupLoc] = req;
		}
	}

	return success;
}

bool memQueue::remove() {
	bool success = true;

	if(_size == 0) success = false;
	else {
		_size--; // decrease size

		_front++; // move front 
		if(_front == _capacity) _front = 0;
	}

	return success;
}

u_int32_t memQueue::getSize() { return _size; }

bool memQueue::frontReady(u_int32_t cycle) { 
	if(_size > 0 && _readyTime[_front] <= cycle) return true;
	else return false;
}

Request memQueue::getFront() { return _queue[_front]; }
