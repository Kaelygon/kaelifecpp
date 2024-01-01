#pragma once

#include "./kaelRandom.hpp"
#include "kaelifeCALock.hpp"
#include "./kaelifeWorldMatrix.hpp"
#include "./kaelifeCAPreset.hpp"
#include "./kaelifeCACache.hpp"

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

#include <barrier>
#include <syncstream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <array>


//Cellular Automata Data
class CAData {
public:

	CAPreset kaePreset;
	CACache kaeCache;
	CACache::ThreadCache mainCache; //cache of CAData that should be used to update thread cache synchronously

	CAData() : neigMask{{}, {}} { // Constructor

		mainCache.threadId		=	(uint)-1; //only threads use this
		mainCache.threadCount	=	std::thread::hardware_concurrency();
		mainCache.activeBuf		=	0;
		mainCache.tileRows		=	576;
		mainCache.tileCols		=	384;

		aspectRatio=(float)mainCache.tileRows/mainCache.tileCols;
		renderWidth = mainCache.tileRows>1024 ? mainCache.tileRows : 1024 ;
		renderHeight = renderWidth/aspectRatio;

		stateBuf[!mainCache.activeBuf].resize(mainCache.tileRows);
		stateBuf[ mainCache.activeBuf].resize(mainCache.tileRows);
		for(uint i=0;i<mainCache.tileRows;i++){
			stateBuf[!mainCache.activeBuf][i].resize (mainCache.tileCols);
			stateBuf[ mainCache.activeBuf][i].reserve(mainCache.tileCols);
		}

		loadPreset(!mainCache.activeBuf); //initialize rest mainCache variables
		cloneBuffer(); 
		kaePreset.printPreset(0);

		targetFrameTime= targetFrameTime<=0.0 ? 0.000001 : targetFrameTime;

		CAPreset::PresetList bufPreset = {
			.name = "RANDOM",
		};
		//uint64_t bufSeed=reinterpret_cast<uint64_t>(bufPreset.name);
		uint64_t bufSeed = 3317025035631099721;
		uint randIndex = kaePreset.addPreset(bufPreset);
		kaePreset.randAll(randIndex,&bufSeed);

		printf("cache size: %lu\n",sizeof(mainCache));
	}

private: //private vars and custom data types

	std::vector<const char*> backlogList; //tasks to do that are not thread safe	

	WorldMatrix<uint8_t> neigMask[2]; //buffered to prevent incomplete read in iterateCell

public: //public vars and custom data types
	//TODO: make a new configuration that CAData constructor reads and assigns correct starting variables
		
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
		//TODO: separate all drawing related stuff to a new class and pass stateBuf and some new config structure to it
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

		neigMask[bufIndex] = kaePreset.current()->neigMask;
		if(kaePreset.current()->stateCount==0){
			printf(" ");
		}

		mainCache.ruleRange		=	kaePreset.current()->ruleRange;
		mainCache.ruleAdd		=	kaePreset.current()->ruleAdd;
		mainCache.stateCount	=	kaePreset.current()->stateCount==0 ? 1 : kaePreset.current()->stateCount; //a lot of arithmetics break with stateCount 0
		mainCache.maskWidth		=	kaePreset.current()->neigMask.matrix   .size(); 	//full mask dimensions
		mainCache.maskHeight	=	kaePreset.current()->neigMask.matrix[0].size();
		mainCache.maskElements	=	mainCache.maskWidth*mainCache.maskHeight;
		mainCache.maskRadx		=	(mainCache.maskWidth )/2; 	//distance from square center. Center included 
		mainCache.maskRady		=	(mainCache.maskHeight)/2;
		mainCache.clipTreshold	=	kaePreset.current()->stateCount/2;

		mainCache.neigMask1d.clear();
		mainCache.neigMask1d.resize( mainCache.maskElements );

		//mainCache.neigMaskInd.clear();
		//mainCache.neigMaskInd.reserve( mainCache.maskElements );

		for (int i = 0; i < mainCache.maskWidth; ++i) {
			for (int j = 0; j < mainCache.maskHeight; ++j) {
				uint ind = i+j*mainCache.maskWidth;
				mainCache.neigMask1d[ind]=neigMask[bufIndex].matrix[i][j]; //store 1d mask
				//if(mainCache.neigMask1d[ind]!=0){ mainCache.neigMaskInd.push_back(ind); } //store non-zero index
			}
		}

