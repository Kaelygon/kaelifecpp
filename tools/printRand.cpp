#include <iostream>
#include <unordered_set>
#include <cstdint>
#include "kaelRandom.hpp"

uint64_t period=0;

int main() {
	
	typedef uint32_t hashType;

	KaelRandom<hashType> randomizer;

	hashType hashValue=0;   
	hashType hashStep=0;   
    KaelRandom<hashType>::ShufflePair<hashType, hashType> hashPair( &hashValue, &hashStep );

	std::vector<hashType> history;
	uint64_t gib = 1024*1024*1024/8;
	

	uint64_t dupe=0;
	uint64_t randTypeMax = hashType(~0);
	history.reserve(std::min(randTypeMax,gib/4));
	
	uint64_t maxPeriod=0;
	const uint64_t wordSize=sizeof(hashType)+1; //+1 null
	char tmpWord[wordSize]={'\0'};

	randTypeMax=(hashType)(~0);

	for (uint64_t i = 0; i < randTypeMax; ++i) {

		for(int j=0;j<(int)wordSize-1;j++){
			if(tmpWord[j]==0){
				tmpWord[j]=1;
				tmpWord[(j+1)%wordSize]++;
			}
		}
		tmpWord[wordSize-1]='\0';
		
		hashValue = randomizer.hashCstr(tmpWord);

		tmpWord[0]++;
		if (std::find(history.begin(), history.end(), hashValue) != history.end() ) {
			dupe++;
			std::cout << "Duplicate number " << (uint64_t)hashValue << " at " << i << " dupes " << dupe << "\n";
			if(dupe>1){
				break;
			}
		}
		const bool insertE = !(i&(~0xF));
		const bool insertF = i<16;
		if(i<16){
			history.push_back(hashValue);
		}
		period++;
		if((period&0b111111111111111111111111)==0){
			std::cout << tmpWord ;
			std::cout << " hash " << hashValue << " i: " << i << "\n";
		}
	}


	std::cout << "Period " << period ;
	std::cout << "\n";
		

	history.clear();

	return 0;
}
