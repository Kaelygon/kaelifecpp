/**
 * @file kaelifeCALock.hpp
 * 
 * @brief CAData thread locks
*/

#pragma once

#include "kaelifeCAData.hpp"

#include <iostream>
#include <vector>
#include <chrono>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <numeric>

/**
 * @brief CAData and Main thread synchronization and locking
 * 
*/
class CALock {
private:
	std::vector<bool> resumeWaitPool;
	std::atomic<bool> allThreadsWaiting = false;

	uint threadCount;
	
	std::mutex resumeMutex;
	std::mutex mainMutex;
	std::condition_variable resumeCV;
	std::condition_variable mainCV;

    std::atomic<bool> isThreadPaused=false; //each thread has it's own memory location to prevent data racing without using per thread mutex
	std::vector<int> transferIterRepeats;
	std::atomic<bool> transferActiveBuf=0;

public:
    std::atomic<bool> isThreadTerminated=false;
	
    CALock() {
		threadCount=1;
		isThreadPaused=1; //start paused
		isThreadTerminated=0;
	}

	/**
	 * @brief Count how many threads are waiting
	*/
	uint getPoolSize(std::vector<bool> &pool){
		return std::accumulate(pool.begin(),pool.end(),0);
	}

	//
	/**
	 * @brief child threads waiting for main thread resume signal
	 * 
	 * @param threadId
	 * @param receiveIterRepeats //Thread unique cache tasks to do
	 * @param currentBuf //Thread unique cache activeBuf
	*/
	void waitResume(uint threadId, uint* receiveIterRepeats, bool* currentBuf) {
		{
			std::unique_lock<std::mutex> lock(resumeMutex);
			resumeWaitPool[threadId]=1;
			if(getPoolSize(resumeWaitPool)==threadCount){
				allThreadsWaiting.store(1);
				mainCV.notify_all();
			}
			resumeCV.wait(lock, [&] { return (transferIterRepeats[threadId] != -1); }); //threads enter pause
			resumeWaitPool[threadId]=0;
			allThreadsWaiting=0;
		}

		*receiveIterRepeats=transferIterRepeats[threadId]; 
		*currentBuf=transferActiveBuf.load();
		transferIterRepeats[threadId]=-1;
	}

	/**
	 * @brief main thread confirms that all threads are at waitResume()
	*/
	void syncMainThread(){
		std::unique_lock<std::mutex> lock(mainMutex);
		mainCV.wait(lock, [&] { return allThreadsWaiting.load(); }); //main waits threads to enter pause
	}

	/**
	 * @brief Tell threads it's safe to continue
	 * 
	 * @note call in main thread after syncMainThread in next cycle
	*/
    void continueThread(uint iters,uint activeBuf) {
		std::fill(transferIterRepeats.begin(),transferIterRepeats.end(),iters); 
		transferActiveBuf=activeBuf;
		resumeCV.notify_all();
	}

	/**
	 * @brief Signal to threads it's time to stop
	*/
    void terminateThread() {
		isThreadTerminated.store(true);
		continueThread(1,0); //threads are paused until iterCount>0
    }

	/**
	 * @brief Set number of expected thread count
	 * 
	 * @note important to be same as CAData startWorkerThreads thread count
	*/
	void expectedThreadCount(uint tc){
		threadCount=tc;
		transferIterRepeats.resize(tc,-1);
		resumeWaitPool.resize(tc,0);
	}
		
};