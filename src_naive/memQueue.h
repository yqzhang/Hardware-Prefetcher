#ifndef MEMQUEUE_H
#define MEMQUEUE_H

#include <sys/types.h>

class Cache;
struct Request;

// implemented as circular queue
class memQueue {
  private:
	char _id;
	u_int32_t _capacity;
	u_int32_t _latency;
	bool _write; // true = write buffer, false = read queue
	bool _pipelined; // true means that multiple requests are handled at once, false means one at a time
	Cache *_source; // the cache where we are receiving requests from; we need this for checking duplicate entries

	u_int32_t _front;
	u_int32_t _rear;
	u_int32_t _size;

	Request* _queue;
	u_int32_t* _readyTime; // the time when this entry will be ready
	u_int32_t* _tags; // the time when this entry will be ready
	u_int32_t* _indexes; // the time when this entry will be ready
	// looks for a duplicate request in the queue... returns the queue location of the dup if there is one, -1 otherwise
	int findDup(u_int32_t tag, u_int32_t index);

  public:

	memQueue(u_int32_t capacity, Cache *c, u_int32_t latency, bool pipelined, bool write, char name);

	// free up queue and readyTime arrays after we deconstruct
	~memQueue();

	// prints the current queue status (addr and time when ready)
  	void printQueue();

	// return false if queue is full, also check to see if this is a duplicate request... if so, ignore it (but still return true)
	bool add(Request req, u_int32_t cycle);

	// return false if it was empty (i.e. nothing to remove)
	bool remove();

	u_int32_t getSize();

	// check whether the front element is ready to go
	bool frontReady(u_int32_t cycle);

	// return front element in queue, don't remove it though
	Request getFront();
};

#endif
