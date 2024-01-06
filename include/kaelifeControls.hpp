
#pragma once

#include <SDL2/SDL.h>
#include <map>
#include <functional>
#include <algorithm>
#include <cmath>
#include <atomic>

#include <thread>
#include <chrono>
#include <stdint.h>

#include "kaelifeCAData.hpp"

/*
controls
Random rules..... [R]
Random add rule.. [T]
Random all....... [Y]
Mutate........... [M]
Draw............. mouse left/right
Draw radius...... [Q]:-1 [E]:+1
Draw random...... [W]
Simu Speed....... [1]:-1 [3]:+1
Pause............ [2], [Shift]+[P]
Iterate once..... [4]
Print rules...... [P]
Switch automata.. [,] [.]
Shader Color..... [Shift]+[N]
print frameTime.. [F]
Hue--............ [Shift]+[Q]
Hue++............ [Shift]+[E]
Color stagger--.. [Alt]+[Q]
Color stagger++.. [Alt]+[E]
Exit:............ [ESC]
*/

std::atomic<bool> QUIT_FLAG = false;

class InputHandler {
private:


	bool isMouseHeld = false;
	// Define a type for function pointers
	using KeyFunction = std::function<void(CAData&)>;
	// Map key combinations to functions
	std::map<SDL_Keycode, KeyFunction> keyFuncMap;

    // Map to store the state of keys or mouse buttons
    std::map<int, bool> keyStates;

	float holdAccel=1.0;

	std::atomic<bool> windowFocus=0;

	uint timerSt, timerNd;

	uint64_t drawSeed=314159265;

public:
		// Initialize key mappings in the constructor
	InputHandler() : keyFuncMap{
			{SDLK_r					 		, 	std::bind(&InputHandler::press_r, 			this, std::placeholders::_1)},
			{SDLK_t					 		, 	std::bind(&InputHandler::press_t, 			this, std::placeholders::_1 )},
			{SDLK_q					 		, 	std::bind(&InputHandler::press_q, 			this )},
			{SDLK_e					 		, 	std::bind(&InputHandler::press_e, 			this )},
			{SDLK_1					 		, 	std::bind(&InputHandler::press_1, 			this )},
			{SDLK_2					 		, 	std::bind(&InputHandler::press_2, 			this )},
			{SDLK_3					 		, 	std::bind(&InputHandler::press_3, 			this )},
			{SDLK_4					 		, 	std::bind(&InputHandler::press_4, 			this )},
			{SDLK_p					 		, 	std::bind(&InputHandler::press_p, 			this, std::placeholders::_1 )},
			{SDLK_w					 		, 	std::bind(&InputHandler::press_w, 			this )},
			{SDLK_f					 		, 	std::bind(&InputHandler::press_f, 			this )},
			{SDLK_m					 		, 	std::bind(&InputHandler::press_m, 			this, std::placeholders::_1 )},
			{SDLK_n					 		, 	std::bind(&InputHandler::press_n, 			this, std::placeholders::_1 )},
			{SDLK_y					 		, 	std::bind(&InputHandler::press_y, 			this, std::placeholders::_1 )},
			{SDLK_q	| (KMOD_LALT<<16)		, 	std::bind(&InputHandler::press_q_LALT, 		this )},
			{SDLK_e	| (KMOD_LALT<<16)		, 	std::bind(&InputHandler::press_e_LALT, 		this )},
			{SDLK_q	| (KMOD_LSHIFT<<16)		, 	std::bind(&InputHandler::press_q_LSHIFT, 	this )},
			{SDLK_e	| (KMOD_LSHIFT<<16)		, 	std::bind(&InputHandler::press_e_LSHIFT, 	this )},
			{SDLK_n	| (KMOD_LSHIFT<<16)		, 	std::bind(&InputHandler::press_n_LSHIFT, 	this )},
			{SDLK_p	| (KMOD_LSHIFT<<16)		, 	std::bind(&InputHandler::press_p_LSHIFT, 	this )},
			{SDLK_PERIOD					, 	std::bind(&InputHandler::press_PERIOD, 		this, std::placeholders::_1 )},
			{SDLK_COMMA						, 	std::bind(&InputHandler::press_COMMA, 		this, std::placeholders::_1 )},
			{SDLK_ESCAPE			 		, 	std::bind(&InputHandler::press_ESCAPE, 		this )}

	} {
//		prevReso[0]=1024;
//		prevReso[1]=1024;
//		clearRequired=0;
	}
	
