//$(ls ./build/kaelRandDieharder* | grep -v .o) | dieharder -B -g 200 -a
//$(ls ./build/kaelRandDieharder*) | pv > /dev/zero
//$(ls ./build/kaelRandDieharder*) > ./test/random.txt
#include "kaelRandom.hpp"

int main() {
	uint8_t n = 0;
//	for(int i=0;i<256;i++){
	kaelRandom<uint64_t>u8;
	while(1){
		u8();
		fwrite(&(*u8.getSeedPtr()), sizeof(uint64_t), 1, stdout);
//		printf("%u\n",(uint8_t)n);
	}
//	}

    return 0;
}