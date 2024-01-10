#!/bin/bash
#CMakeBuild.sh [ALL,ACTIVE] [DEBUG, ASAN, OPTIMIZED] [PATH TO .cpp] [.cpp BASENAME]
BUILD="${1:-ALL}" #ALL builds every .cpp source in SRC_DIR. ACTIVE builds single source ${PROG}.cpp
TYPE="${2:-OPTIMIZED}" #Type of build: DEBUG, ASAN, OPTIMIZED. GCC debug/optimization flags
SRC_DIR="${3:-./src}" #path to .cpp source. Uses ./include as .hpp directory
PROG="${4:-kaelifecpp}" #Base name of .cpp file if using ACTIVE.
CONFIG=./CMakeLists.txt

echo cmake "${CONFIG} -DBUILD="${BUILD}" -DTYPE="${TYPE}" -DSRC_DIR="${SRC_DIR}" -DPROG="${PROG}
cmake "${CONFIG}" -DBUILD="${BUILD}" -DTYPE="${TYPE}" -DSRC_DIR="${SRC_DIR}" -DPROG="${PROG}"

make #VERBOSE=1
