
//https://wiki.libsdl.org/SDL2/SDL_Keycode

#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

/*

//Or more organized structs that hold similar things

class InputHandler {
private:
struct PrivateInputConfig {
	using KeyFunction = std::function<void(CAData&)>;
//...
};
PrivateInputConfig privCfg;

public:
struct PublicInputConfig {
	std::atomic<bool> QUIT_FLAG = false;
//...
};
PublicInputConfig publCfg;

Then copy the config in constructor

//constructor
InputHandler(MasterConfig masterCfg){
	privCfg = masterCfg.privCfg;
	publCfg = masterCfg.publCfg;
	//...
}


*/

#include "./kaelifeControls.hpp" //inputHandler
#include "./kaelifeCAData.hpp" //CAData
#include "./kaelifeCAPreset.hpp" //CAPreset
#include "./kaelifeCACache.hpp" //CACache

class configHandler{


    public:
    struct MasterConfig{
        CAPreset::PresetList presetCfg;
        CACache::ThreadCache cacheCfg;
        //
        //
    };
};

int main() {

    // Use config struct in your program...

    return 0;
}
