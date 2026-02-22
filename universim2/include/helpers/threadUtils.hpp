/*	
 *	This file has helper functions on how many threads can be used for which purpose
 */

#include <algorithm>
#include <iostream>
#include <thread>

#define INCREASE_THREAD_COUNT 1
#define DECREASE_THREAD_COUNT -1

static unsigned int threadCount() {
	const unsigned int threadCount = std::thread::hardware_concurrency();
    //if (threadCount == 0) {
    //    std::cout << "CPU thread count not detectable\n";
    //} else {
    //    std::cout << "CPU supports " << threadCount << " concurrent threads\n";
    //}
	return threadCount;	
}

// For the creation of the universe, we use all threads but one
static int getThreadCountCreate() {
	return std::max(threadCount()-1, (unsigned int) 1);
}

static int getThreadCountLocalUpdate() {
	switch(threadCount()) {
		case 1: return(1);
		case 4: return(1);
		case 16: return(6);
		default: return(1);
	}
}

static int getThreadCountStellarUpdate() {
	switch(threadCount()) {
		case 1: return(1);
		case 4: return(1);
		case 16: return(1);
		default: return(1);
	}
}

static int getMaxThreadCountRenderer() {
	switch(threadCount()) {
		case 1: return(1);
		case 4: return(2);
		case 16: return(8);
		default: return(1);
	}
}
