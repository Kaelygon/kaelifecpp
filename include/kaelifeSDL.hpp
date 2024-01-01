#pragma once
#include <iostream>
#include <SDL2/SDL.h>
#include <GL/glew.h>

SDL_GLContext kaelifeInitSDL(	SDL_Window* &SDLWindow, int windowWidth, int windowHeight){

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
	}    

	// Get the number of available displays
	int numDisplays = SDL_GetNumVideoDisplays();
	if (numDisplays <= 0) {
		std::cerr << "No displays found" << std::endl;
	}

	// Choose the display index (monitor) you want to use
	int displayIndex = 2;  // Change this to the desired display index

	// Set the window position to the chosen display
	int windowX = SDL_WINDOWPOS_CENTERED_DISPLAY(displayIndex);
	int windowY = SDL_WINDOWPOS_CENTERED_DISPLAY(displayIndex);

	SDLWindow = SDL_CreateWindow("GPU Rendering", windowX, windowY, windowWidth, windowHeight, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	if (!SDLWindow) {
		std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
	}

	SDL_GLContext glContext = SDL_GL_CreateContext(SDLWindow);
	SDL_GL_MakeCurrent(SDLWindow, glContext);

	if (!glContext) {
		std::cerr << "OpenGL context creation failed: " << SDL_GetError() << std::endl;
	}

	GLenum err = glewInit();
	if (err != GLEW_OK) {
		std::cerr << "GLEW initialization failed: " << glewGetErrorString(err) << std::endl;
	}

	// Set the swap interval (0 for immediate updates, 1 for updates synchronized with the vertical retrace)
	if (SDL_GL_SetSwapInterval(1) < 0) {
		std::cerr << "Failed to set VSync: " << SDL_GetError() << std::endl;
		// Handle the error as needed
	}

	SDL_SetRelativeMouseMode(SDL_FALSE);

	return glContext;
}