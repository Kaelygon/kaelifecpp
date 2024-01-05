
#pragma once
#include <iostream>
#include <vector>
#include <cstdint>
#include <cmath>
#include <cstring>


#include <type_traits>
typedef unsigned int uint128_t __attribute__((mode(TI)));

//Simple rorr lcg prng
template <typename InsT = uint64_t> //instance type
class kaelRandom {
private:

	template <typename V = uint64_t>
		inline const V kaelLCG(const V n) {
		static constexpr uint const bitSize = std::numeric_limits<V>::digits;
		static constexpr uint const shift = (sizeof(V)*3)-1;
		V buf = 0;
		buf = ( (n>>shift) | (n<<(bitSize-shift)) ) * mul + add;
		return buf ;
	}

	template <typename V = uint64_t>
	V kaelHash(const char* cstr) {
		InsT hash = prm35;
    	while(*cstr){
			hash = (hash*3343+437947) ^ ((V)cstr[0]*46337+3382607);
			hash = kaelLCG<V>( hash );
			cstr++;
		}

		return hash;
	}

	InsT seed;
	static constexpr const InsT mul=(InsT)3083;
	static constexpr const InsT add=(InsT)13238717;
	static constexpr const InsT prm35 =(InsT)mul*add+(InsT)126;

public:
	static_assert(std::is_unsigned<InsT>::value || std::is_same<InsT, uint128_t>::value , "Template type must be an unsigned integer type");

	//iterate instance seed
		//definitely not thread safe
		inline InsT const operator()() { 
			seed = kaelLCG(seed);
			return seed;
		}
		inline void setSeed(InsT n) {
			seed = n;
		}
		//potentially not thread safe
		//Seed pointer null check. If input *n is nullptr, return instance seed ptr. Otherwise return *n unchanged
		inline InsT* getSeedPtr(InsT *n){
			if(n==nullptr){
				return &seed;
			}
			return n;
		}
		//Pointer to instance seed. Not thread safe to modify. No null check.
		inline InsT* getSeedPtr() {
			return &seed;
		}
	//

	//thread safe as we are iterating the given argument value or ptr. Unless the pointer is instance seed
		//iterate literal (thread safe)
		template <typename TL, typename = std::enable_if_t<!std::is_reference<TL>::value && !std::is_pointer<TL>::value>>
		inline const TL operator()(const TL n) {
			return kaelLCG(n);
		}
		//iterate pointer
		template <typename U = InsT, typename V = typename std::remove_pointer<U>::type, typename = std::enable_if_t<std::is_pointer<U>::value>>
		inline const V operator()(const U n) {
			*n = kaelLCG(*n);
			return *n;
		}
	//

	//other randomizers
		template <typename U>
		U hashCstr(const std::string& str) {
			return kaelHash<U>(str.c_str());
		}
		template <typename U>
		U hashCstr(const char* str) {
			return kaelHash<U>(str);
		}


};
kaelRandom kaelRand;