# CMakefile

cmake_minimum_required(VERSION 3.20)
project(kaelifecpp)

string(TOUPPER ${BUILD} BUILD)
string(TOUPPER ${TYPE} TYPE)

if(NOT TYPE) 
	set(TYPE "DEBUG") #use debug flags by default
endif()

if(NOT PROG) 
	set(BUILD "ALL") #If no program specified build all 
endif()

# Find SDL2, OpenGL, and GLEW
find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

#Set base variables
set(CMAKE_CXX_COMPILER "g++" CACHE STRING "C++ Compiler" FORCE)
set(CXX_STD "cxx_std_20")
set(LINK_LIBRARIES SDL2::SDL2 OpenGL::GL GLEW::GLEW) # Use SDL2, OpenGL, and GLEW targets
set(CMAKE_CXX_STANDARD 20)

# Set optimization or debugger flags
set(CMAKE_BUILD_TYPE Debug)
set(OPTIMIZED_FLAGS "-O3") # OPTIMIZED
set(DEBUG_FLAGS "-Wall" "-Wextra" "-pedantic" "-D_GLIBCXX_DEBUG" "-O0") # DEBUG
set(ASAN_FLAGS "-Wall" "-Wextra" "-pedantic" "-D_GLIBCXX_DEBUG") # ASAN

#Set directiories
if(NOT SRC_DIR) 
	set(SRC_DIR "./src")
endif()
if(NOT INCLUDE_DIR) 
	set(INCLUDE_DIR "./include")
endif()
set(BUILD_DIR "./build")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BUILD_DIR})

include_directories(${INCLUDE_DIR})

if(TYPE STREQUAL "ASAN") 
#    set (ASAN_FLAGS "${ASAN_FLAGS}" "-fno-omit-frame-pointer" "-fsanitize=address")
    set (LINK_LIBRARIES "${LINK_LIBRARIES}" "-fno-omit-frame-pointer" "-fsanitize=address")
    set(FLAGS ${ASAN_FLAGS})
    set(CMAKE_BUILD_TYPE ${TYPE})
elseif(TYPE STREQUAL "OPTIMIZED") 
	set(FLAGS ${OPTIMIZED_FLAGS})
    set(CMAKE_BUILD_TYPE ${TYPE})
else()
    set(FLAGS ${DEBUG_FLAGS})
    set(CMAKE_BUILD_TYPE ${TYPE})
endif()


# Function to build a specific source file
function(build_active_source PROG)
    add_executable(${PROG}_${TYPE} ${SRC_DIR}/${PROG}.cpp)
    set_target_properties(${PROG}_${TYPE} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${BUILD_DIR})
    target_compile_options(${PROG}_${TYPE} PRIVATE
        $<$<CONFIG:DEBUG>:${DEBUG_FLAGS}>
        $<$<CONFIG:OPTIMIZED>:${OPTIMIZED_FLAGS}>
        $<$<CONFIG:ASAN>:${ASAN_FLAGS}>
    )
    target_compile_features(${PROG}_${TYPE} PRIVATE ${CXX_STD})  # Set C++ standard
    target_link_libraries(${PROG}_${TYPE} PRIVATE ${LINK_LIBRARIES})
endfunction()

# Function to build all source files
function(build_all_sources)
    file(GLOB_RECURSE SOURCES ${SRC_DIR}/*.cpp)
    foreach(source_file ${SOURCES})
        get_filename_component(target_name ${source_file} NAME_WE)
        add_executable(${target_name}_${TYPE} ${source_file})
        set_target_properties(${target_name}_${TYPE} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${BUILD_DIR})
		target_compile_options(${target_name}_${TYPE} PRIVATE
            $<$<CONFIG:DEBUG>:${DEBUG_FLAGS}>
            $<$<CONFIG:OPTIMIZED>:${OPTIMIZED_FLAGS}>
            $<$<CONFIG:ASAN>:${ASAN_FLAGS}>
        )
        target_compile_features(${target_name}_${TYPE} PRIVATE ${CXX_STD})  # Set C++ standard
        target_link_libraries(${target_name}_${TYPE} PRIVATE ${LINK_LIBRARIES})
    endforeach()
endfunction()

if(BUILD STREQUAL "ACTIVE" AND PROG)
    message("Building ACTIVE")
    build_active_source(${PROG})
elseif(BUILD STREQUAL "ALL")
    message("Building ALL")
    build_all_sources()
endif()

message("Building ${CXX_STD}/${SRC_DIR} ${PROG} using ${CXX} ${FLAGS} Libs ${LINK_LIBRARIES}")