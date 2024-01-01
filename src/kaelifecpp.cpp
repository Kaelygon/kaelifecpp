//2-bit cellular automata by Kaelygon CC BY-SA 4.0 2023
//https://creativecommons.org/licenses/by-sa/4.0/
#include <GL/glew.h>
#include <iostream>
#include <SDL2/SDL.h>
#include <cmath>
#include <time.h>
#include <vector>
#include <algorithm>

#define KAELIFE_DEBUG 0

#include "./kaelifeWorldMatrix.hpp"
#include "./kaelifeCAData.hpp"
#include "./kaelifeControls.hpp"
#include "./kaelifeSDL.hpp"
#include "./kaelifeRenderer.hpp"

void placeHolderDraw(CAData &kaelife){	
	{
		if(kaelife.mainCache.tileRows>16 && kaelife.mainCache.tileCols>16 ){
			//draw 2 fliers
			WorldMatrix<uint8_t> flier={
				{0,0,1,0},
				{2,2,2,0},
				{0,2,2,3},
				{2,2,2,0},
				{0,3,3,0},
				{0,1,1,0}
			};

			uint posx=kaelife.mainCache.tileRows/2-1;
			uint posy=kaelife.mainCache.tileCols/2-2;
			for(size_t i=0;i<flier.wmSize();i++){
				for(size_t j=0;j<flier[0].wmSize();j++){
					kaelife.stateBuf[!kaelife.mainCache.activeBuf][i + posx  ][j + posy   ] = flier[i][j]; //write to inactive buffer
					kaelife.stateBuf[!kaelife.mainCache.activeBuf][i + posx-12][j + posy-1] = flier[i][j];
				}
			}

		}

		if(kaelife.mainCache.tileRows>64 && kaelife.mainCache.tileCols>32 ){
			WorldMatrix<uint8_t> diagonalFlier={
				{2,3,3,0},
				{3,3,1,0},
				{0,2,2,1},
				{3,2,2,3},
				{0,2,2,0},
				{0,3,3,0},
				{0,1,1,0}
			};

			uint posx=kaelife.mainCache.tileRows/2 + 16;
			uint posy=kaelife.mainCache.tileCols/2;
			for(size_t k=0;k<3;k++){
				for(size_t i=0;i<diagonalFlier.wmSize();i++){
					for(size_t j=0;j<diagonalFlier[0].wmSize();j++){
						int ofx=18-(-4);
						int ofy= 4+(-4);
						ofx=ofx*k+(2*(k==2)-(3)*(k==0));
						ofy=ofy*k-(2*(k==2)-(3)*(k==0));
						kaelife.stateBuf[!kaelife.mainCache.activeBuf][i + posx+ofx][j + posy+ofy] = diagonalFlier[i][j]; //maybe possible to keep them alive together
					}
				}
			}
		}


		kaelife.addBacklog("cloneBuffer");
		kaelife.doBacklog();
	}

}

