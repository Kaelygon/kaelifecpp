----------------------------------------------------------------------------------------------

![alt text](./icon.png)

# kaelifecpp Generalized Cellular Automata

## Rules explanation
```
The cellular automata in this program can have number of different 
configurations.

Let's use example Kaelife
```
```c++
{
    .name = "Kaelife",
    .stateCount = 4,
    .ruleRange = {6,9,11,24},
    .ruleAdd = {-1,1,-1,0,-1},
    .neigMask = {
        {255, 255, 255},
        {255,   0, 255},
        {255, 255, 255},
    },
    .clipTreshold = stateCount/2
}
```
```
stateCount is the number of states, 0 to 3
ruleRange is a sorted list of possible neighbor sums
ruleAdd is list of modifiers that will be added to the cell
neigMask (neighbor mask) is a 2D WorldMatrix. WorldMatrix which acts similar to vector but it 
matches the world space orientation and it is always a rectangle. These values are a
nominator of 255. As an example neigMask 128 will result in floor(cellValue*128/255) which
is added to neighbor sum.
clipTreshold is value, any any cell values below it are discarded.

Each iteration following calculations are performed on each cell as follows:

-Center of the neigMask is placed on same coordinate as the current cell.
-Any cells inside the mask below clipTreshold are discarded.
-Each cell value is multiplied by corresponding neigMask value, divided by 255 and added to 
    neighbor sum.
-Neighbor sum is compared with every ruleRange element until next ruleRange is
    bigger than the neighbor sum. 
-Number of steps from ruleRange beginning is used to choose ruleAdd element
-Chosen ruleAdd element is added to currently iterated cell and clamped between 0 and 
    stateCount.
-Newly calculated cell value is written to a buffer until all cells are computed
-Completed buffer values are used for next iteration
```

## User Controls
```
The engine uses SDL2 and GLEW for simple display and keyboard and mouse input.
There is no GUI and most of the information is printed in console.
The cellular automata is displayed on window with default size of 576x384
```
```
Controls
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
```

----------------------------------------------------------------------------------------------

## Building
Dependencies 
```
GL - Usually provided by graphics driver
GLEW - https://archlinux.org/packages/extra/x86_64/glew/
SDL2 - https://archlinux.org/packages/extra/x86_64/sdl2/
```

The only platform I use to program is x86 Arch Linux so your mileage may vary. <br>
Open console in some directory and clone the repository using
```
$ git clone https://github.com/Kaelygon/kaelifecpp/
cd kaelifecpp/
```

Then you can build running the provided CMake bash script. Any arguments are optional. 
```
sh CMakeBuild.sh [ALL, ACTIVE] [DEBUG, ASAN, OPTIMIZED] [SRC_DIR] [PROG]
```
ALL : (Default) Builds every .cpp file into a program in SRC_DIR <br>
ACTIVE : Builds program named PROG.cpp in folder ./SRC_DIR 

DEBUG : "-Wall -Wextra -pedantic -D_GLIBCXX_DEBUG -O0". <br>
ASAN : Address sanitizer flags "-fsanitize=address". Not compatible with Valgrind. <br>
OPTIMIZED : (Default) Compiles using "-O3" and disables all debuggers and symbols.

PROG : program base file name. "kaelifecpp" by default <br>
SRC_DIR : Path to main() function .cpp source. Default is "./src"

Or you can run cmake manually. The script generates and runs this by default.
```
cmake ./CMakeLists.txt -DBUILD=ALL -DTYPE=OPTIMIZED -DSRC_DIR=./src -DPROG=kaelifecpp
make
```

And if the program builds successfully you can run it wit this command. <br>
Make sure to run it from root folder as the program has to link GL shader files.
```
./build/kaelifecpp_OPTIMIZED
```

----------------------------------------------------------------------------------------------

## Source Files
```
src
kaelifecpp.cpp                Generalized 8-bit cellular automata c++ https://github.com/Kaelygon/kaelifecpp/

include    
    kaelife.hpp               Global namespace kaelife::
    kaelifeBMPIO.hpp          TODO: Import and export world state to bitmap
    kaelifeConfigIO.hpp       JSON Config parser and MasterConfig struct in ConfigHandler class
    kaelifeControls.hpp       Manage SDL2 user input
    kaelifeRender.hpp         OpenGL CA render all world cells in cellState[][][]
    kaelifeSDL.hpp            Initialize SDL2 window and GLEW
    kaelifeWorldCore.hpp      Main thread CA iteration managing loop
    kaelifeWorldMatrix.hpp    2D rectangle std::vector in world orientation and transform
    kaelRandom.hpp            Fast pseudo randomizers and hashers

include/CA
    kaelifeCABacklog.hpp      CAData Backlog thread critical tasks and execute them later
    kaelifeCACache.hpp        CAData Thread cache and copy
    kaelifeCAData.hpp         Manages and iterates cellState that holds CA cell states
    kaelifeCADraw.hpp         CAData Convert mouse press points to pixels to be updated in cellState[][][]
    kaelifeCALock.hpp         CAData thread locks
    kaelifeCAPreset.hpp       CAData preset manager


./shader/fragment.fs.glsl     glsl fragment shader
./shader/vertex.vs.glsl       glsl vertex shader

```