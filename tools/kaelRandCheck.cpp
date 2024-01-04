#include "kaelRandom.hpp"

template <typename T = uint64_t>
T useSeed(const T *seed){
	T buf = *seed;
	buf*=23;
	buf+=13;
	return buf;
}

void randState(uint64_t* seed=nullptr ){
	uint64_t* seedPtr = kaelRand.getSeedPtr(seed);
	for(uint i=0;i<3;i++){
		*seedPtr = kaelRand(seedPtr) + i;
	}
}

int main() {
    kaelRandom<uint64_t> kaelRand;

	//iterate literal
	printf("literal %d\n",kaelRand((int)123));

	//iterate kaelRand() instance variable
    for (int i = 0; i < 5; ++i) {
		printf("instance %lu\n",kaelRand());
	}

	//iterate variable
    uint64_t n = 123;
    for (int i = 0; i < 5; ++i) { 
        n = kaelRand(&n);
		printf("variable %lu\n",n);
    }

	//iterate and modify pointer
    uint64_t t = 123;
	uint64_t *tPtr = &t;
    for (int i = 0; i < 2; ++i) { 
		randState(tPtr);
		printf("pointer modify64 %lu\n",*tPtr);
    }

	//mismatching uint widths is possible
    uint32_t m = 123;
	uint32_t *mPtr = &m;
    for (int i = 0; i < 5; ++i) { 
        uint32_t buf = kaelRand(mPtr);
		buf = useSeed(mPtr);
		printf("pointer %u\n",buf);
    }

	//In case seedPtr is null, it still returns valid value using the instance seed
	//Must still share instance type
	uint64_t *someSeedPtr = nullptr;
	uint64_t *newPtr = kaelRand.getSeedPtr(someSeedPtr);
	printf("nullptr %lu\n",kaelRand(newPtr));

	//Valid even if the ptr happens points back to the instance seed
	newPtr = kaelRand.getSeedPtr();
	printf("nullptr %lu\n",kaelRand(newPtr));

    return 0;
}
