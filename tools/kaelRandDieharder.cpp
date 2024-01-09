//$(ls ./build/kaelRandDieharder* | grep -v '\.o$') | dieharder -B -g 200 -a
//$(ls ./build/kaelRandDieharder* | grep -v '\.o$') | pv > /dev/zero
#include <iostream>
#include <thread>
#include <vector>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <chrono>
#include <string>
#include <signal.h>
#include <stdio.h>

bool QUIT_FLAG=0;
void sigint(int a)
{
    printf("Pressed ^C\n");
    QUIT_FLAG=1;
}

#include "kaelRandom.hpp"

const int THREAD_COUNT = 24;
const int BUFFER_SIZE = 10000000;

//typedef uint64_t randType;
typedef uint32_t randType;

kaelRandom<randType> randomizer;

void generateRandomData(randType* resultBuffer, int bufferSize, int threadId, randType start) {
    randType n = threadId + start;
    for (int i = 0; i < bufferSize; ++i) {
         n = randomizer(&n);
        //n = randomizer.shuffleLCG(n);
        resultBuffer[i] = n;
    }
}


void generateRandomData2(randType* resultBuffer, int bufferSize, int threadId, randType start) {

    uint wordSize = sizeof(randType)+1; //+1 null terminate
    char* tmpWord = new char[wordSize];
    randType num = (start*threadId*80131)^932517113;
    kaelRandom<randType>::ShufflePair<randType, randType> hashPair( &num );
    
    randType n = threadId + start;
    for (int i = 0; i < bufferSize; ++i) {
        for(uint i=0;i<wordSize;i++){
            char letter = ((num>>(8*i))&0b11111111);
            letter += letter=='\0';
            tmpWord[i]=letter;  //get letters
        }
        randomizer.shuffle(hashPair);

        tmpWord[wordSize-1]='\0';
        
        n = randomizer.hashCstr(tmpWord);
        //n = std::hash<std::string>{}(tmpWord);
        resultBuffer[i] = n;

        //std::cout << " " << tmpWord << " " << n << "\n";
    }
    
    delete [] tmpWord;

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
    
    signal(SIGINT, sigint);
    for (;;) {
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
        if(QUIT_FLAG){
            break;
        }
    }

    for (size_t i = 0; i < threads.size(); ++i) {
        auto& thread = threads[i];
        if (thread.joinable()) {
            thread.join();
        }
    }

    // Cleanup
    for (size_t i = 0; i < buffers.size(); ++i) {
        delete[] buffers[i];
    }
    printf("Exit\n");

    return 0;
}
