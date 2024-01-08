#include <iostream>
#include <unordered_set>
#include <cstdint>
#include "kaelRandom.hpp"


int main() {
	
	typedef uint32_t hashType;

	kaelRandom<hashType> randomizer;

	hashType hashValue=0;   
	hashType hashStep=0;   
    kaelRandom<hashType>::ShufflePair<hashType, hashType> hashPair( &hashValue, &hashStep );

	std::unordered_set<hashType> history;
	uint64_t gib = 1024*1024*1024;
	

	uint dupe=0;
	uint64_t randTypeMax = hashType(~0);
	history.reserve(std::min(randTypeMax,gib/4));
	
	uint maxPeriod=0;
	const uint wordSize=4+1; //+1 null
	char tmpWord[wordSize]={'\0'};

	randTypeMax=(hashType)(~0);

	for (uint i = 0; i < randTypeMax; ++i) {

		for(int j=0;j<(int)wordSize-1;j++){
			if(tmpWord[j]==tmpWord[j+1]){
				tmpWord[j]++;
			}
			if(tmpWord[j]==0){
				tmpWord[j]=1;
				tmpWord[(j+1)%wordSize]++;
			}

			//std::cout << (uint)(uint8_t)tmpWord[j] << " " ;
		}
		
		*hashPair.seed = randomizer.hashCstr(tmpWord);

		//randomizer.shuffle(hashPair); // *hashPair.seed is set here
		
		//std::cout << " hash " << (uint)*hashPair.seed << "\n";

		tmpWord[0]++;
		if (history.count(*hashPair.seed) > 0) {
			std::cout << "Duplicate number " << (uint)*hashPair.seed << "\n";
			dupe++;
			if(dupe>32){
				break;
			}
		}
		history.insert(*hashPair.seed);

	}

	if(maxPeriod<history.size()){
		maxPeriod=history.size();
		std::cout << "Period " << maxPeriod ;
		std::cout << "\n";
	}

	history.clear();

	return 0;
}