	//set shader color
	//{hue,stagger} Sort 256 rgb 6-6-7 colors by luminosity
	uint8_t colorPreset[5][2]={
		{1,239 }, //0 pink
		{2,69 }, //0 cyan pink
		{40,108 }, //1 yellow blue
		{23,156 }, //2 brown blue white
		{4,83 } 
	};

	uint shaderColor=2; //0 grayscale, 1 normalmap, 2 color mapped
	uint useColPreset=1;
	uint8_t shaderHue=colorPreset[useColPreset][0];
	uint8_t colorStagger=colorPreset[useColPreset][1];

	float simSpeed=1; //iterations per frame

	bool stepFrame=false;
	bool displayFrameTime=false;
	bool pause=false;
	
	int drawRadius=2;
	float drawStrength=1.0;
	bool drawRandom=false;

	const bool debug=true; //const at least for now

    static int cursorPos[2];


	// Function prototypes
	void press_r(CAData &cellData);
	void press_t(CAData &cellData);
	void press_q();
	void press_e();
	void press_1();
	void press_2();
	void press_3();
	void press_4();
	void press_p(CAData &cellData);
	void press_w();
	void press_f();
	void press_m(CAData &cellData);
	void press_n(CAData &cellData);
	void press_y(CAData &cellData);
	void press_q_LALT();
	void press_e_LALT();
	void press_q_LSHIFT();
	void press_e_LSHIFT();
	void press_n_LSHIFT();
	void press_p_LSHIFT();
	void press_PERIOD(CAData &cellData);
	void press_COMMA(CAData &cellData);
	void press_ESCAPE();


	// Detect keys method
	void detectInput(CAData &cellData, SDL_Window* &SDLWindow);
	void cursorDraw(CAData &cellData, int strength, SDL_Window* &SDLWindow);
	

	static std::array<double, 4> getWorldTransform (const CAData &cellData, SDL_Window* &SDLWindow);
	static std::array<uint, 2>   getWorldCursorPos (const CAData &cellData, SDL_Window* &SDLWindow);


	int prevReso[2] = { 1024, 1024 };
	uint hasChanged = 0;

	bool hasResoChanged(int x, int y) {
		hasChanged-=hasChanged>0;
		if(
			prevReso[0] != x ||
			prevReso[1] != y
		){
			hasChanged=1; // clear next frame(s)
		}
		prevReso[0] = x;
		prevReso[1] = y;
		return hasChanged;
	}
};




int InputHandler::cursorPos[2] = {0, 0};

