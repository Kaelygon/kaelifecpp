/**
 * @file kaelifeCAData.hpp
 * @brief Manages and iterates cellState that holds CA cell states
 * 
 * CAData Holds double buffered list of every world cell states "cellState[Active Buffer][Tile Rows][Tile Cols]"
 * This class initializes threads that run the algorithms to iterate cellState  
 * 
 * This class executes cellState tasks in this chronological order
 * 1. startWorkerThreads starts the worker threads and joins then upon exit. Each thread cache is initialized with default mainCache.
 * 2. Each thread runs a loop in iterateWorld and has a unique stripe of cellState so (ideally) no data race is possible
 * 3. Each thread waits for CAData.CALock.continueThread([TASK SIZE],[ACTIVE BUFFER]) command and iterates number of [TASK SIZE] times. 
 * 4. When all threads are done, they wait for next "continueThread" call, or until "terminateThread" is called
 * 
 * Main thread and classes like InputHandler may add not-thread-safe tasks to backlog
 * Main thread may call backlog.doBacklog() which executes queued tasks
 * 
*/

#pragma once

#include "kaelife.hpp"
#include "kaelRandom.hpp"
#include "kaelifeCALock.hpp"
#include "kaelifeWorldMatrix.hpp"
#include "kaelifeCAPreset.hpp"
#include "kaelifeCACache.hpp"
#include "kaelifeCADraw.hpp"

#include <iostream>
#include <cmath>
#include <numeric>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <cstring>
#include <string.h>
#include <array>
#include <memory>
#include <barrier>
#include <syncstream>
#include <thread>
#include <mutex>
#include <condition_variable>

class CABacklog; // Forward declaration

/**
 * @brief Cellular Automata Data. Manages and iterates cellState that holds CA cell states
 * 
 */
class CAData {
public:
	
	/** @brief Thread locking*/
	CALock kaeMutex; 
	/** @brief Automata configurations*/
	CAPreset kaePreset; 
	/** @brief Unique thread chaches*/
	CACache kaeCache; 
	/** @brief cache of CAData that should be used to update thread cache synchronously*/
	CACache::ThreadCache mainCache; 
	/** @brief InputHandler drawn pixels*/
	CADraw kaeDraw; 
	/** @brief Not thread safe task queue*/
    std::unique_ptr<CABacklog> backlog; 

	CAData();

public: //public vars and custom data types

	//Doxygen doesn't understand __aligned__
    /**
     * @brief Holds 2D cellular automata states. Double buffered.
	 * 
     * Writes must be done to [Active Buffer] !mainCache.activeBuf,
     * and reads must be done from mainCache.activeBuf. X is left to right.
     * Y is down to up (Row major). cellState[Active Buffer][X][Y]
     *
     */
    __attribute__((__aligned__(64))) std::vector<std::vector<uint8_t>> cellState[2];


	//BOF vars that only CAData writes but others may read
		float targetFrameTime = 20.0; //target frame time
		uint slowFrameTime = 50; //target "lagging" frame time

		float aspectRatio;
		uint renderWidth;
		uint renderHeight;
	//EOF vars that CAData write
	

public: //public functions

	/**
	 * @brief Load current CAPreset to mainCache. Not thread safe
	 * 
	*/
	void loadPreset(){

		mainCache.ruleRange		=	kaePreset.current()->ruleRange;
		mainCache.ruleAdd		=	kaePreset.current()->ruleAdd;
		mainCache.stateCount	=	kaePreset.current()->stateCount==0 ? 1 : kaePreset.current()->stateCount; //a lot of arithmetics break with stateCount 0
		mainCache.maskWidth		=	kaePreset.current()->neigMask.getWidth(); 	//full mask dimensions
		mainCache.maskHeight	=	kaePreset.current()->neigMask.getHeight();
		mainCache.maskElements	=	mainCache.maskWidth*mainCache.maskHeight;
		mainCache.maskRadx		=	(mainCache.maskWidth )/2; 	//distance from square center. Center included 
		mainCache.maskRady		=	(mainCache.maskHeight)/2;
		mainCache.clipTreshold	=	kaePreset.current()->clipTreshold;

		mainCache.neigMask1d.clear();
		if(mainCache.maskElements!=0){
			mainCache.neigMask1d.resize( mainCache.maskElements );
		}

		for (int i = 0; i < mainCache.maskWidth; ++i) {
			for (int j = 0; j < mainCache.maskHeight; ++j) {
				uint ind = i+j*mainCache.maskWidth;
				mainCache.neigMask1d[ind]=kaePreset.current()->neigMask[i][j]; //store 1d mask
			}
		}

		mainCache.index++;
	}

