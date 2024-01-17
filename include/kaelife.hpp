//kaelife.hpp
//Functions that would otherwise be free functions but are related to kaelife

#pragma once

constexpr bool KAELIFE_DEBUG = 1;

#include "kaelifeSDL.hpp"
#include "kaelifeWorldCore.hpp"
#include "kaelifeBMPIO.hpp"
#include "kaelRandom.hpp"

#include <stdint.h>

namespace kaelife {
    extern KaelRandom<uint64_t>rand;
    SDL_GLContext initSDL(SDL_Window* &SDLWindow, int windowWidth, int windowHeight);  
    void worldCore(CAData &kaelife, InputHandler &kaeInput, CARender &kaeRender, SDL_Window* &SDLWindow);
    void placeHolderDraw(CAData &kaelife);
}
