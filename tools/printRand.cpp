



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

typedef uint16_t randType;
//typedef uint128_t randType;

int main() {
	kaelRandom <randType>randomizer;

	randType n = 0;

	for(int i=0;i<=randType(-1);i++){
		n=randomizer.testing(&n);
		printf("%d\n",n);
	}

    return 0;
}
