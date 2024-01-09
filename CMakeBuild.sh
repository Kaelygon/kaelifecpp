#!/bin/bash

BUILD="${1:-ALL}"
TYPE="${2:-DEBUG}"
SRC_DIR="${3:-./src}"
PROG="${4:-kaelifecpp}"
CONFIG=./CMakeLists.txt

echo cmake "${CONFIG} -DBUILD="${BUILD}" -DTYPE="${TYPE}" -DSRC_DIR="${SRC_DIR}" -DPROG="${PROG}
cmake "${CONFIG}" -DBUILD="${BUILD}" -DTYPE="${TYPE}" -DSRC_DIR="${SRC_DIR}" -DPROG="${PROG}"

make #VERBOSE=1
