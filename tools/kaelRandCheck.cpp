#include "kaelRandom.hpp"

typedef uint64_t randType;
    KaelRandom<randType> randomizer;


template <typename T = randType>
T useSeed(const T *seed){
	T buf = *seed;
	buf*=23;
	buf+=13;
	return buf;
}

void randState(randType* seed=nullptr ){
	randType* seedPtr = randomizer.validSeedPtr(seed);
	for(uint i=0;i<3;i++){
		*seedPtr = randomizer(seedPtr) + i;
	}
}

int main() {

	//iterate literal
	printf("literal %lu\n",randomizer((uint)123));

	//iterate randomizer() instance variable
    for (int i = 0; i < 5; ++i) {
		printf("instance %lu\n",randomizer());
	}

	//iterate variable
    randType n = 123;
    for (int i = 0; i < 5; ++i) { 
        randomizer(&n);
		printf("variable %lu\n",n);
    }

	//iterate and modify pointer
    randType t = 123;
	randType *tPtr = &t;
    for (int i = 0; i < 2; ++i) { 
		randState(tPtr);
		printf("pointer modify64 %lu\n",*tPtr);
    }

	//mismatching uint widths is possible
    uint16_t m = 123;
	uint16_t *mPtr = &m;
    for (int i = 0; i < 5; ++i) { 
        uint16_t buf = randomizer(mPtr);
		buf = useSeed(mPtr);
		printf("pointer %u\n",buf);
    }

	//In case seedPtr is null, it still returns valid value using the instance seed
	//Must still share instance type
	randType *someSeedPtr = nullptr;
	randType *newPtr = randomizer.validSeedPtr(someSeedPtr);
	printf("nullptr %lu\n",randomizer(newPtr));

	//Valid even if the ptr happens points back to the instance seed
	newPtr = randomizer.getSeedPtr();
	printf("nullptr %lu\n",randomizer(newPtr));

	printf( "hash abcd %lu\n", randomizer.hashCstr("abcd") );
	printf( "hash abce %lu\n", randomizer.hashCstr("abce") );
	printf( "hash 223446abb %lu\n", randomizer.hashCstr("223446abb") );
	printf( "hash 223456abb %lu\n", randomizer.hashCstr("223456abb") );
	printf( "hash 123456abb %lu\n", randomizer.hashCstr("123456abb") );
	printf( "hash 123456abc %lu\n", randomizer.hashCstr("123456abc") );
	printf( "hash cstrin 123456abd %lu\n", randomizer.hashCstr("123456abd") );
	printf( "hash string 123456abd %lu\n", randomizer.hashCstr((std::string)"123456abd") );
	printf( "hash 000 %lu\n", randomizer.hashCstr("000") );
	printf( "hash 100 %lu\n", randomizer.hashCstr("100") );
	printf( "hash 001 %lu\n", randomizer.hashCstr("001") );
	printf( "hash \257\257 %lu\n", randomizer.hashCstr("\257\257") );
	printf( "hash \257\256 %lu\n", randomizer.hashCstr("\257\256") );
	printf( "hash null %lu\n", randomizer.hashCstr("\0") );
	printf( "hash 1 null %lu\n", randomizer.hashCstr("\1\0") );

	uint num = 123456789;
	randomizer.shuffle(&num);
	printf( "hash %u\n", num );

    return 0;
}
