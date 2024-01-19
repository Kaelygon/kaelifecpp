//TODO: JSON Config parser and MasterConfig struct in ConfigHandler class

#pragma once

#include "kaelife.hpp"
#include "kaelifeCAData.hpp" 
#include "kaelRandom.hpp"
#include "kaelifeCACache.hpp"

#include <SDL2/SDL.h>
#include <map>
#include <functional>
#include <algorithm>
#include <cmath>
#include <atomic>

#include <thread>
#include <stdint.h>
#include <mutex>


class CADraw{
private:

	//pixels to be cloned to cellState
	struct drawnPixel{
		uint16_t pos[2]; //list of coordinates to update {{123,23},...,{3,7}}
		uint8_t state;
	};

	//center of drawn circles
	struct drawnPoint{
		uint16_t pos[2];
		uint8_t radius;
	};

	struct drawBuffer{
		std::vector<drawnPixel> pixels;
		std::vector<drawnPoint> points;
		std::mutex mtx;
		void clear(){
			pixels.clear();
			points.clear();
		}
	};

	uint drawSeed=131071;
	drawBuffer drawBuf;

	//Check if pixel is already in buffer for this frame
	bool alreadyDrawn(uint16_t ax, uint16_t ay) {
		for (size_t i = 0; i < drawBuf.points.size(); ++i) {
			int64_t deltaX = (int)drawBuf.points[i].pos[0]-ax;
			int64_t deltaY = (int)drawBuf.points[i].pos[1]-ay;
			deltaX*=deltaX;
			deltaY*=deltaY;			
			int radSq = drawBuf.points[i].radius*drawBuf.points[i].radius;

			if (deltaX + deltaY < radSq) {
				return true; 
			}
		}
		return false; 
	}

public:

	//mouse draw on cellState values. called by InputHandler
	//mainsync before cursorDraw call
	void cursorDraw(int strength, int cursorX, int cursorY, int drawRadius, bool drawRandom, CACache::ThreadCache cache) {
		std::lock_guard<std::mutex> lock(drawBuf.mtx); //make sure drawBuf is not being copied while drawing
		uint numStates = cache.stateCount;

		if( drawBuf.points.size()!=0 ){//Ensure we aren't checking uninitialized elements
			//this only discards if this and previous positions are the same. 
			//With big canvas it's unlikely to have two exact same positions, 
			//and calculating the duplicates has marginal performance impact compared to std::find
			if( ( drawBuf.points.back().pos[0]==cursorX && drawBuf.points.back().pos[1]==cursorY ) ) { 
				return;
			}
		}

		uint8_t drawValue=ceil(strength * (numStates - 1) * strength);

		for (int i = -drawRadius; i <= drawRadius; i++) {
			for (int j = -drawRadius; j <= drawRadius; j++) {

				int mouseDist = i*i+j*j;
				int radSq = drawRadius*drawRadius;
				if(mouseDist >= radSq ){continue;} //if outside the circle, discard

				//calculate point of the circle world position 
				uint16_t ax = ((int)cursorX + i + cache.tileRows) % cache.tileRows;
				uint16_t ay = ((int)cursorY + j + cache.tileCols) % cache.tileCols;
				if ( alreadyDrawn(ax, ay) ) { continue; }
				drawSeed++;
				if(drawRandom) { drawValue = (kaelife::rand(&drawSeed))%numStates; }

				drawnPixel pixelInCircle={
					.pos={ax,ay},
					.state = drawValue
				};
				
				drawBuf.pixels.push_back(pixelInCircle);
			}
		}

		//store cursor position. alreadyDrawn() uses this to determine if pixels were already drawn on the same frame
		drawnPoint cursorPosHistory = {
			.pos={ (uint16_t)cursorX, (uint16_t)cursorY },
			.radius=(uint8_t)drawRadius
		};
		drawBuf.points.push_back(cursorPosHistory);

	}

	//Perform only when threads are paused
	uint copyDrawBuf(std::vector<std::vector<uint8_t>> &cellState, CACache::ThreadCache cache){
		std::lock_guard<std::mutex> lock(drawBuf.mtx);//wait till drawing is done

		if(drawBuf.pixels.empty()){	return 0; } //This was previously outside mutex lock which was potential cause for "attempt to copy from a singular iterator"

		for (const auto& pixel : drawBuf.pixels) {
			uint16_t x = pixel.pos[0]%cache.tileRows;
			uint16_t y = pixel.pos[1]%cache.tileCols;

			if(KAELIFE_DEBUG){
				if(x>=cache.tileRows || y>=cache.tileCols){
					printf("OUT OF WORLD BOUNDS copyDrawBuf\n");
					abort();
				}	
			}

			cellState[x][y] = pixel.state;
		}
		drawBuf.clear();

		return 1;
	}

};