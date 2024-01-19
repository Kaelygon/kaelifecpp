//kaelifeCAData.hpp
//Manages and iterates cellState that holds CA cell states

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

//Cellular Automata Data
class CAData {
public:

	CALock kaeMutex;
	CAPreset kaePreset;
	CACache kaeCache;
	CACache::ThreadCache mainCache; //cache of CAData that should be used to update thread cache synchronously
	CADraw kaeDraw;
    std::unique_ptr<CABacklog> backlog;

	CAData();

public: //public vars and custom data types

	//Holds 2D cellular automata states. Double buffered. 
	//Writes must be done to [Buffer Index] !mainCache.activeBuf and reads must be done from mainCache.activeBuf
	//X is left to right. Y is down to up
	//cellState[Buffer Index][X][Y]
	__attribute__((aligned(64))) std::vector<std::vector<uint8_t>>cellState[2]; // double buffer

	//BOF vars that only CAData writes but others may read
		float targetFrameTime = 20.0; //target frame time
		uint slowFrameTime = 50; //target "lagging" frame time

		float aspectRatio;
		uint renderWidth;
		uint renderHeight;
	//EOF vars that CAData write
	

public: //public functions

	//Load current CAPreset, not thread safe
	//
	void loadPreset(int bufIndex=-1){
		bufIndex = bufIndex==-1 ? !mainCache.activeBuf : bufIndex;

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

	//assumes updatedCells[0] and updatedCells[1] are same size
	//Thread safe cloneBuffer
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

	//not thread safe cloneBuffer
	void cloneBuffer(){
		//clone buffer
		cellState[mainCache.activeBuf] = cellState[!mainCache.activeBuf];

		//swap buffer index
		mainCache.activeBuf = !mainCache.activeBuf;
	}



	//BOF iterate functions
	public:
		//progress state[][] by 1 step multi threaded
		inline void iterateStateMT(std::vector<std::thread> &threads) {

			kaeMutex.resizeThread(mainCache.threadCount);
			std::barrier localBarrier(mainCache.threadCount);
			CACache::ThreadCache cache = mainCache;
			kaeCache.copyCache(&cache, mainCache);

			for (uint i = 0; i < mainCache.threadCount; ++i) {
				cache.threadId=i;

				threads.emplace_back([&, cache]() {
					iterateState(cache, localBarrier);
				});
			}
			for (auto &thread : threads){
				if (thread.joinable()){
					thread.join();
				}
			}
		}

	private:
		inline void iterateState(CACache::ThreadCache lv, std::barrier<>& localBarrier) {
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
				if(KAELIFE_DEBUG){
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

#include "kaelifeCABacklog.hpp"

CAData::CAData() {
    backlog = std::make_unique<CABacklog>(*this);

	mainCache.threadId		=	UINT_MAX; //only threads use this
	mainCache.activeBuf		=	0;
	mainCache.tileRows		=	32; 
	mainCache.tileCols		=	32;
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

	CAPreset::PresetList bufPreset("RANDOM");
	uint randIndex = kaePreset.addPreset(bufPreset);
	kaePreset.seedFromName(randIndex);

	printf("cache size: %lu\n",sizeof(mainCache));

	loadPreset(!mainCache.activeBuf); //initialize rest mainCache variables
	cloneBuffer(); 
	kaePreset.printPreset(0);
}