
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

void placeHolderDraw(CAData &kaelife){	//perhaps ppm/png to world could be done
 
	uint rows = kaelife.mainCache.tileRows;
	uint cols = kaelife.mainCache.tileCols;

		//draw 2 fliers
		WorldMatrix<uint8_t> flier={
			{0,0,1,0},
			{2,2,2,0},
			{0,2,2,3},
			{2,2,2,0},
			{0,3,3,0},
			{0,1,1,0}
		};

		uint posx=rows/2-1;
		uint posy=cols/2-2;
		for(size_t i=0;i<flier.getWidth();i++){
			for(size_t j=0;j<flier.getHeight();j++){
				uint ai = i + posx   ; 
				uint aj = j + posy   ;
				uint bi = i + posx-12;
				uint bj = j + posy-1 ;
				ai=(ai+rows)%rows;
				aj=(aj+cols)%cols;
				bi=(bi+rows)%rows;
				bj=(bj+cols)%cols;
				kaelife.stateBuf[!kaelife.mainCache.activeBuf][ai][aj] = flier[i][j]; //write to inactive buffer
				kaelife.stateBuf[!kaelife.mainCache.activeBuf][bi][bj] = flier[i][j];
			}
		}

		WorldMatrix<uint8_t> diagonalFlier={
			{2,3,3,0},
			{3,3,1,0},
			{0,2,2,1},
			{3,2,2,3},
			{0,2,2,0},
			{0,3,3,0},
			{0,1,1,0}
		};

		posx=kaelife.mainCache.tileRows/2 + 16;
		posy=kaelife.mainCache.tileCols/2;
		for(size_t k=0;k<3;k++){
			for(size_t i=0;i<diagonalFlier.getWidth();i++){
				for(size_t j=0;j<diagonalFlier.getHeight();j++){
					int ofx=18-(-4);
					int ofy= 4+(-4);
					ofx=ofx*k+(2*(k==2)-(3)*(k==0));
					ofy=ofy*k-(2*(k==2)-(3)*(k==0));
					ofx=((i+posx+ofx)+rows)%rows;
					ofy=((j+posy+ofy)+cols)%cols;
					kaelife.stateBuf[!kaelife.mainCache.activeBuf][ofx][ofy] = diagonalFlier[i][j]; //maybe possible to keep them alive together
				}
			}
		}

	//border test
//	kaelife.stateBuf[!kaelife.mainCache.activeBuf][0][0]=1;
//	kaelife.stateBuf[!kaelife.mainCache.activeBuf][kaelife.mainCache.tileRows-1][0]=2;
//	kaelife.stateBuf[!kaelife.mainCache.activeBuf][kaelife.mainCache.tileRows-1][kaelife.mainCache.tileCols-1]=3;

	kaelife.addBacklog("cloneBuffer");
	kaelife.doBacklog();
}

int main() {

	CAData kaelife;
	InputHandler kaeInput;

	SDL_Window* mainSDLWindow;
	SDL_GLContext glContext = kaelifeInitSDL(mainSDLWindow,kaelife.renderWidth,kaelife.renderHeight);
	if(!glContext){
		return -1;
	}	

	initPixelMap(kaelife);

	//could do this in draw class if one ever is made
	placeHolderDraw(kaelife);

	SDL_GL_SetSwapInterval(0);

	std::vector<std::thread> iterThreads;

	std::thread iterHandler = std::thread([&]() {
		kaelife.iterateStateMT(iterThreads);
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

	float avgTime = kaelife.slowFrameTime;
	float avgIters = 1000.0/kaelife.slowFrameTime;

	float guessMaxIters = 1000.0/kaelife.slowFrameTime;
	
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

				kaelife.kaeMutex.continueThread(iterTask,kaelife.mainCache.activeBuf); //pass iteration count

				float wholeIters=iterTask*kaelife.targetFrameTime; //Simulation time of whole iterations
				iterAccumulate-=(float)wholeIters; //substract the iteration count passed to continueThread
				iterAccumulate= iterAccumulate<0 ? 0 : iterAccumulate;
				
				periodIters[(periodIndex)%periodSize]=iterTask;
			}
		}
		SDL_GL_SwapWindow(mainSDLWindow);
		renderCells(kaelife,kaeInput,mainSDLWindow); 

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
			avgTime =(avgTime *(periodSize-1)+elapsedTime							)/periodSize;
			avgIters=(avgIters*(periodSize-1)+periodIters[(periodIndex)%periodSize]	)/periodSize;
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
		kaelife.doBacklog(); //execute not-thread-safe-tasks thread-safely
	}

	//join any running threads
	kaelife.kaeMutex.terminateThread();
	inputThread.join();
	iterHandler.join();
	
	// Cleanup
    glUseProgram(0); // Unbind shader program
	SDL_GL_DeleteContext(glContext);
	SDL_DestroyWindow(mainSDLWindow);
	SDL_Quit();

	return 0;
}

//benchmarking , needs -g flag
//valgrind --tool=callgrind ./build/kaelifecpp_optimized
//
