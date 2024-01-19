//kaelifeWorldCore.hpp
//Main thread CA iteration managing loop

#pragma once

#include "kaelifeCARender.hpp" 
#include "kaelifeCAData.hpp" 
#include "kaelifeControls.hpp" 
#include "kaelife.hpp"

#include <iostream>
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <cmath>
#include <vector>

namespace kaelife {

	void worldCore(CAData &kaelife, InputHandler &kaeInput, CARender &kaeRender, SDL_Window* &SDLWindow){
		std::vector<std::thread> iterThreads;

		std::thread iterHandler = std::thread([&]() {
			kaelife.iterateStateMT(iterThreads);
		});

		std::thread inputThread = std::thread([&]() {
			kaeInput.detectInput();
		});


		uint frameStartTime = SDL_GetTicks();
		float iterAccumulate = 0; //due simulation time (ms)
		uint lastframeTime=0; //last time that frame time was printed (ms)
		uint iterTask=0; //number of iterations to do in this cycle

		uint elapsedTime=0;

		int periodSize=2*1000.0/kaelife.targetFrameTime; //twice the frame rate
		std::vector<uint> periodIters(periodSize,1);
		uint periodIndex = 0;

		std::vector<uint> periodTime(periodSize,kaelife.slowFrameTime);

		float avgTime = kaelife.slowFrameTime;
		float avgIters = 1000.0/kaelife.slowFrameTime;

		float guessMaxIters = 1000.0/kaelife.slowFrameTime;
		
		while (!kaeInput.QUIT_FLAG) {

			periodIndex++;
			periodIndex%=periodSize;
			periodIters[periodIndex]=0;
			
			if(!kaeInput.pause || kaeInput.stepFrame){
				if(kaeInput.stepFrame){
					iterTask=1;
					iterAccumulate=kaelife.targetFrameTime;
					kaeInput.stepFrame=0;
				}else{// Simulate rules with a fixed time step
					iterAccumulate += ((float)kaelife.targetFrameTime * kaeInput.simSpeed); //calculate sped up time spent in simulation. Accumulate until simulation time is higher than 1 frame
				}


				if(iterAccumulate>=kaelife.targetFrameTime){//at least 1 iteration
					iterTask = iterAccumulate/kaelife.targetFrameTime; //calculate new iter count per frame
					if(iterTask > guessMaxIters){
						iterTask=guessMaxIters;
						iterAccumulate = iterTask*kaelife.targetFrameTime;
					}

					kaelife.kaeMutex.continueThread(iterTask,kaelife.mainCache.activeBuf); //pass iteration count

					float wholeIters=iterTask*kaelife.targetFrameTime; //Simulation time of whole iterations
					iterAccumulate-=(float)wholeIters; //substract the iteration count passed to continueThread
					iterAccumulate= iterAccumulate<0 ? 0 : iterAccumulate;
					
					periodIters[periodIndex]=iterTask;
				}
			}
			//pass previous buffer iteration to GPU while iterTask is being computed in new buffer
			SDL_GL_SwapWindow(SDLWindow);
			kaeRender.renderCells();

			do{ // Cap the frame rate
				elapsedTime=SDL_GetTicks() - frameStartTime;
				SDL_Delay(1);
			}while(elapsedTime < kaelife.targetFrameTime);
			frameStartTime = SDL_GetTicks();

			lastframeTime+=elapsedTime;
			periodTime[periodIndex]=elapsedTime;

			//infrequent updates
			if(lastframeTime>=500){
				avgIters=0;
				avgTime=0;
				for(int i=0;i<periodSize;i++){ //average every period value
					avgIters+=periodIters[i];
					avgTime+=periodTime[i];
				}
				avgIters/=(float)periodSize;
				avgTime /=(float)periodSize;

				if(kaeInput.displayFrameTime){
					float itersPerSec = avgIters*(1000.0/avgTime) * !kaeInput.pause;
					printf("%f ms %f iter/s\n", avgTime, itersPerSec);
					//printf("guess max %f\n", guessMaxIters);
				}
				lastframeTime=0;
			}else{
				//average current avg and previous value otherwise
				avgTime =(avgTime *(periodSize-1)+elapsedTime				)/periodSize;
				avgIters=(avgIters*(periodSize-1)+periodIters[periodIndex]	)/periodSize;
			}
			periodSize=(int)2*(1000.0/avgTime); //change period size depending on frame rate
			periodSize+=periodSize==0;

			float frameTimeDistance = abs( avgTime - kaelife.slowFrameTime ); //how far off frame time is from slowest allowed time
			if( frameTimeDistance > kaelife.targetFrameTime && !kaeInput.pause){ //if higher than 1 frame deviation
				if(avgTime>kaelife.slowFrameTime){ //if the simulation is lagging behind from target
					float newGuess = guessMaxIters/((float)avgTime/kaelife.slowFrameTime);
					guessMaxIters=(guessMaxIters+2*newGuess)/3.0; //smooth by average
					
					guessMaxIters = guessMaxIters<1 ? 1 : guessMaxIters;
				}else{
					float newGuess = guessMaxIters * ((float)kaelife.slowFrameTime/avgTime);
					newGuess = guessMaxIters > kaeInput.simSpeed ? kaeInput.simSpeed : guessMaxIters;
					guessMaxIters=(guessMaxIters+2*newGuess)/3.0;

					guessMaxIters+=1.0;
				}
			}

			kaelife.kaeMutex.syncMainThread(); //sync iterations
			kaelife.backlog->doBacklog(); //execute not-thread-safe-tasks thread-safely
		}

		//join any running threads
		kaelife.kaeMutex.terminateThread();
		inputThread.join();
		iterHandler.join();
	}

}