	/**
	 * @brief Multithreaded cloneBuffer
	 * 
	 * @param lv unique thread cache
	 * 
	 * Each thread clones stripes of cellState to activeBuf and local thread activeBuf is swapped
	*/
	inline void threadCloneBuffer(CACache::ThreadCache &lv) {

		//clone buffer for updated cells only
		for (size_t i = 0; i < lv.updatedCells[0].size(); ++i) {
			uint16_t x = lv.updatedCells[0][i];
			uint16_t y = lv.updatedCells[1][i];
			cellState[lv.activeBuf][x][y] = cellState[!lv.activeBuf][x][y];
		}

		lv.updatedCells[0].clear();
		lv.updatedCells[1].clear();

		//swap buffer index
		lv.activeBuf = !lv.activeBuf;
	}

	/**
	 * @brief Single threaded clone buffer
	*/
	void cloneBuffer(){
		//clone buffer
		cellState[mainCache.activeBuf] = cellState[!mainCache.activeBuf];

		//swap buffer index
		mainCache.activeBuf = !mainCache.activeBuf;
	}



	//BOF iterate functions
	public:
		/**
		 * @brief Start worker threads and wait them to join
		 * 
		 * Each creates a unique copy of mainCache 
		*/
		inline void startWorkerThreads(std::vector<std::thread> &threads) {

			kaeMutex.expectedThreadCount(mainCache.threadCount);
			std::barrier localBarrier(mainCache.threadCount);
			CACache::ThreadCache cache = mainCache;
			kaeCache.copyCache(&cache, mainCache);

			for (uint i = 0; i < mainCache.threadCount; ++i) {
				cache.threadId=i;

				threads.emplace_back([&, cache]() {
					iterateWorld(cache, localBarrier);
				});
			}
			for (auto &thread : threads){
				if (thread.joinable()){
					thread.join();
				}
			}
		}

	private:
		/**
		 * @brief Iterate every cellState. Multithreaded
		 * 
		 * Each thread has unique cache "lv" which potentially improves cpu caching performance
		 * Each thread computes a unique stripe of cellState that (ideally) no data race is possible
		 * 
		 * @param lv Unique thread cache
		 * @param localBarrier barrier to synchronize critical parts 
		*/
		inline void iterateWorld(CACache::ThreadCache lv, std::barrier<>& localBarrier) {
			uint localIterTask=0;

			//spread threads task to 2D stripes 
			uint iterSize	=(lv.tileRows+lv.threadCount/2)/lv.threadCount;
			uint remainder	= lv.tileRows%lv.threadCount; //remaining rows that couldn't be split evenly
			uint iterStart	= lv.threadId*iterSize;
			uint iterEnd	=(lv.threadId+1)*iterSize;
			iterStart+=remainder    *(lv.threadId)/lv.threadCount; //spread out by remainder
			iterEnd  +=(remainder)*(lv.threadId+1)/lv.threadCount; //fill gaps with remaining rows
				
			iterEnd = iterEnd > lv.tileRows ? lv.tileRows : iterEnd;

			while(1){
				localIterTask=0;

				if(lv.index!=mainCache.index){
					kaeCache.copyCache(&lv, mainCache);
				}
				kaeMutex.waitResume(lv.threadId,&localIterTask,&lv.activeBuf); //wait main thread resume signal
				
				if(kaeMutex.isThreadTerminated.load()){
					return;
				}
				
				for(size_t i=0;i<localIterTask;i++){ //iterate the given amount 

					//iterate stripe of the world
					for (size_t tx = iterStart; tx < iterEnd; tx++) {
						//check if tx is near border
						bool nearBorderX = (tx < lv.maskRadx) || (tx >= lv.tileRows - lv.maskRadx);
						for (size_t ty = 0; ty < lv.tileCols; ++ty) {
							iterateCellLV(tx, ty, lv, nearBorderX);
						}
					}

					//Each thread has to be done before next iteration. Otherwise part of the world would simulate at different speed
					localBarrier.arrive_and_wait(); 
					threadCloneBuffer(lv);

				}
				//Ensure very slow threads catch up before entering waitResume()
				localBarrier.arrive_and_wait(); 
			}
		}