// Function implementations
	//randomize all
	void InputHandler::press_y(CAData &cellData){
		cellData.addBacklog("randAll");
	};
	//random mask
	void InputHandler::press_n(CAData &cellData){
		cellData.addBacklog("randMask");
	};
	//randomize ranges
	void InputHandler::press_r(CAData &cellData){
		cellData.addBacklog("randRange");
	};
	//randomize adds
	void InputHandler::press_t(CAData &cellData){
		cellData.addBacklog("randAdd");
	};
	//rand mutate
	void InputHandler::press_m(CAData &cellData){
		cellData.addBacklog("randMutate");
	};
	//shader color stagger--
	void InputHandler::press_q_LALT(){
		uint8_t add=std::min(holdAccel*holdAccel/40.0f,3.0f)+1;
		colorStagger-=add;
		colorStagger+=colorStagger==0;
		if(debug){
			printf("colorStagger: %d\n",colorStagger);
		}
	};
	//shader color stagger++
	void InputHandler::press_e_LALT(){
		uint8_t add=std::min(holdAccel*holdAccel/40.0f,3.0f)+1;
		colorStagger+=add;
		colorStagger+=colorStagger==0;
		if(debug){
			printf("colorStagger: %d\n",colorStagger);
		}
	};
	//shader hue--
	void InputHandler::press_q_LSHIFT(){
		uint8_t add=std::min(holdAccel*holdAccel/40.0f,3.0f)+1;
		shaderHue-=add;
		if(debug){
			printf("shaderHue: %d\n",shaderHue);
		}
	};
	//shader hue++
	void InputHandler::press_e_LSHIFT(){
		uint8_t add=std::min(holdAccel*holdAccel/40.0f,3.0f)+1;
		shaderHue+=add;
		if(debug){
			printf("shaderHue: %d\n",shaderHue);
		}
	};

	//draw radius--
	void InputHandler::press_q(){
		float sub=(holdAccel*holdAccel/10)+1;
		if( ((float)drawRadius-sub)>1 ){
			drawRadius-=(uint)sub;
		}else{
			drawRadius=1;
		}
		if(debug){
			printf("drawRadius: %d\n",drawRadius);
		}
	};
	//draw radius++
	void InputHandler::press_e(){
		float add=(holdAccel*holdAccel/10)+1;
		if( ((float)drawRadius+add)<255 ){
			drawRadius+=(uint)add;
		}else{
			drawRadius=255;
		}
		if(debug){
			printf("drawRadius: %d\n",drawRadius);
		}
	};

	//slow down sim
	void InputHandler::press_1(){ 
		float sub=(holdAccel*holdAccel)/1000.0;
		if(simSpeed>(sub+0.01)){
			simSpeed-=sub;
			simSpeed=round(simSpeed*1000)/1000;//round to 1/1000th
		}else{
			simSpeed=0.01;
		}
		if(debug){
			printf("simulationSpeed: %f\n",simSpeed);
		}
	};
	//pause
	void InputHandler::press_2(){ 
		pause=!pause;
		if(debug){
			printf("pause: %d\n", pause);
		}
	};
	//speed up sim
	void InputHandler::press_3(){ 
		float add=(holdAccel*holdAccel)/1000.0;
		if(simSpeed<1000){
			simSpeed+=add;
			simSpeed=round(simSpeed*1000)/1000;//round to 1/1000th
		}else{
			simSpeed=1000;
		}
		if(debug){
			printf("simulationSpeed: %f\n",simSpeed);
		}
	};
	//progress one sim step
	void InputHandler::press_4(){ 
		stepFrame=1;
		if(debug){
			printf("stepFrame: %d\n",stepFrame);
		}
	};
	//draw random
	void InputHandler::press_w(){
		drawRandom=!drawRandom;
		if(debug){
			printf("drawRandom: %d\n",drawRandom);
		}
	};
	//print rules
	void InputHandler::press_p(CAData &cellData){
		cellData.kaePreset.printPreset();
		printf("Shader {%d,%d},\n",shaderHue,colorStagger);
	};
	//show frame time
	void InputHandler::press_f(){
		displayFrameTime=!displayFrameTime;
		if(debug){
			printf("displayFrameTime: %d\n", displayFrameTime);
		}
	};
	//use nomral
	void InputHandler::press_n_LSHIFT(){
		shaderColor++;
		shaderColor=(shaderColor)%3;
		if(debug){
			printf("shaderColor: %d\n", shaderColor);
		}
	};
	//show debug 
	void InputHandler::press_p_LSHIFT(){
		pause=!pause;
		if(debug){
			printf("pause: %d\n", pause);
		}
	};
	//quit
	void InputHandler::press_ESCAPE(){
		QUIT_FLAG = true;
		if(debug){
			printf("Exit\n");
		}
	};
	//next preset
	void InputHandler::press_PERIOD(CAData &cellData){
		int ind =cellData.kaePreset.nextPreset();
		if(debug){
			printf("preset: %d\n",ind);
		}
		//wait buffer sync
		cellData.addBacklog("loadPreset");
	};
	//previous preset
	void InputHandler::press_COMMA(CAData &cellData){
		int ind = cellData.kaePreset.prevPreset();
		if(debug){
			printf("preset: %d\n",ind);
		}
		cellData.addBacklog("loadPreset");
	};



