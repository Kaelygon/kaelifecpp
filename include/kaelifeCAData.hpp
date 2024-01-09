#pragma once

#include "kaelRandom.hpp"
#include "kaelifeCALock.hpp"
#include "kaelifeWorldMatrix.hpp"
#include "kaelifeCAPreset.hpp"
#include "kaelifeCACache.hpp"

#include <iostream>
#include <cmath>
#include <numeric>
#include <functional>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <cstring>
#include <string.h>

#include <barrier>
#include <syncstream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <array>


//Cellular Automata Data
class CAData {
public:

	CALock kaeMutex;
	CAPreset kaePreset;
	CACache kaeCache;
	CACache::ThreadCache mainCache; //cache of CAData that should be used to update thread cache synchronously

	CAData() { // Constructor
		//TODO: better init config system
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
			stateBuf[j].resize(mainCache.tileRows);
			for (uint i = 0; i < mainCache.tileRows; i++) {
				stateBuf[j][i].resize(mainCache.tileCols);
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

private: //private vars and custom data types

	std::vector<const char*> backlogList; //tasks to do that are not thread safe

public: //public vars and custom data types
	//TODO: make a new configuration that CAData constructor reads and assigns correct starting variables
	//TODO: separate all drawing related stuff to a new class and pass stateBuf and some new config structure to it
		
	//Origin is at left bottom corner. X is horizontally right, Y is vertically up
	std::vector<std::vector<uint8_t>>stateBuf[2]; // double buffer

	//BOF vars that only CAData writes but others may read
		float targetFrameTime = 20.0; //target frame time
		uint slowFrameTime = 50; //target "lagging" frame time

		float aspectRatio;
		uint renderWidth;
		uint renderHeight;
	//EOF vars that CAData write

	//BOF vars that CAData+InputHandler write
		//drawn pixel buffer 
		typedef struct{
			uint16_t pos[2]; //list of coordinates to update {{123,23},...,{3,7}}
			uint8_t state;
		}drawBuffer;

		typedef struct{
			uint16_t pos[2];
			uint8_t radius;
		}drawMouseBuffer;
		
		std::vector<drawBuffer> drawBuf;
		std::vector<drawMouseBuffer> drawMouseBuf;
		std::mutex drawMutex;

	//EOF CAData+InputHandler
	

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

		//mainCache.neigMaskInd.clear();
		//mainCache.neigMaskInd.reserve( mainCache.maskElements );

		for (int i = 0; i < mainCache.maskWidth; ++i) {
			for (int j = 0; j < mainCache.maskHeight; ++j) {
				uint ind = i+j*mainCache.maskWidth;
				mainCache.neigMask1d[ind]=kaePreset.current()->neigMask[i][j]; //store 1d mask
				//if(mainCache.neigMask1d[ind]!=0){ mainCache.neigMaskInd.push_back(ind); } //store non-zero index
			}
		}

		mainCache.index++;
	}

	//execute before cloneBuffer not-thread safe functions backlog
	//writes must happen in !activeBuf
	void addBacklog(const char* keyword){
		//check if already backlogged
		auto it = std::find(backlogList.begin(), backlogList.end(), keyword);
		if(backlogList.end()==it){
			backlogList.push_back(keyword);
		}
	}
	
	void doBacklog(){
		if(backlogList.size()==0){ return; }

		//this flag is used to prevent cloning buffer every single keyword
		uint cloneBufferRequest=0;

		std::vector<const char *> backlogBuf=backlogList;
		auto keyword = backlogBuf.begin();

		while (keyword != backlogBuf.end()) {
			if (strcmp(*keyword, "cloneBuffer") == 0) {
				cloneBufferRequest=1;
			}else 
			if (strcmp(*keyword, "loadPreset") == 0) {
				loadPreset();
				kaePreset.printPreset();
				cloneBufferRequest=1;
			}else 
			if(strcmp(*keyword, "cursorDraw") == 0){
				if(drawMouseBuf.size()>0){ 
					copyDrawBuf(); 
					cloneBufferRequest=1;
				}//copy drawBuf 
			}else//randomizers
			if(strcmp(*keyword, "randAll") == 0){
				auto copyIndex = kaePreset.copyPreset((std::string)"RANDOM",kaePreset.index);
				kaePreset.setPreset(copyIndex[0]);
				uint64_t randSeed = kaePreset.randAll(copyIndex[0]);
				randState(kaePreset.current()->stateCount);
				loadPreset();
				kaePreset.printPreset();
				printf("RANDOM seed: %lu\n",(uint64_t)randSeed);
				cloneBufferRequest=1;
			}else 
			if(strcmp(*keyword, "randAdd") == 0){
				auto copyIndex = kaePreset.copyPreset((std::string)"RANDOM",kaePreset.index);
				kaePreset.setPreset(copyIndex[0]);
				int rr=std::min( (int)(kaePreset.current()->stateCount-1) ,127);
				kaePreset.randRuleAdd(kaePreset.index,-rr,rr);
				loadPreset();
				kaePreset.printRuleAdd(kaePreset.index);
				cloneBufferRequest=1;
			}else 
			if(strcmp(*keyword, "randRange") == 0){
				auto copyIndex = kaePreset.copyPreset((std::string)"RANDOM",kaePreset.index);
				kaePreset.setPreset(copyIndex[0]);
				kaePreset.randRuleRange(kaePreset.index,0);
				loadPreset();		
				kaePreset.printRuleRange(kaePreset.index);	
				cloneBufferRequest=1;
			}else 
			if(strcmp(*keyword, "randMask") == 0){
				auto copyIndex = kaePreset.copyPreset((std::string)"RANDOM",kaePreset.index);
				kaePreset.setPreset(copyIndex[0]);
				kaePreset.randRuleMask(kaePreset.index);
				loadPreset();
				kaePreset.printRuleMask(kaePreset.index);
				cloneBufferRequest=1;
			}else 
			if(strcmp(*keyword, "randMutate") == 0){
				auto copyIndex = kaePreset.copyPreset((std::string)"RANDOM",kaePreset.index);
				kaePreset.setPreset(copyIndex[0]);
				kaePreset.randRuleMutate(kaePreset.index);
				loadPreset();
				kaePreset.printRuleRange(kaePreset.index);
				kaePreset.printRuleAdd(kaePreset.index);
				cloneBufferRequest=1;
			}else{
				++keyword;
				continue;
			}
			keyword = backlogBuf.erase(keyword);
		}
		backlogList.clear();

		if(cloneBufferRequest){
			cloneBuffer();
			cloneBufferRequest=0;
		}
	}

	//assumes updatedCells[0] and updatedCells[1] are same size
	//Thread safe cloneBuffer
	void threadCloneBuffer(CACache::ThreadCache &lv) {

		//clone buffer for updated cells only
		for (size_t i = 0; i < lv.updatedCells[0].size(); ++i) {
			uint16_t x = lv.updatedCells[0][i];
			uint16_t y = lv.updatedCells[1][i];
			stateBuf[lv.activeBuf][x][y] = stateBuf[!lv.activeBuf][x][y];
		}

		lv.updatedCells[0].clear();
		lv.updatedCells[1].clear();

		//swap buffer index
		lv.activeBuf = !lv.activeBuf;
	}

	//not thread safe cloneBuffer
	void cloneBuffer(){
		//clone buffer
		stateBuf[mainCache.activeBuf] = stateBuf[!mainCache.activeBuf];

		//swap buffer index
		mainCache.activeBuf = !mainCache.activeBuf;
	}

	//Perform only when threads are paused
	void copyDrawBuf(){
		std::lock_guard<std::mutex> lock(drawMutex);//wait till drawing is done

    	for (const auto& pixel : drawBuf) {
			uint16_t x = pixel.pos[0]%mainCache.tileRows;
			uint16_t y = pixel.pos[1]%mainCache.tileCols;

			#if KAELIFE_DEBUG
				if(x>=mainCache.tileRows || y>=mainCache.tileCols){
					printf("OUT OF WORLD BOUNDS copyDrawBuf\n");
					abort();
				}	
			#endif

			stateBuf[!mainCache.activeBuf][x][y] = pixel.state;
		}
		drawMouseBuf.clear();
		drawMouseBuf.clear();
		drawBuf.clear();
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
			uint iterSize=(lv.tileRows+lv.threadCount/2)/lv.threadCount;
			uint remainder=lv.tileRows%lv.threadCount; //remaining rows that couldn't be split evenly
			uint iterStart	=	 lv.threadId*iterSize;
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
				
				for(uint i=0;i<localIterTask;i++){ //iterate the given amount 

					//iterate stripe of the world
					for (uint tx = iterStart; tx < iterEnd; tx++) {
						//check if tx is near border
						bool nearBorderX = (tx < lv.maskRadx) || (tx >= lv.tileRows - lv.maskRadx);
						for (uint ty = 0; ty < lv.tileCols; ++ty) {
							iterateCellLV(tx, ty, lv, nearBorderX);
						}
					}

					//Each thread has to be done before next iteration. Otherwise part of the world would simulate at different speed
					localBarrier.arrive_and_wait(); 
					threadCloneBuffer(lv);

				}
			}
		}

