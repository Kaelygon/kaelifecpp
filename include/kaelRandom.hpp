/**
 * @file kaelRandom.hpp
 * @brief Fast pseudo randomizers and hashers
*/

#pragma once
#include <iostream>
#include <vector>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <climits>
#include <algorithm>
#include <type_traits>

template <typename TRand = uint64_t> //Random type
/**
 * @brief Kaelygon randomizers and hashers
 *
 * The class is template 'TRand' specifies highest default uint width 
 *
 * Example usage:
 * @code
 * KaelRandom<uint64_t> rand;
 * @endcode
 */
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
	KaelRandom(){ isUint<TRand>(); }

	/**
	 * @brief Store pointers to be used and modified in kaelShfl
	 * If no pointer is given, private variables are used
	 * 
	 * @param seedInit starting seed
	 * @param stepInit step progress. optional
	*/
	template <typename X = TRand, typename Y = TRand>
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

	/**
	 * @brief Iterate instance seed. Not thread-safe.
	 *
	 * @return iteration result
	 */
	inline TRand const operator()() { 
		seed = kaelLCG(seed);
		return seed;
	}
	/**
	 * @brief Set instance seed
	 */
	inline void setSeed(TRand n) {
		seed = n;
	}
	/**
	 * @brief Validate seed pointer. If the input pointer is nullptr, return the instance seed pointer.
	 *        Otherwise, return the input pointer unchanged.
	 *        
	 * @note Not thread-safe to modify if the input pointer is nullptr.
	 *
	 * @param seedPtr Pointer to a seed
	 * @return Pointer to a valid seed
	 */
	inline TRand* validSeedPtr(TRand *seedPtr){
		return seedPtr==nullptr ? &seed : seedPtr;
	}
    /**
     * @brief Get the pointer to the instance seed.
     *        
	 * @note Not thread-safe to modify.
     *
     * @return Pointer to the instance seed.
     */
	inline TRand* getSeedPtr() {
		return &seed;
	}

	/**
	 * @brief Iterate literal (thread safe)
	 * @param seed uint literal 
	 *
	 * @return Iterate result
	 */
	template <typename U = TRand, typename = std::enable_if_t<!std::is_pointer<std::remove_reference_t<U>>::value>>
	inline const TRand operator()(const U& seed) { 
		return kaelLCG(seed);
	}
	/**
	 * @brief Iterate pointer
	 * @param seedPtr *uint
	 *
	 * @return Iterated result
	 */
	template <typename U = TRand, typename = std::enable_if_t<std::is_pointer<std::remove_reference_t<U>>::value>>
	inline const TRand operator()(U seedPtr) { 
		*seedPtr = kaelLCG(*seedPtr);
		return *seedPtr;
	}

    /**
     * @brief Hash std::string.
     *
     * @param str Input string.
     * @return Hash result.
     */
	template <typename U = TRand>
	inline U hashCstr(const std::string& str) {
		return kaelHash<U>(str.c_str());
	}
    /**
     * @brief Hash const char*.
     *
     * @param str Input const char*.
     * @return Hash result.
     */
	template <typename U = TRand>
	inline U hashCstr(const char* str) {
		return kaelHash<U>(str);
	}

	/**
	 * @brief Shuffle ShufflePair.seed
	 *
	 * @param shflPair ShufflePair
	 *
	 * @return ShufflePair
	 */
	template <typename X = TRand, typename Y = TRand>
	inline ShufflePair<X, Y>& shuffle(ShufflePair<X, Y>& shflPair) {
		return kaelShfl(shflPair);
	}

	/**
	 * @brief Shuffle *seedPtr
	 *
	 * @param seedPtr Seed pointer
	 * @param step step progress
	 *
	 * @return ShufflePair.seed
	 */
	template <typename X = TRand, typename Y = TRand>
	inline TRand shuffle(X *seedPtr, Y step=0) {
		Y *stepPtr = step==0 ? nullptr : &step;
		ShufflePair<X,Y> bufPair( seedPtr, stepPtr );
		return *kaelShfl( bufPair ).seedPtr;
	}

private:
	//Rotate right x86 instruction
	template <typename V = TRand>
	inline V RORR(const V num, const size_t shift, const size_t invShift ){
		return (num>>shift) | (num<<invShift);
	}
	template <typename V = TRand>
	inline V RORR(const V num, const size_t shift ){
		const V invShift = sizeof(V)*CHAR_BIT - shift;
		return (num>>shift) | (num<<invShift);
	}

	/**
	 * @brief Instance seed
	 * 
	 * @note Not thread safe to modify
	*/
	TRand seed=0;
	
	static constexpr const TRand mulInd = log2(sizeof(TRand)*8)-3; //Constant multiplier and addend index depending on uint width. uint8=1 uint16=2 uint32=3...
	static constexpr const TRand addInd = log2(sizeof(TRand)*8)-3;

/*
	other uint32 candidates
	high period, good randomness 
	(3343, 11770513)=2^32-162245	(3457, 11774779)	(3457, 11747231)	(3433, 11860031)
	high period, less random
	(173, 103) (349, 103) (421, 127) (971, 139)
*/
	//LCG constants
	static constexpr const TRand mulArray[] = { (TRand)47, (TRand)365, (TRand)    3343 };
	static constexpr const TRand addArray[] = { (TRand)7 , (TRand)921, (TRand)11770513 };

	//Choose element depending on TRand width. Select last if index exceeds constants array size
	static constexpr const TRand mul = mulArray[std::min( (TRand)mulInd, (TRand)(sizeof(mulArray)/sizeof(mulArray[0]) - 1 ) )];
	static constexpr const TRand add = addArray[std::min( (TRand)addInd, (TRand)(sizeof(addArray)/sizeof(addArray[0]) - 1 ) )];

 	//Generate pseudo random numbers RORR LCG
	//Periods: ui8 = 2^8-1, ui16 = 2^16-1, ui32 = 2^32-162245, ui64 (tested up to) 2^38
	template <typename V = TRand>
	inline const TRand kaelLCG( V n ) {	isUint<V>();
		static constexpr const V bitSize = sizeof(V)*CHAR_BIT;
		static constexpr const V shift = (sizeof(V)*3)-1;
		static constexpr const V invShift = bitSize-shift;

		return RORR(n,shift,invShift) * mul + add;
	}

	/*
		Unique hashes of strings length of sieof(V): 
		ui8 255, ui16 65250, ui32 ~2^31.89, ui64 (tested up to) 2^34.5
	*/
	static constexpr const TRand prm35 =(TRand)(mul*add+126U); //Some large prime for hasher starting seed
 	//Evenly distributed hash of const char*
	template <typename V = TRand>
	V kaelHash(const char* cstr) {	isUint<V>();
		static constexpr const V shift = CHAR_BIT;

		TRand hash=prm35;
		TRand step=0; //starting step 0 reduces collisions significantly
		ShufflePair<TRand,TRand> bufPair = {&hash, &step};

		TRand charVal;
		while(true){ //iterate through every char until null termination
			charVal = (uint8_t)cstr[0];
			hash+=charVal; //Mix character
			hash = *shuffle(bufPair).seed; //shuffle hash
			cstr++;
			if(!*cstr){ break; } //RORR is redundant if there's no characters left to mix
			hash = RORR(hash,shift); //RORR byte
		}
		hash = kaelLCG(hash); //randomize hash

		return (V)hash;
	}

	//Bijective shuffle function. Mod 4 is predictable
	template <typename X = TRand, typename Y = TRand>
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