void InputHandler::detectInput(CAData &cellData, SDL_Window* &SDLWindow) {
	holdAccel=1.0;
	while(!QUIT_FLAG){
		SDL_Event event;
		
		timerSt=SDL_GetTicks();

		while(SDL_PollEvent(&event) != 0){

			//wrap and track mouse
			SDL_GetMouseState(&cursorPos[0], &cursorPos[1]);



			if (event.type == SDL_QUIT) {
				QUIT_FLAG = true;
			} else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
				if((event.type == SDL_KEYDOWN)){//accelerator increment
					holdAccel+=0.25;
				}else{
					holdAccel=1.0;
				}

				// Store the state of the key
				keyStates[event.key.keysym.sym] = (event.type == SDL_KEYDOWN);


				SDL_Keymod modState = SDL_GetModState();
				SDL_Keycode combinedKey = event.key.keysym.sym | (modState<<16);

				if(event.type == SDL_KEYDOWN){
					if(debug){
						printf("Key: %d, Modifiers: %d, Result: %d \n", event.key.keysym.sym, (modState<<16), combinedKey);
					}
					// Find the associated function in the map
					auto it = keyFuncMap.find(combinedKey);

					// If the key combination is found, execute the associated function
					if (it != keyFuncMap.end()) {
						KeyFunction& func = it->second;
						func(cellData);
					}
				}
			}

			if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) {
				// Store the state of the mouse button
				keyStates[event.button.button] = (event.type == SDL_MOUSEBUTTONDOWN);
				windowFocus=keyStates[event.button.button];

				if (debug) {
					printf("Mouse button %d pressed\n", event.button.button);
				}
				if(debug){
					printf("Mouse pos (%d, %d)\n", cursorPos[0], cursorPos[1]);
				}
				#ifdef KAELIFE_DEBUG
					auto cursorPosW = getWorldCursorPos(cellData, SDLWindow);
					printf("Mouse world (%d, %d)\n", cursorPosW[0], cursorPosW[1]);
				#endif
			}
		}
			
		//run these every call
		if (keyStates[SDL_BUTTON_LEFT] || keyStates[SDL_BUTTON_RIGHT] || event.type == SDL_MOUSEBUTTONDOWN) {
			// Mouse motion or left button pressed
			int strength = keyStates[SDL_BUTTON_LEFT] ? 1 : 0;
			cursorDraw(cellData, strength, SDLWindow);
		}

		if (windowFocus){
			int windowWrapLimit[2];
			SDL_GetWindowSize(SDLWindow, &windowWrapLimit[0], &windowWrapLimit[1]);
			windowWrapLimit[0]-=1;
			windowWrapLimit[1]-=1;

			bool moveCursor=0;
			//if past window edges
			if ( cursorPos[0]>= windowWrapLimit[0] ) {cursorPos[0]=0;	moveCursor=true;}
			else
			if ( cursorPos[0]<= 0  ) {cursorPos[0]=windowWrapLimit[0];moveCursor=true;}
			
			if ( cursorPos[1]>= windowWrapLimit[1] ) {cursorPos[1]=0;	moveCursor=true;}
			else
			if ( cursorPos[1]<= 0 ) {cursorPos[1]=windowWrapLimit[1];	moveCursor=true;}
			
			if(moveCursor){
				SDL_WarpMouseInWindow(SDLWindow, cursorPos[0], cursorPos[1]);
			}
		}

		do{ //update twice a frame
			timerNd=SDL_GetTicks() - timerSt;
			SDL_Delay(1);
		}while(timerNd < cellData.targetFrameTime/2);
	}
}





bool isDrawOverlap(CAData &cellData, uint16_t ax, uint16_t ay) {
	for (size_t i = 0; i < cellData.drawMouseBuf.size(); ++i) {
		int64_t deltaX = (int)cellData.drawMouseBuf[i].pos[0]-ax;
		int64_t deltaY = (int)cellData.drawMouseBuf[i].pos[1]-ay;
		deltaX*=deltaX;
		deltaY*=deltaY;			
		int radSq = cellData.drawMouseBuf[i].radius*cellData.drawMouseBuf[i].radius;

		if (deltaX + deltaY < radSq) {
			return true; 
		}
	}
	return false; 
}

