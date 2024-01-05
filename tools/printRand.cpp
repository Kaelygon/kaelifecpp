



#include <iostream>
#include <thread> 
#include <vector>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <chrono>

#include "kaelRandom.hpp"

const int THREAD_COUNT = 12;
const int BUFFER_SIZE = 10000000;

typedef uint32_t randType;
//typedef uint128_t randType;

int main() {
	kaelRandom <randType>randomizer;

	for(int i=0;i<256;i++){
		printf("%d\n",randomizer());
	}

    return 0;
}
