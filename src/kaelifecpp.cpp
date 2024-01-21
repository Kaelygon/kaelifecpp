/**
 * @file kaelifecpp.cpp
 * 
 * @brief Generalized 8-bit cellular automata c++ 
 * https://github.com/Kaelygon/kaelifecpp/
*/

#include "kaelRandom.hpp" //Randomizers and Hashers
namespace kaelife { 
	KaelRandom<uint64_t>rand; //global randomizer kaelife::rand()
    constexpr bool CA_DEBUG = 0; //Enable debugging for CA related classes
    constexpr bool INPUT_DEBUG = 1; //Enable debugging for InputHandler
}

#include "kaelife.hpp" //kaelife:: Namespace functions
#include "kaelifeBMPIO.hpp" //TODO: import export world data to bitmap
#include "CA/kaelifeCAData.hpp" //CA simulation iterator
#include "kaelifeControls.hpp" //user controls
#include "kaelifeConfigIO.hpp" //TODO: Import JSON to data struct and pass sub structs to each class constructor. This could act as global like variables
#include "kaelifeRender.hpp" //OpenGL render world as texture
#include "kaelifeSDL.hpp" //SDL window creation
#include "kaelifeWorldCore.hpp" //Manages user input and simulation threads

#include <iostream>
#include <SDL2/SDL.h>
#include <GL/glew.h>

int main() {

//	MasterConfig config("./config/"); //todo

    CAData kaelife;

    SDL_Window* mainSDLWindow;
    SDL_GLContext glContext = kaelife::initSDL(mainSDLWindow, kaelife.renderWidth, kaelife.renderHeight);
    if (!glContext) {
        return -1;
    }

    InputHandler kaeInput(kaelife, mainSDLWindow);

	CADraw kaeDraw;
    CARender kaeRender(kaelife, kaeInput, mainSDLWindow);
 
	kaeRender.initOpenGL();

	kaelife::placeHolderDraw(kaelife);

	kaelife::worldCore(kaelife, kaeRender, kaeInput, mainSDLWindow);

	SDL_GL_SetSwapInterval(0);

	// Cleanup
    glUseProgram(0);
	SDL_GL_DeleteContext(glContext);
	SDL_DestroyWindow(mainSDLWindow);
	SDL_Quit();

	return 0;
}

//benchmarking , needs -g flag
//valgrind --tool=callgrind ./build/kaelifecpp_OPTIMIZED
//