int main() {
	
	CAData kaelife;
	CALock kaeMutex;
	InputHandler kaeInput;

	SDL_Window* mainSDLWindow;
	SDL_GLContext glContext = kaelifeInitSDL(mainSDLWindow,kaelife.renderWidth,kaelife.renderHeight);
	if(!glContext){
		return -1;
	}	

	initPixelMap(kaelife);

	//could do this in draw calss if one ever is made
	placeHolderDraw(kaelife);

	SDL_GL_SetSwapInterval(0);

	std::vector<std::thread> iterThreads;

	std::thread iterHandler = std::thread([&]() {
		kaelife.iterateStateMT(iterThreads, &kaeMutex);
	});

	std::thread inputThread = std::thread([&]() {
		kaeInput.detectInput(kaelife, mainSDLWindow);
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

	double avgTime = kaelife.slowFrameTime;
	double avgIters = 1000.0/kaelife.slowFrameTime;

	double guessMaxIters = 1000.0/kaelife.slowFrameTime;
	
	while (!QUIT_FLAG) {

		periodIndex++;
		periodIters[(periodIndex)%periodSize]=0;
		
		if(!kaeInput.pause || kaeInput.stepFrame){
			if(kaeInput.stepFrame){
				iterTask=1;
				iterAccumulate=kaelife.targetFrameTime;
				kaeInput.stepFrame=0;
			}else{// Simulate rules with a fixed time step
				iterAccumulate += ((float)kaelife.targetFrameTime * kaeInput.simSpeed); //calculate sped up time spent in simulation. Accumulate until simulation time is higher than 1 frame
			}


			if(iterAccumulate>=kaelife.targetFrameTime){//at least 1 iteration
				//iterAccumulate = avgTime > kaelife.slowFrameTime ? kaelife.slowFrameTime : iterAccumulate; //cap to slow frame time
				iterTask = iterAccumulate/kaelife.targetFrameTime; //calculate new iter count per frame
				if(iterTask > guessMaxIters){
					iterTask=guessMaxIters;
					iterAccumulate = iterTask*kaelife.targetFrameTime;
				}

				kaeMutex.continueThread(iterTask,kaelife.mainCache.activeBuf); //pass iteration count

				double wholeIters=iterTask*kaelife.targetFrameTime; //Simulation time of whole iterations
				iterAccumulate-=(double)wholeIters; //substract the iteration count passed to continueThread
				iterAccumulate= iterAccumulate<0 ? 0 : iterAccumulate;
				
				periodIters[(periodIndex)%periodSize]=iterTask;
			}
		}
		renderCells(kaelife,kaeInput,mainSDLWindow); 
		SDL_GL_SwapWindow(mainSDLWindow);

		do{ // Cap the frame rate
			elapsedTime=SDL_GetTicks() - frameStartTime;
			SDL_Delay(1);
		}while(elapsedTime < kaelife.targetFrameTime);
		frameStartTime = SDL_GetTicks();

		lastframeTime+=elapsedTime;
		periodTime[(periodIndex)%periodSize]=elapsedTime;

		//infrequent updates
		if(lastframeTime>=500){
			avgIters=0;
			avgTime=0;
			for(int i=0;i<periodSize;i++){ //average every period value
				avgIters+=periodIters[i];
				avgTime+=periodTime[i];
			}
			avgIters/=(double)periodSize;
			avgTime /=(double)periodSize;

			if(kaeInput.displayFrameTime){
				double itersPerSec = avgIters*(1000.0/avgTime) * !kaeInput.pause;
				printf("%f ms %f iter/s\n", avgTime, itersPerSec);
				//printf("guess max %f\n", guessMaxIters);
			}
			lastframeTime=0;
		}else{
			//average current avg and previous value otherwise
			avgTime =(avgTime *(periodSize-1)+elapsedTime							)/periodSize;
			avgIters=(avgIters*(periodSize-1)+periodIters[(periodIndex)%periodSize]	)/periodSize;
		}
		periodSize=(int)2*(1000.0/avgTime); //change period size depending on frame rate
		periodSize+=periodSize==0;

		double frameTimeDistance = abs( avgTime - kaelife.slowFrameTime ); //how far off frame time is from slowest allowed time
		if( frameTimeDistance > kaelife.targetFrameTime && !kaeInput.pause){ //if higher than 1 frame deviation
			if(avgTime>kaelife.slowFrameTime){ //if the simulation is lagging behind from target
				double newGuess = guessMaxIters/((double)avgTime/kaelife.slowFrameTime);
				guessMaxIters=(guessMaxIters+2*newGuess)/3.0; //smooth by average
				
				guessMaxIters = guessMaxIters<1 ? 1 : guessMaxIters;
			}else{
				double newGuess = guessMaxIters * ((double)kaelife.slowFrameTime/avgTime);
				newGuess = guessMaxIters > kaeInput.simSpeed ? kaeInput.simSpeed : guessMaxIters;
				guessMaxIters=(guessMaxIters+2*newGuess)/3.0;

				guessMaxIters+=1.0;
			}
		}

		kaeMutex.syncMainThread(); //sync iterations
		kaelife.doBacklog(); //execute not-thread-safe-tasks thread-safely
	}

	//join any running threads
	kaeMutex.terminateThread();
	inputThread.join();
	iterHandler.join();
	
	// Cleanup
    glUseProgram(0); // Unbind shader program
	SDL_GL_DeleteContext(glContext);
	SDL_DestroyWindow(mainSDLWindow);
	SDL_Quit();

	return 0;
}

//valgrind --tool=callgrind ./kaelifeMain
//LSAN_OPTIONS=verbosity=1:log_threads=1 | ./kaelifeMain