		mainCache.index++;
	}

	//Perform only when threads are paused
	void copyDrawBuf(){
		std::lock_guard<std::mutex> lock(drawMutex);//wait till drawing is done

    	for (const auto& pixel : drawBuf) {
			uint16_t x = pixel.pos[0]%mainCache.tileRows;
			uint16_t y = pixel.pos[1]%mainCache.tileCols;

			stateBuf[!mainCache.activeBuf][x][y] = pixel.state;
		}
		drawMouseBuf.clear();
		drawMouseBuf.clear();
		drawBuf.clear();
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
				auto copyIndex = kaePreset.copyPreset("RANDOM",kaePreset.index);
				kaePreset.setPreset(copyIndex[0]);
				uint64_t randSeed = kaePreset.randAll(copyIndex[0]);
				randState(kaePreset.current()->stateCount);
				loadPreset();
				kaePreset.printPreset();
				printf("RANDOM seed: %lu\n",(uint64_t)randSeed);
				cloneBufferRequest=1;
			}else 
			if(strcmp(*keyword, "randAdd") == 0){
				auto copyIndex = kaePreset.copyPreset("RANDOM",kaePreset.index);
				kaePreset.setPreset(copyIndex[0]);
				int rr=std::min( (int)ceil((kaePreset.current()->stateCount-1)) ,127);
				kaePreset.randRuleAdd(kaePreset.index,-rr,rr);
				loadPreset();
				kaePreset.printRuleAdd(kaePreset.index);
				cloneBufferRequest=1;
			}else 
			if(strcmp(*keyword, "randRange") == 0){
				auto copyIndex = kaePreset.copyPreset("RANDOM",kaePreset.index);
				kaePreset.setPreset(copyIndex[0]);
				kaePreset.randRuleRange(kaePreset.index,0);
				loadPreset();		
				kaePreset.printRuleRange(kaePreset.index);	
				cloneBufferRequest=1;
			}else 
			if(strcmp(*keyword, "randMask") == 0){
				auto copyIndex = kaePreset.copyPreset("RANDOM",kaePreset.index);
				kaePreset.setPreset(copyIndex[0]);
				kaePreset.randRuleMask(kaePreset.index);
				loadPreset();
				kaePreset.printRuleMask(kaePreset.index);
				cloneBufferRequest=1;
			}else 
			if(strcmp(*keyword, "randMutate") == 0){
				auto copyIndex = kaePreset.copyPreset("RANDOM",kaePreset.index);
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
		neigMask[mainCache.activeBuf] = neigMask[!mainCache.activeBuf];

		//swap buffer index
		mainCache.activeBuf = !mainCache.activeBuf;
	}


	//BOF iterate functions
	public:
		//progress state[][] by 1 step multi threaded
		inline void iterateStateMT(std::vector<std::thread> &threads, CALock *iterLock) {

			iterLock->resizeThread(mainCache.threadCount);
			std::barrier localBarrier(mainCache.threadCount);
			CACache::ThreadCache cache = mainCache;
			kaeCache.copyCache(&cache, mainCache);

			for (uint i = 0; i < mainCache.threadCount; ++i) {
				cache.threadId=i;

				threads.emplace_back([&, iterLock, cache]() {
					iterateState(&(*iterLock), cache, localBarrier);
				});
			}
			for (auto &thread : threads){
				if (thread.joinable()){
					thread.join();
				}
			}
		}

	private:
		inline void iterateState(CALock *iterLock, CACache::ThreadCache lv, std::barrier<>& localBarrier) {

			uint localIterTask=0;
			uint stripeSize=ceil(lv.tileCols/lv.threadCount);

			while(1){
				localIterTask=0;

				if(lv.index!=mainCache.index){
					kaeCache.copyCache(&lv, mainCache);
				}
				iterLock->waitResume(lv.threadId,&localIterTask,&lv.activeBuf); //wait main thread resume signal
				
				if(iterLock->isThreadTerminated.load()){
					return;
				}
				
				for(uint i=0;i<localIterTask;i++){ //iterate the given amount 

					for (uint tx = 0; tx < lv.tileRows; ++tx) {
						bool nearBorderX = (tx < lv.maskRadx) || (tx >= lv.tileRows - lv.maskRadx);
						for (uint ty = lv.threadId*stripeSize; ty < (lv.threadId+1)*stripeSize; ty += 1) {
							iterateCellLV(tx, ty, lv, nearBorderX);
						}
					}

					localBarrier.arrive_and_wait(); //Make sure no threads are writing
					threadCloneBuffer(lv);

				}
			}
		}

		//Cellular automata iteration logic local variables
		inline void iterateCellLV(const uint ti, const uint tj, CACache::ThreadCache &lv, bool nearBorder){

			int neigsum=0;
			int addValue=lv.ruleAdd.back();
			int cellState = stateBuf[lv.activeBuf][ti][tj];
			int ogState = cellState;

			//check if near border
			nearBorder = nearBorder || (tj < lv.maskRady) || (tj >= lv.tileCols - lv.maskRady );
			uint nx,ny;


			for(int i=0;i<lv.maskElements;++i){		
			//for (size_t i : lv.neigMaskInd) {		
				if(lv.neigMask1d[i]==0){continue;}

				int x=i%lv.maskWidth;
				x=x-lv.maskRadx; // map coords e.g. [0,5] to [-2,2] 
				int y=i/lv.maskWidth-lv.maskRady;

				//mask cells in world space, wrapped if out of bound
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

				uint neigValue=stateBuf[lv.activeBuf][nx][ny];

				if(neigValue==0){continue;}
				
				neigValue*=(neigValue>=lv.clipTreshold); //clip treshold stateCount/2
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

	//randomize state[!activeBuf][][]. Not thread safe
	void randState(uint numStates, uint64_t* seed=nullptr ){
		uint64_t* seedPtr = kaelRand.getSeedPtr(seed);
		for(uint i=0;i<mainCache.tileRows;i++){
			for(uint j=0;j<mainCache.tileCols;j++){
				stateBuf[!mainCache.activeBuf][i][j]=kaelRand(seedPtr)%numStates;
			}
		}
	}

};