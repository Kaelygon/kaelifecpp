//kaelife.cpp
//Generalized 8-bit cellular automata c++ 
//https://github.com/Kaelygon/kaelifecpp/

#include "kaelRandom.hpp" //Randomizers and Hashers
namespace kaelife { KaelRandom<uint64_t>rand; } //kaelife::rand()
#include "kaelife.hpp" //kaelife:: Namespace functions
#include "kaelifeSDL.hpp" //SDL window creation
#include "kaelifeWorldMatrix.hpp" //2D vector in world space
#include "kaelifeCAData.hpp" //CA simulation iterator
#include "kaelifeConfigIO.hpp" //TODO: Import JSON to data struct and pass sub structs to each class constructor. This could act as global like variables
#include "kaelifeWorldCore.hpp" //Manages user input and simulation threads
#include "kaelifeBMPIO.hpp" //TODO: import export world data to bitmap
#include "kaelifeControls.hpp" //user controls
#include "kaelifeCARender.hpp" //OpenGL render world as texture
#include "kaelifeCADraw.hpp" //Change world cell states from mouse input 

#include <iostream>
#include <SDL2/SDL.h>
#include <GL/glew.h>

int main() {

//	MasterConfig config("./config/");

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

	kaelife::worldCore(kaelife, kaeInput, kaeRender, mainSDLWindow);

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
