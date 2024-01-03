
#pragma once
#include <iostream>
#include <vector>
#include <cstdint>
#include <cmath>
#include <cmath>
#include <limits>

//Simple rorr lcg prng
template <typename T = uint64_t>
class kaelRandom {
public:
    static_assert(std::is_unsigned<T>::value, "Template type must be an unsigned integer type");

	//iterate kaelRand() seed
		//definitely not thread safe
		inline const T operator()() { 
			seed = kaelLCG(seed);
			return seed;
		}
		void setSeed(T n) {
			seed = n;
		}
		//potentially not thread safe
		//if input *n is nullptr, return kaelRand() seed ptr. Otherwise return *n unchanged 
		inline T* getSeedPtr(T *n){
			if(n==nullptr){
				return &seed;
			}
			return n;
		}
		//Pointer to kaelRand() seed. Not thread safe to modify
		inline T* lastValue() {
			return &seed;
		}
	//

	//thread safe as we are iterating the given argument value or ptr. Unless the pointer is seed
		template <typename U = T, typename = std::enable_if_t<!std::is_reference<U>::value>>
		inline const T operator()(T n) {
			n = kaelLCG(n);
			return n;
		}
		inline const T operator()(T &n) {
			n = kaelLCG(n);
			return n;
		}
		template <typename U = T, typename = std::enable_if_t<std::is_pointer<U>::value>>
		inline const T operator()(U n) {
			*n = kaelLCG(*n);
			return *n;
		}
	//

private:
	inline const T kaelLCG(T n) {
		constexpr int bitSize = std::numeric_limits<T>::digits;
		uint lsb = n&0b111;
		uint rorrN = bitSize * lsb + 1;
		return ((n >> rorrN) | (n << (bitSize - rorrN))) * mul + add;
	}

	uint64_t seed;
	uint64_t mul=3083;
	uint64_t add=13238717;
};
kaelRandom kaelRand;