//mouse draw on stateBuf values. called by InputHandler
//mainsync before cursorDraw call
void InputHandler::cursorDraw(CAData &cellData, int strength, SDL_Window* &SDLWindow) {
	auto cursorPos = getWorldCursorPos(cellData, SDLWindow);
	uint numStates = cellData.mainCache.stateCount;

	if( cellData.drawMouseBuf.size()!=0 ){//Ensure we aren't checking uninitialized elements
		//this only discards if this and previous positions are the same. 
		//With big canvas it's unlikely to have two exact same positions, 
		//and calculating the duplicates has marginal performance impact compared to std::find
		if( ( cellData.drawMouseBuf.back().pos[0]==cursorPos[0] && cellData.drawMouseBuf.back().pos[1]==cursorPos[1] ) ) { 
			return;
		}
	}

	uint8_t drawValue=ceil(drawStrength * (numStates - 1) * strength);

	{
		std::lock_guard<std::mutex> lock(cellData.drawMutex); //make sure drawBuf is not being copied while drawing

		for (int i = -drawRadius; i <= drawRadius; i++) {
			for (int j = -drawRadius; j <= drawRadius; j++) {

				int mouseDist = i*i+j*j;
				int radSq = drawRadius*drawRadius;
				if(mouseDist >= radSq ){continue;} //if inside the circle

				//calculate point of the circle world position 
				uint16_t ax = ((int)cursorPos[0] + i + cellData.mainCache.tileRows) % cellData.mainCache.tileRows;
				uint16_t ay = ((int)cursorPos[1] + j + cellData.mainCache.tileCols) % cellData.mainCache.tileCols;
				if ( isDrawOverlap(cellData, ax, ay) ) { continue; }
				drawSeed++;
				uint8_t rnum = (kaelRand(&drawSeed))%numStates;
				drawValue= drawRandom ? rnum : drawValue;

				CAData::drawBuffer pixel={
					.pos={ax,ay},
					.state = drawValue
				};
				
				cellData.drawBuf.push_back(pixel);
			}
		}
	}

	//store to be checked by isDrawOverlap if cursorDraw is called on same frame
	CAData::drawMouseBuffer cursorPosBuf = {
		.pos={ (uint16_t)cursorPos[0], (uint16_t)cursorPos[1] },
		.radius=(uint8_t)drawRadius
	};
	cellData.drawMouseBuf.push_back(cursorPosBuf);
	cellData.addBacklog("cursorDraw");
}



	//sdl window to world transform
	std::array<double, 4> InputHandler::getWorldTransform(const CAData &cellData, SDL_Window* &SDLWindow) {
		int x, y;
		SDL_GetWindowSize(SDLWindow, &x, &y );

		double windowAspectRatio = (double)(x) / (double)(y);

		double offsetX	= 0;
		double offsetY	= 0;
		double scaledX = x;
		double scaledY = y;
		
		//Height or width restricted 
		if (windowAspectRatio > cellData.aspectRatio) {
			// Calculate the scale and offset for black bars
			scaledX = y * cellData.aspectRatio;
			offsetX = (x - scaledX) / 2.0;
		} else {
			scaledY = x / cellData.aspectRatio;
			offsetY = (y - scaledY) / 2.0;
		}

		std::array<double, 4> output = {
			offsetX,
			offsetY,
			scaledX,
			scaledY
		};

		return output;

	}

	//mouse world coordinates
	std::array<uint, 2> InputHandler::getWorldCursorPos(const CAData &cellData, SDL_Window* &SDLWindow) {
		auto offsetScale = getWorldTransform(cellData, SDLWindow);
		std::vector<float> worldCursorPos(2);

		//black bar offset
		worldCursorPos[0]=cursorPos[0]-offsetScale[0];
		worldCursorPos[1]=cursorPos[1]-offsetScale[1];

		//normalize to 0-1.0
		worldCursorPos[0] = 	  (((float)worldCursorPos[0]) / (float)(offsetScale[2]));
		worldCursorPos[1] = 1.0 - (((float)worldCursorPos[1]) / (float)(offsetScale[3])); //glew vs sdl have inverted Y

		//scale to tile grid
		worldCursorPos[0] *= cellData.mainCache.tileRows;
		worldCursorPos[1] *= cellData.mainCache.tileCols;

		std::array<uint, 2> worldCursorOut;
		worldCursorOut[0]=worldCursorPos[0];
		worldCursorOut[1]=worldCursorPos[1];
		return worldCursorOut;
	}