
#pragma once
#include <iostream>
#include <vector>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <climits>

#include <type_traits>

//Simple rorr lcg prng
template <typename InsT = uint64_t> //instance type
class kaelRandom {
	typedef unsigned int uint128_t __attribute__((mode(TI)));
	template <typename... Types>
	void isUint() {	
		static_assert( 
			((
			std::is_unsigned<std::remove_pointer_t<Types>>::value || 
			std::is_same< std::remove_pointer_t<Types>, std::remove_pointer_t<uint128_t> >::value
			) && ...) , "All type must be an unsigned integer type"
		); 
	}
public:
	kaelRandom(){ isUint<InsT>(); }

	//Store pointers to be used and modified in kaelShfl
	//If no pointer is give, private variables are used
	template <typename X = InsT, typename Y = InsT>
	struct ShufflePair {
		X* seed;
		Y* step;
		ShufflePair(X* seedInit=nullptr, Y* stepInit=nullptr)
			: seed(seedInit), step(stepInit) {	
				seed = seedInit==nullptr ? &privateSeed : seedInit;
				step = stepInit==nullptr ? &privateStep : stepInit;
		}

		private:
			static constexpr const X defaultSeed = (sizeof(X)==1) ? (X)125 : (X)200 ;
			static constexpr const Y defaultStep = (sizeof(Y)==1) ? (Y)125 : (Y)200 ;	
			X privateSeed = defaultSeed;
			Y privateStep = defaultStep;
	};

private:

	InsT seed;
	static constexpr const InsT mul= (InsT) (sizeof(InsT) == 1 ? 235 : 913	 	);
	static constexpr const InsT add= (InsT) (sizeof(InsT) == 1 ? 191 : 13238717 );
 	//Generate pseudo random numbers RORR LCG
	template <typename V = InsT>
	inline const InsT kaelLCG(const V n) {	isUint<V>();
		static constexpr const uint bitSize = sizeof(V)*CHAR_BIT;
		static constexpr const uint shift = (sizeof(V)*3)-1;
		static constexpr const uint invShift = bitSize-shift;

		V buf = 0;
		buf = ( (n>>shift) | (n<<invShift) ) * mul + add;
		return buf ;
	}

	static constexpr const InsT prm35 =(InsT)(mul*add+126U);
 	//Evenly distributed hash 
	template <typename V = InsT>
	V kaelHash(const char* cstr) {
		static constexpr const uint bitSize = sizeof(V)*CHAR_BIT;
		
		InsT hash=prm35;
		InsT step=(V)14939;
		ShufflePair<InsT,InsT> bufPair = {&hash, &step};

		InsT charVal = (uint8_t)cstr[0];
		while(true){ //iterate through every char until null termination
			hash^=charVal;
			shuffle(bufPair); //shuffle hash
			cstr++;
			if(!*cstr){ break; }
			hash = (hash>>CHAR_BIT) | (hash<<(bitSize-CHAR_BIT)) ; //RORR byte
			charVal = (uint8_t)cstr[0];
		}
		hash = kaelLCG(hash); //randomize hash

		return (V)hash;
	}

	//Bijective shuffle function. Mod 4 is predictable
	template <typename X = InsT, typename Y = InsT>
	ShufflePair<X, Y>& kaelShfl(ShufflePair<X, Y>& shufflePair) {
		constexpr const X a0 = (sizeof(X) == 1) ? 4 	: 4		; //Some values increase periodicity greatly
		constexpr const X a1 = (sizeof(X) == 1) ? 31 	: 6619	;
		constexpr const Y b0 = (sizeof(X) == 1) ? 13 	: 3083	;
		constexpr const Y b1 = (sizeof(X) == 1) ? 7 	: 5419	;

		*shufflePair.step &= (~0b1); //has to be even for the next check to work
		*shufflePair.step += ( (*shufflePair.step & 3) == 2) || ((*shufflePair.step & 3) == 3) ? 2 : 0; //skip any step congruent to 2 or 3 of mod 4 
		
		*shufflePair.seed = *shufflePair.seed * ( (*shufflePair.step|0b1)+a0 ) + a1; //LCG
		*shufflePair.step = *shufflePair.step * b0+b1; //some values have longer periods of m up to 1 quarter of states available

		return shufflePair;
	}


public:

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
		template <typename U = InsT, typename = std::enable_if_t<!std::is_pointer<std::remove_reference_t<U>>::value>>
		inline const InsT operator()(const U& n) { isUint<U>();
			return kaelLCG(n);
		}
		//Iterate pointer
		template <typename U = InsT, typename = std::enable_if_t<std::is_pointer<std::remove_reference_t<U>>::value>>
		inline const InsT operator()(U n) { isUint<U>();
			*n = kaelLCG(*n);
			return *n;
		}
	//

	//other randomizers
		//hash c string
		inline InsT hashCstr(const std::string& str) {
			return kaelHash<InsT>(str.c_str());
		}
		
		inline InsT hashCstr(const char* str) {
			return kaelHash<InsT>(str);
		}

		template <typename X = InsT, typename Y = InsT>
		inline ShufflePair<X, Y>& shuffle(ShufflePair<X, Y>& shflPair) {
			return kaelShfl(shflPair);
		}
	//

};
kaelRandom kaelRand;