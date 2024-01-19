//kaelifeControls.hpp
//Manage SDL2 user input

#pragma once

#include <SDL2/SDL.h>
#include <map>
#include <functional>
#include <algorithm>
#include <cmath>
#include <atomic>

#include <thread>
#include <stdint.h>

#include "kaelife.hpp"
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

//TODO: organize these billion variables to structs
class InputHandler {
private:

    CAData& cellData;
    SDL_Window*& SDLWindow;
	// Map key combinations to functions
	std::map<SDL_Keycode, std::function<void(CAData&)>> keyFuncMap;

    // Map to store the state of keys or mouse buttons
    std::map<int, bool> keyStates;
	
	bool isMouseHeld = false;
	float holdAccel=1.0;
	bool windowFocus=0;
	uint timerSt, timerNd;
	uint64_t drawSeed=314159265;

public:
	std::atomic<bool> QUIT_FLAG = false;
	
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
	
	// Initialize key mappings in the constructor
	InputHandler(
		CAData &inCAData, 
		SDL_Window*& inSDL_Window
	) : 
		cellData(inCAData), 
		SDLWindow(inSDL_Window),
		keyFuncMap
	{
			{SDLK_r					 		, 	std::bind(&InputHandler::press_r, 			this )},
			{SDLK_t					 		, 	std::bind(&InputHandler::press_t, 			this )},
			{SDLK_q					 		, 	std::bind(&InputHandler::press_q, 			this )},
			{SDLK_e					 		, 	std::bind(&InputHandler::press_e, 			this )},
			{SDLK_1					 		, 	std::bind(&InputHandler::press_1, 			this )},
			{SDLK_2					 		, 	std::bind(&InputHandler::press_2, 			this )},
			{SDLK_3					 		, 	std::bind(&InputHandler::press_3, 			this )},
			{SDLK_4					 		, 	std::bind(&InputHandler::press_4, 			this )},
			{SDLK_p					 		, 	std::bind(&InputHandler::press_p, 			this )},
			{SDLK_w					 		, 	std::bind(&InputHandler::press_w, 			this )},
			{SDLK_f					 		, 	std::bind(&InputHandler::press_f, 			this )},
			{SDLK_m					 		, 	std::bind(&InputHandler::press_m, 			this )},
			{SDLK_n					 		, 	std::bind(&InputHandler::press_n, 			this )},
			{SDLK_y					 		, 	std::bind(&InputHandler::press_y, 			this )},
			{SDLK_q	| (KMOD_LALT<<16)		, 	std::bind(&InputHandler::press_q_LALT, 		this )},
			{SDLK_e	| (KMOD_LALT<<16)		, 	std::bind(&InputHandler::press_e_LALT, 		this )},
			{SDLK_q	| (KMOD_LSHIFT<<16)		, 	std::bind(&InputHandler::press_q_LSHIFT, 	this )},
			{SDLK_e	| (KMOD_LSHIFT<<16)		, 	std::bind(&InputHandler::press_e_LSHIFT, 	this )},
			{SDLK_n	| (KMOD_LSHIFT<<16)		, 	std::bind(&InputHandler::press_n_LSHIFT, 	this )},
			{SDLK_p	| (KMOD_LSHIFT<<16)		, 	std::bind(&InputHandler::press_p_LSHIFT, 	this )},
			{SDLK_PERIOD					, 	std::bind(&InputHandler::press_PERIOD, 		this )},
			{SDLK_COMMA						, 	std::bind(&InputHandler::press_COMMA, 		this )},
			{SDLK_ESCAPE			 		, 	std::bind(&InputHandler::press_ESCAPE, 		this )}

	} {}

	// Function prototypes
	void press_r();
	void press_t();
	void press_q();
	void press_e();
	void press_1();
	void press_2();
	void press_3();
	void press_4();
	void press_p();
	void press_w();
	void press_f();
	void press_m();
	void press_n();
	void press_y();
	void press_q_LALT();
	void press_e_LALT();
	void press_q_LSHIFT();
	void press_e_LSHIFT();
	void press_n_LSHIFT();
	void press_p_LSHIFT();
	void press_PERIOD();
	void press_COMMA();
	void press_ESCAPE();


	// Detect keys method
	void detectInput();
	

	std::array<double, 4> getWorldTransform ();
	std::array<int, 2>   getWorldCursorPos ();
};




int InputHandler::cursorPos[2] = {0, 0};

// Function implementations
	//randomize all
	void InputHandler::press_y(){
		cellData.backlog->add("randAll");
	};
	//random mask
	void InputHandler::press_n(){
		cellData.backlog->add("randMask");
	};
	//randomize ranges
	void InputHandler::press_r(){
		cellData.backlog->add("randRange");
	};
	//randomize adds
	void InputHandler::press_t(){
		cellData.backlog->add("randAdd");
	};
	//rand mutate
	void InputHandler::press_m(){
		cellData.backlog->add("randMutate");
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
	void InputHandler::press_p(){
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
	void InputHandler::press_PERIOD(){
		int ind =cellData.kaePreset.nextPreset();
		if(debug){
			printf("preset: %d\n",ind);
		}
		//wait buffer sync
		cellData.backlog->add("loadPreset");
	};
	//previous preset
	void InputHandler::press_COMMA(){
		int ind = cellData.kaePreset.prevPreset();
		if(debug){
			printf("preset: %d\n",ind);
		}
		cellData.backlog->add("loadPreset");
	};



void InputHandler::detectInput() {
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
						std::function<void(CAData&)>& func = it->second;
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
				if(KAELIFE_DEBUG){
					auto cursorPosW = getWorldCursorPos();
					printf("Mouse world (%d, %d)\n", cursorPosW[0], cursorPosW[1]);
				}
			}
		}
			
		//run these every call
		if (keyStates[SDL_BUTTON_LEFT] || keyStates[SDL_BUTTON_RIGHT] || event.type == SDL_MOUSEBUTTONDOWN) {
			auto cursorPos = getWorldCursorPos();
			if(KAELIFE_DEBUG){
				printf("cursorPos[0] %d\n",cursorPos[0]);
				printf("cursorPos[1] %d\n",cursorPos[1]);
			}
			// Mouse motion or left button pressed
			int strength = keyStates[SDL_BUTTON_LEFT] ? 1 : 0;
			cellData.kaeDraw.cursorDraw(strength, cursorPos[0], cursorPos[1], drawRadius, drawRandom, cellData.mainCache);
			cellData.backlog->add("cursorDraw");
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
			if ( cursorPos[0]<= 0 ) {cursorPos[0]=windowWrapLimit[0];	moveCursor=true;}
			
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




	//sdl window to world transform
	std::array<double, 4> InputHandler::getWorldTransform() {
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

	//mouse world coordinates wrapped
	std::array<int, 2> InputHandler::getWorldCursorPos() {
		auto offsetScale = getWorldTransform();
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

		std::array<int, 2> worldCursorOut;
		worldCursorOut[0]=(int)(worldCursorPos[0]+cellData.mainCache.tileRows)%cellData.mainCache.tileRows;
		worldCursorOut[1]=(int)(worldCursorPos[1]+cellData.mainCache.tileCols)%cellData.mainCache.tileCols;
		return worldCursorOut;
	}