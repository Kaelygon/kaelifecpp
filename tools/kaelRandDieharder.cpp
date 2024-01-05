//$(ls ./build/kaelRandDieharder* | grep -v '\.o$') | dieharder -B -g 200 -a
//$(ls ./build/kaelRandDieharder* | grep -v '\.o$') | pv > /dev/zero
#include <iostream>
#include <thread>
#include <vector>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <chrono>

#include "kaelRandom.hpp"

const int THREAD_COUNT = 24;
const int BUFFER_SIZE = 10000000;

//typedef uint64_t randType;
typedef uint128_t randType;

kaelRandom<randType> randomizer;

void generateRandomData(randType* buffer, int bufferSize, int threadId, randType start) {
    randType n = threadId + start;
    for (int i = 0; i < bufferSize; ++i) {
        n = randomizer(&n);
        buffer[i] = n;
    }
}

void generateRandomData2(randType* buffer, int bufferSize, int threadId, randType start) {

    uint csize = sizeof(randType);
    char* bufChar = new char[csize];

    randType n = threadId + start;
    for (int i = 0; i < bufferSize; ++i) {
        for(uint i=0;i<csize;i++){
            bufChar[i]=(char)((n>>(8*i))*7+3+n);   
        }
        
        n = randomizer.hashCstr<randType>(bufChar);
        buffer[i] = n;
    }
    
    delete [] bufChar;

}

int main() {
    std::vector<std::thread> threads;
    std::vector<randType*> buffers;

    std::vector<randType> start(THREAD_COUNT,0);

    // Create threads and buffers outside the main loop
    for (int i = 0; i < THREAD_COUNT; ++i) {
        start[i]+=randomizer((randType)(start[i]+i));
        buffers.push_back(new randType[BUFFER_SIZE]);
        threads.emplace_back(generateRandomData, buffers[i], BUFFER_SIZE, i, start[i]);
    }

    while (true) {
        for (size_t i = 0; i < threads.size(); ++i) {
            auto& thread = threads[i];
            if (thread.joinable()) {
                thread.join();
                start[i]+=BUFFER_SIZE+randomizer((randType)(start[i]+i));
                fwrite(reinterpret_cast<const void*>(buffers[i]), sizeof(randType), BUFFER_SIZE, stdout);
                std::memset(buffers[i], 0, BUFFER_SIZE * sizeof(randType));
                threads[i] = std::thread(generateRandomData, buffers[i], BUFFER_SIZE, i, start[i]);
            }
        }
    }

    // Cleanup
    for (size_t i = 0; i < buffers.size(); ++i) {
        delete[] buffers[i];
    }

    return 0;
}