		//Cellular automata iteration logic using ThreadCache lv
		inline void iterateCellLV(const uint ti, const uint tj, CACache::ThreadCache &lv, bool nearBorder){

			int neigsum=0;
			int addValue=lv.ruleAdd.back();
			int cellState = stateBuf[lv.activeBuf][ti][tj];
			int ogState = cellState;

			//check if ti,tj is near border
			nearBorder = nearBorder || (tj < lv.maskRady) || (tj >= lv.tileCols - lv.maskRady );
			uint nx,ny;

			for(int i=0;i<lv.maskElements;++i){		
			//for (size_t i : lv.neigMaskInd) {		
				if(lv.neigMask1d[i]==0){continue;}

				//iterator to coordinate relative to mask center
				int x=i%lv.maskWidth-lv.maskRadx;
				int y=i/lv.maskWidth-lv.maskRady;

				//mask cell coordinate in world space, wrapped if out of bound
				nx = nearBorder ? (ti+x+lv.tileRows)%lv.tileRows : ti+x;
				ny = nearBorder ? (tj+y+lv.tileCols)%lv.tileCols : tj+y;
				#if KAELIFE_DEBUG
					if(nx>=lv.tileRows || ny>=lv.tileCols){
						printf("OUT OF WORLD BOUNDS iterateCellLV0\n");
						abort();
					}
					if(ti>=lv.tileRows || tj>=lv.tileCols){
						printf("OUT OF WORLD BOUNDS iterateCellLV1\n");
						abort();
					}			
				#endif

				uint neigValue=stateBuf[lv.activeBuf][nx][ny]; //get world cell value

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

			cellState += addValue;
			cellState = std::clamp(cellState, 0, (int)(lv.stateCount) - 1);
			if(ogState == cellState){return;}
			lv.updatedCells[0].push_back(ti);
			lv.updatedCells[1].push_back(tj);
			stateBuf[!lv.activeBuf][ti][tj] = cellState; //write to inactive buffer
		}
	//EOF iterate functions

	private:
	//BOF stateBuf functions
	
	//randomize state[!activeBuf][][]. Not thread safe
	void randState(uint numStates, uint64_t* seed=nullptr ){
		uint64_t* seedPtr = kaelRand.validSeedPtr(seed);
		for(uint i=0;i<mainCache.tileRows;i++){
			for(uint j=0;j<mainCache.tileCols;j++){
				stateBuf[!mainCache.activeBuf][i][j]=kaelRand(seedPtr)%numStates;
			}
		}
	}
	//EOF stateBuf functions
};