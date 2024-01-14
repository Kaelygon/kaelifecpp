//kaelRandom.hpp
//Fast pseudo randomizers and hashers

#pragma once
#include <iostream>
#include <vector>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <climits>
#include <algorithm>

#include <type_traits>

template <typename InsT = uint64_t> //instance type
//Kaelygon randomizer and hash functions
class KaelRandom {
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
	KaelRandom(){ isUint<InsT>(); }

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
		inline InsT* validSeedPtr(InsT *n){
			return n==nullptr ? &seed : n;
		}
		//Pointer to instance seed. Not thread safe to modify. No null check.
		inline InsT* getSeedPtr() {
			return &seed;
		}
	//

	//thread safe as we are iterating the given argument value or ptr. Unless the pointer is instance seed
		//iterate literal (thread safe)
		template <typename U = InsT, typename = std::enable_if_t<!std::is_pointer<std::remove_reference_t<U>>::value>>
		inline const InsT operator()(const U& n) { 
			return kaelLCG(n);
		}
		//Iterate pointer
		template <typename U = InsT, typename = std::enable_if_t<std::is_pointer<std::remove_reference_t<U>>::value>>
		inline const InsT operator()(U n) { 
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

		template <typename X = InsT, typename Y = InsT>
		inline InsT shuffle(X *seed, Y step=0) {
			Y *stepPtr = step==0 ? nullptr : &step;
			ShufflePair<X,Y> bufPair( seed, stepPtr );
			return *kaelShfl( bufPair ).seed;
		}
	//

public:
	template <typename V = InsT>
	inline V RORR(const V num, const size_t shift, const size_t invShift ){
		return (num>>shift) | (num<<invShift);
	}
	template <typename V = InsT>
	inline V RORR(const V num, const size_t shift ){
		const V invShift = sizeof(V)*CHAR_BIT - shift;
		return (num>>shift) | (num<<invShift);
	}

	InsT seed=0;
	static constexpr const InsT mulInd = log2(sizeof(InsT)*8)-3; //uint8=1 uint16=2 uint32=3...
	static constexpr const InsT addInd = log2(sizeof(InsT)*8)-3;

/*
	other uint32 candidates
	high period, good randomness 
	(3343, 11770513)=2^32-162245	(3457, 11774779)	(3457, 11747231)	(3433, 11860031)
	high period, less random
	(173, 103) (349, 103) (421, 127) (971, 139)
*/
	static constexpr const InsT mulArray[] = { (InsT)47, (InsT)365, (InsT)    3343 };
	static constexpr const InsT addArray[] = { (InsT)7 , (InsT)921, (InsT)11770513 };

	static constexpr const InsT mul = mulArray[std::min( (InsT)mulInd, (InsT)(sizeof(mulArray)/sizeof(mulArray[0]) - 1 ) )]; //choose last element if bigger than array size
	static constexpr const InsT add = addArray[std::min( (InsT)addInd, (InsT)(sizeof(addArray)/sizeof(addArray[0]) - 1 ) )];

 	//Generate pseudo random numbers RORR LCG
	//Periods: ui8 = 2^8-1, ui16 = 2^16-1, ui32 = 2^32-162245
	template <typename V = InsT>
	inline const InsT kaelLCG(V n, V mul2=mul, V add2=add) {	isUint<V>();
		static constexpr const V bitSize = sizeof(V)*CHAR_BIT;
		static constexpr const V shift = (sizeof(V)*3)-1;
		static constexpr const V invShift = bitSize-shift;

		return RORR(n,shift,invShift) * mul2 + add2;
	}

	static constexpr const InsT prm35 =(InsT)(mul*add+126U);
 	//Evenly distributed hash 
	template <typename V = InsT>
	V kaelHash(const char* cstr) {	isUint<V>();
		static constexpr const V shift = CHAR_BIT;

		InsT hash=prm35;
		InsT step=0; //starting step 0 reduces collisions significantly
		ShufflePair<InsT,InsT> bufPair = {&hash, &step};

		InsT charVal;
		while(true){ //iterate through every char until null termination
			charVal = (uint8_t)cstr[0];
			hash+=charVal;
			hash = *shuffle(bufPair).seed; //shuffle hash
			cstr++;
			if(!*cstr){ break; } //RORR is redundant if there's no characters left to mix
			hash = RORR(hash,shift); //RORR byte
		}
		hash = kaelLCG(hash); //randomize hash

		return (V)hash;
	}

	//Bijective shuffle function. Mod 4 is predictable
	template <typename X = InsT, typename Y = InsT>
	ShufflePair<X, Y>& kaelShfl(ShufflePair<X, Y>& shufflePair) {	isUint<X,Y>();
		constexpr const X a0 = (sizeof(X) == 1) ? 4 	: 4		; //Some values increase periodicity greatly
		constexpr const X a1 = (sizeof(X) == 1) ? 31 	: 6619	;
		constexpr const Y b0 = (sizeof(X) == 1) ? 13 	: 3083 ;
		constexpr const Y b1 = (sizeof(X) == 1) ? 7 	: 5419 ;

		*shufflePair.step &= (~0b1); //has to be even for the next check to work
		*shufflePair.step += ( (*shufflePair.step & 3) == 2) || ((*shufflePair.step & 3) == 3) ? 2 : 0; //skip any step congruent to 2 or 3 of mod 4 
		
		*shufflePair.seed = *shufflePair.seed * ( (*shufflePair.step|0b1)+a0 ) + a1; //LCG
		*shufflePair.step = *shufflePair.step * b0+b1; //some values have longer periods of m up to 1 quarter of m total states available

		return shufflePair;
	}

};