		//Cellular automata iteration logic using ThreadCache lv
		/**
		 * @brief Iterate single cellState[Active Buf][ti][tj]
		 * 
		 * @param ti Row
		 * @param tj Column
		 * @param nearBorder Is cellState[lv.activeBuf][ti][tj] closer than maskRad from world border
		*/
		inline void iterateCellLV(const uint ti, const uint tj, CACache::ThreadCache &lv, bool nearBorder){

			int neigsum=0;
			int addValue=lv.ruleAdd.back();
			int currentCellState = cellState[lv.activeBuf][ti][tj]; //current cell value
			int ogState = currentCellState;

			//check if ti,tj is near border
			nearBorder = nearBorder || (tj < lv.maskRady) || (tj >= lv.tileCols - lv.maskRady );
			uint nx,ny;

			for(int i=0;i<lv.maskElements;++i){		
				if(lv.neigMask1d[i]==0){continue;}

				//iterator to coordinate relative to mask center
				int x=i%lv.maskWidth-lv.maskRadx;
				int y=i/lv.maskWidth-lv.maskRady;

				//mask cell coordinate in world space, wrapped if out of bound
				nx = nearBorder ? (ti+x+lv.tileRows)%lv.tileRows : ti+x;
				ny = nearBorder ? (tj+y+lv.tileCols)%lv.tileCols : tj+y;
				if(kaelife::CA_DEBUG){
					if(nx>=lv.tileRows || ny>=lv.tileCols){
						printf("OUT OF WORLD BOUNDS iterateCellLV0\n");
						abort();
					}
					if(ti>=lv.tileRows || tj>=lv.tileCols){
						printf("OUT OF WORLD BOUNDS iterateCellLV1\n");
						abort();
					}			
				}

				uint neigValue=cellState[lv.activeBuf][nx][ny]; //get world cell value

				if(neigValue < lv.clipTreshold){continue;} //clip any values below clipTreshold
				
				neigValue=neigValue*lv.neigMask1d[i]/UINT8_MAX; //weight
				neigsum+=neigValue;
			}

			//linear search which range neigsum lands on
			for(size_t i=0;i<lv.ruleRange.size();i++){
				if(neigsum<lv.ruleRange[i]){
					addValue=lv.ruleAdd[i];
					break;
				}
			}

			currentCellState += addValue;
			currentCellState = std::clamp(currentCellState, 0, (int)(lv.stateCount) - 1);
			if(ogState == currentCellState){return;}
			lv.updatedCells[0].push_back(ti);
			lv.updatedCells[1].push_back(tj);
			cellState[!lv.activeBuf][ti][tj] = currentCellState; //write to inactive buffer
		}
	//EOF iterate functions

	public:
	//BOF cellState functions
	
	//randomize state[!activeBuf][][]. Not thread safe
	/**
	 * @param numStates number of possible automata states
	 * @param seed randomizer seed. If no seed is given, use kaelife::rand() instance seed
	*/
	void randState(uint numStates, uint64_t* seed=nullptr ){
		uint64_t* seedPtr = kaelife::rand.validSeedPtr(seed);
		for(uint i=0;i<mainCache.tileRows;i++){
			for(uint j=0;j<mainCache.tileCols;j++){
				cellState[!mainCache.activeBuf][i][j]=kaelife::rand(seedPtr)%numStates;
			}
		}
	}
	//EOF cellState functions
};

//include here to prevent circular dependency
#include "kaelifeCABacklog.hpp"

/**
 * @brief CAData constructor and initialization
*/
CAData::CAData() {
    backlog = std::make_unique<CABacklog>(*this);

	mainCache.threadId		=	UINT_MAX; //only threads use this
	mainCache.activeBuf		=	0;
	mainCache.tileRows		=	576; 
	mainCache.tileCols		=	384;
	mainCache.threadCount	=	std::thread::hardware_concurrency();
	if(mainCache.threadCount>mainCache.tileCols){mainCache.threadCount=mainCache.tileCols;}

	aspectRatio=(float)mainCache.tileRows/mainCache.tileCols;
	if(true){
		renderWidth = mainCache.tileRows*2 ;
		renderHeight = mainCache.tileCols*2;
	}else{
		renderWidth = mainCache.tileRows>1024 ? mainCache.tileRows : 1024 ;
		renderHeight = renderWidth/aspectRatio;
	}

	for (int j = 0; j < 2; j++) {
		cellState[j].resize(mainCache.tileRows);
		for (uint i = 0; i < mainCache.tileRows; i++) {
			cellState[j][i].resize(mainCache.tileCols);
		}
	}

	targetFrameTime= targetFrameTime<=0.0 ? 0.000001 : targetFrameTime;

	CAPreset::RulePreset bufPreset("RANDOM");
	uint randIndex = kaePreset.addPreset(bufPreset);
	kaePreset.seedFromName(randIndex);

	if(kaelife::CA_DEBUG){
		printf("cache size: %lu\n",sizeof(mainCache));
	}

	loadPreset(); //initialize rest mainCache variables
	cloneBuffer(); 
	kaePreset.printPreset(0);
}