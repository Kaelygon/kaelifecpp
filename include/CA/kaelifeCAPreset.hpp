/**
 * @file kaelifeCAPreset.hpp
 * 
 * @brief CAData preset manager
*/

#pragma once

#include "kaelifeSDL.hpp"
#include "kaelRandom.hpp"
#include "kaelife.hpp"
#include "kaelifeWorldMatrix.hpp"

#include <iostream>
#include <vector>
#include <cstdint>
#include <cmath>
#include <limits>
#include <cstring>
#include <algorithm>
#include <mutex>

/**
 * @brief Cellular automata preset manager
*/
class CAPreset {
public:
	static constexpr const size_t maxNameLength = 32;
	static constexpr const char* unsetName = "UNNAMED";

	/**
	 * @brief Cellular automata rules preset
	*/
	struct RulePreset {
    	std::string name;
		uint stateCount;
		std::vector<int16_t> ruleRange;
		std::vector<int8_t> ruleAdd;
		uint8_t clipTreshold;
		WorldMatrix<uint8_t> neigMask;
		uint64_t presetSeed;

		RulePreset(
			std::string n = unsetName,
			uint sc = 0,
			const std::vector<int16_t>& rr = {0},
			const std::vector<int8_t>& ra = {0, 0},
			const WorldMatrix<uint8_t>& m =
			{
				{UINT8_MAX, UINT8_MAX, UINT8_MAX},
				{UINT8_MAX, 0, UINT8_MAX},
				{UINT8_MAX, UINT8_MAX, UINT8_MAX}
			},
			uint64_t ps = UINT64_MAX
		) : 
			name(n), 
			stateCount(sc), 
			ruleRange(rr), 
			ruleAdd(ra), 
			clipTreshold(sc / 2),
			neigMask(m),
			presetSeed(ps)
			{}
	};
	
private:

	//
	/**
	 * @brief List of automata presets
	 * 
	 * TODO: Maybe a different config file for this list. json maybe
	*/
	std::vector<RulePreset> list = 
	{
		{//0
			"Kaelife",
			4,
			{6,9,11,24},
			{-1,1,-1,0,-1}
		},{//1
			"Conway",
			2,
			{2,3,4},
			{-1,0,1,-1}
		},{//2
			"Hexagon",
			256,
			{22,101,102,108,176,211,277,337,405,569,679,820,1180,1289,1411,1442},
			{-68,-50,17,124,111,-62,-30,86,-19,-2,-73,62,-106,75,70,-76,-79},
			{
				{  0, 16,255, 16,  0},
				{255, 16,  0, 16,255},
				{ 16,  0,  0,  0, 16},
				{255, 16,  0, 16,255},
				{  0, 16,255, 16,  0}
			}
		},{//3
			"Conway2Bit",
			4,
			{4,6,8},
			{-1,0,1,-1}
		},{//4
			"none",
			0,
			{0},
			{0},
			{{0}},
		},{//5
			"Set seed",
			0,
			{0},
			{0},
			{{0}},
			15858568242867829530UL
		},{
			"testing",
			8,
			{8,16,24,32,40,48,56},
			{2,-1,1,3,-2,1,-1,-2},
			{
				{ 32, 64,255, 64, 32},
				{255, 16, 96, 16,255},
				{ 64, 96,  0, 96, 64},
				{255, 64, 96, 64,255},
				{ 32, 64,255, 64, 32}
			}
		}
	}; 

public:
	std::mutex indexMutex;
	uint index=0; //current preset index

	CAPreset() {
		setPreset(0);
        for (auto& automata : list) {
            if (automata.presetSeed != UINT64_MAX) {
                uint ind = getPresetIndex(automata.name);
                uint64_t* seedPtr = &automata.presetSeed;
                randAll(ind, seedPtr);
            }
			if( automata.name == "\0" ){
				automata.name=unsetName;
			}
        }
	}

	/**
	 * @return Get pointer to active RulePreset
	*/
	const RulePreset* current() const {
		return &list[index];
	}

	//BOF preset managing func
		uint setPreset(const uint ind=0){
			std::unique_lock<std::mutex> lock(indexMutex); //index may be set by CAData or InputHandler 
			if(ind>=(uint)list.size() && ind < UINT_MAX/2){
				index=0;
			}else if(ind>=UINT_MAX/2){
				index=list.size()-1;
			}else{
				index=ind;
			}
			return index;
		}
		uint nextPreset(){
			return setPreset(index+1);
		}
		uint prevPreset(){
			return setPreset(index-1);
		}

		/**
		 * @brief add to preset and return its index
		 * 
		 * @param preset RulePreset
		*/
		uint addPreset(RulePreset preset){
			preset.name= preset.name=="" ? unsetName : preset.name;

			preset.name.resize(std::min(preset.name.length(), maxNameLength)); //truncate
			size_t nameLen = preset.name.length();

			uint isDupe = getPresetIndex(preset.name); //check if already exists
				
			if(isDupe!=UINT_MAX){
				const size_t ogMaxSfxLen = 4;
				size_t maxSfxLen = std::min(ogMaxSfxLen,nameLen); //clip suffix to name length

				uint sfxLen = 0; //character length of name suffix number e.g. abc3d123 length is 3, suffix value is 123
				//if ascii '0' to '9'
				std::string strNum;
				strNum.resize(maxSfxLen);
				do{
					strNum[sfxLen] = preset.name[nameLen-1-sfxLen];
				}while( isdigit(strNum[sfxLen]) && (++sfxLen < maxSfxLen) );

				if(sfxLen==0){ //if no index but duplicate exists, add suffix 0
					preset.name[ std::min(preset.name.length(),maxNameLength) ]='0';
				}else{ //increment suffix
					strNum.resize(sfxLen);
					std::reverse(strNum.begin(),strNum.end());
					uint sfxValue=stoul(strNum);
					
					if( ((uint)log10(sfxValue+1)!=(uint)log10(sfxValue)) && sfxValue!=0 ){ //if carry 99 -> 100
						if(sfxLen==ogMaxSfxLen){ //overflow 9999 -> 0 only if past original max suffix length
							sfxValue=UINT_MAX;
							sfxLen=0;
							nameLen-=maxSfxLen;
						}
						sfxLen++; nameLen++; // 999 -> 1000
					}
					sfxValue++; //add 1 to the suffix

					preset.name.resize( std::clamp( (size_t)(nameLen-sfxLen), (size_t)0, (size_t)maxNameLength) ); //truncate
					preset.name.append(std::to_string(sfxValue));
				}
			}

//			list[list.size()-1].name=preset.name;
			list.push_back(preset);
			return list.size()-1;
		}

		template <typename T1, typename T2,
				typename = std::enable_if_t<std::is_same<T1, std::string>::value || std::is_same<T1, uint>::value>,
				typename = std::enable_if_t<std::is_same<T2, std::string>::value || std::is_same<T2, uint>::value>>
		/**
		 * @brief Copy a src preset to dst
		 * 
		 * @param dst destination preset. preset index in list[] OR std::string (name)
		 * @param src source preset. preset index in list[] OR std::string (name)
		 * 
		 * @param keepName (optional) Copy everything except dst preset name
		*/
		std::array<uint, 2> copyPreset(T1 dst, T2 src, bool keepName=true) {

			uint srcInd = getPresetIndex(src);
			uint dstInd = getPresetIndex(dst);

			if( srcInd==UINT_MAX || dstInd==UINT_MAX ){ //null check
				printf("Destination or source preset doesn't exist!\n");
				return {srcInd,dstInd};
			}

			std::string presetName=list[srcInd].name;
			if(keepName){
				presetName=list[dstInd].name;
			}

			//if same index, no need to copy
			if(dstInd!=srcInd){
				list[dstInd] = list[srcInd];
			}
			list[dstInd].name=presetName;

			return {dstInd,srcInd};
		}

		/**
		 * @brief rename preset
		 * 
		 * @param nameOrId preset index in list[] OR std::string (name)
		 * 
		*/
		template <typename T1, typename = std::enable_if_t<std::is_same<T1, std::string>::value || std::is_same<T1, uint>::value>>
		uint renamePreset(T1 nameOrId, std::string presetName="UNNAMED"){
			uint ind = getPresetIndex(nameOrId);
			if( ind==UINT_MAX ){ //null check
				printf("Preset doesn't exist!\n");
				return ind;
			}
			list[ind].name=presetName;
			return ind;
		}

		//randomize preset by name or index. 
		/**
		 * @brief convert name to hash and use it as seed for randomizing the preset
		 * 
		 * @param nameOrId preset index in list[] OR std::string (name)
		 * 
		 * @return returns -1 if preset was not found otherwise return seed
		*/
		template <typename T1, typename = std::enable_if_t<std::is_same<T1, std::string>::value || std::is_same<T1, uint>::value>>
		uint64_t seedFromName(T1 nameOrId){
			uint ind = getPresetIndex(nameOrId);
			uint64_t interpSeed;
			if constexpr (std::is_same<T1, uint>::value) { //if preset by index
				if(ind==UINT_MAX){
					printf("Preset not found by index!\n");
					return ind;
				}
				interpSeed = kaelife::rand.hashCstr(list[ind].name); //previously reinterpret_cast
			}else{ //if preset by name
				interpSeed = kaelife::rand.hashCstr(nameOrId);
			}
			uint64_t *seedPtr = &interpSeed;

			if(ind!=UINT_MAX){ //if a preset was found by name
				interpSeed = randAll(ind, seedPtr); 
			}else{
				printf("Preset not found by name!\n");
				return -1;
			}

			return interpSeed;
		}
	//EOF preset managing func

	//BOF Printers
	
		/**
		 * @brief print preset ruleRange[]
		 * 
		 * @param ind preset index. Defaults to current index
		*/
		void printRuleRange(const uint ind=UINT_MAX){
			uint tmpind = ind==UINT_MAX ? index : ind;
			printf(".ruleRange {");
			for(uint i=0;i<list[tmpind].ruleRange.size();++i){
				printf("%u",list[tmpind].ruleRange[i]);
				if(i!=list[tmpind].ruleRange.size()-1){
					printf(",");
				}
			}
			printf("},\n");
		}

		/**
		 * @brief print preset ruleAdd[]
		 * 
		 * @param ind preset index. Defaults to current index
		*/
		void printRuleAdd(const uint ind=UINT_MAX){
			uint tmpind = ind==UINT_MAX ? index : ind;
			printf(".ruleAdd {");
			for(uint i=0;i<list[tmpind].ruleAdd.size();++i){
				printf("%d",list[tmpind].ruleAdd[i]);
				if(i!=list[tmpind].ruleAdd.size()-1){
					printf(",");
				}
			}
			printf("},\n");
		}

		/**
		 * @brief print preset .neigMask.printCodeSpace()
		*/
		void printRuleMask(const uint ind=UINT_MAX){
			uint tmpind = ind==UINT_MAX ? index : ind;
			printf(".neigMask{\n");
			list[tmpind].neigMask.printCodeSpace();
			printf("},\n");
		}

		/**
		 * @brief print all information about preset
		 * 
		 * @param ind preset index. Defaults to current index
		*/
		void printPreset(const uint ind=UINT_MAX){
			uint tmpind = ind==UINT_MAX ? index : ind;		
			printf("\n%s\n",list[tmpind].name.c_str());
			printRuleRange	(tmpind);
			printRuleAdd	(tmpind);
			printRuleMask	(tmpind);
			printf("State Count: %u\n",list[tmpind].stateCount);
			if(kaelife::CA_DEBUG){
				printf("maxNeigsum %u\n",calcMaxNeigsum(ind));
			}
			if(list[tmpind].presetSeed!=UINT64_MAX){printf("Seed: %lu\n",list[tmpind].presetSeed);}
		}
	//EOF Printers

	//BOF Randomizers
		/**
		 * @brief randomize preset neigMask[!activeBuf]
		 * 
		 * @param ind preset index
		 * @param maskX Max mask width. Default 8
		 * @param maskY Max mask Height. Default 8
		 * @param seed seed. Default kaelife::rand() instance seed
		 * 
		*/
		void randRuleMask( const uint ind, uint maskX=0, uint maskY=0, uint64_t* seed=nullptr ){
			const uint cellStates=list[ind].stateCount;
			if(cellStates==0){return;}
			const uint frac256 = ((uint16_t)UINT8_MAX+cellStates-2)/(cellStates-1);
			uint64_t* seedPtr = kaelife::rand.validSeedPtr(seed);
			const uint maxSize=8;
			maskX = maskX==0 ? (kaelife::rand(seedPtr)%maxSize)+1 : maskX;
			maskY = maskY==0 ? (kaelife::rand(seedPtr)%maxSize)+1 : maskY;

			list[ind].neigMask.setWidth(maskX);
			list[ind].neigMask.setHeight(maskY);

			//symmetrize
			bool symX=kaelife::rand(seedPtr)%4;//75% chance
			bool symY=kaelife::rand(seedPtr)%4;

			size_t mw=list[ind].neigMask.getWidth(); 
			size_t mh=list[ind].neigMask.getHeight();
			size_t randWidth = (mw+symX)/(symX+1); //Skip elements that will be replaced by symmetry
			size_t randHeight = (mh+symY)/(symY+1);
			uint maxDelta = mw*mh;

			const uint multi = 4;
			const uint divid = 2;
			for(uint i=0;i<randWidth;++i){
				for(uint j=0;j<randHeight;++j){
					uint mwDelta = mw - abs((int)mw/2-(int)i); //distance from mask center
					uint mhDelta = mh - abs((int)mh/2-(int)j);
					
					uint maskValue = ceil ( (((float)(kaelife::rand(seedPtr)%cellStates)) / cellStates) * UINT8_MAX ); //randomized mask value
					maskValue=maskValue*(mwDelta*mhDelta)/(maxDelta)*multi/divid; //scale lower the further from mask center
					maskValue = (maskValue+frac256-1)/frac256*frac256; //round to nearest cellState multiple scaled to 255
					maskValue = maskValue > UINT8_MAX ? UINT8_MAX : maskValue;
					list[ind].neigMask[i][j]=(uint8_t)maskValue;
				}
			}

			//symmetrize remainding elements 
			for(uint i=randWidth; i<mw; ++i){
				if(!symX){break;}
				for(uint j=0; j<mh; ++j){
					list[ind].neigMask[i][j]=list[ind].neigMask[mw-i-1][j];
				}
			}

			for(uint i=0; i<mw; ++i){
				if(!symY){break;}
				for(uint j=randHeight; j<mh; ++j){
					list[ind].neigMask[i][j]=list[ind].neigMask[i][mh-j-1];
				}
			}

		}

		/**
		 * @brief randomize preset ruleRange[]
		 * 
		 * @param ind preset index
		 * @param minValue min rule range
		 * @param maxValue max rule range
		 * @param seed seed. Default kaelife::rand() instance seed
		*/
		void randRuleRange(const uint ind, uint16_t minValue, uint16_t maxValue=0, uint64_t* seed=nullptr ) {
			maxValue = maxValue ? maxValue :  calcMaxNeigsum(ind);
			uint64_t* seedPtr = kaelife::rand.validSeedPtr(seed);
			uint rangeSize=list[ind].ruleRange.size();
			list[ind].ruleRange.clear();

			uint current=minValue;
			uint i;
			for(i=0; i<rangeSize; ++i){
				uint16_t addMax = 2*(((maxValue)-current+1)/(rangeSize-i)); //divide maxValue, current delta by remaining elements
				addMax+=addMax==0; //prevent divide by 0
				current+= kaelife::rand(seedPtr)%addMax; //add to next range
				list[ind].ruleRange.push_back(current);
				if(current>maxValue){current=maxValue; break;}  //any element above maxValue isn't used
			}
		}


		/**
		 * @brief randomize preset ruleAdd[]
		 * 
		 * @param ind preset index
		 * @param minValue min rule add
		 * @param maxValue max rule add
		 * @param seed seed. Default kaelife::rand() instance seed
		*/
		void randRuleAdd(const uint ind, int8_t minValue=0, int8_t maxValue=0, uint64_t* seed=nullptr ) {
			uint64_t* seedPtr = kaelife::rand.validSeedPtr(seed);

			if(minValue==0 && maxValue==0){
				uint cellStates = list[ind].stateCount/2;
				uint m0 = kaelife::rand(seedPtr)%cellStates;
				uint m1 = kaelife::rand(seedPtr)%cellStates;
				uint medRand = std::sqrt((uint)m0*m1); //geometric mean
				medRand = (uint)std::round( std::lerp<uint,uint,double>(medRand,0,0.25) ); //shift median to first quarter
				medRand = std::clamp(medRand,(uint)1,(uint)127);
				maxValue= medRand;
				minValue=-medRand;
			}
			int randRange = maxValue-minValue+1;

			for(uint i=0;i<list[ind].ruleAdd.size();++i){
				list[ind].ruleAdd[i]=kaelife::rand(seedPtr)%randRange+minValue;
			}
		}

		/**
		 * @brief randomize all CA rules
		 * 
		 * @param ind preset index
		 * @param seed seed. Default kaelife::rand() instance seed
		*/
		uint64_t randAll(const uint ind, uint64_t* seed=nullptr){
			uint64_t* seedPtr = kaelife::rand.validSeedPtr(seed);
			uint64_t startSeed=*seedPtr;
			
			list[ind].stateCount=kaelife::rand(seedPtr)%(UINT8_MAX-1)+2;
			uint maxMask=7;
			uint newMaskX=kaelife::rand(seedPtr)%maxMask+1;
			uint newMaskY=kaelife::rand(seedPtr)%maxMask+1;

			randRuleMask(ind, newMaskX, newMaskY, seedPtr);

			uint maxNeigSum = calcMaxNeigsum(ind);
			
			uint maxRules=kaelife::rand(seedPtr)%(std::min(maxNeigSum,(uint)16)+1)+1;
			list[ind].ruleRange.resize(maxRules+1);
			list[ind].ruleAdd.resize(maxRules+2);

			int rr= (kaelife::rand(seedPtr)%list[ind].stateCount)/2;
			rr=std::clamp(rr,1,127);
			randRuleAdd(ind,-rr,rr,seedPtr);
			randRuleRange(ind,0,maxNeigSum,seedPtr);
			
			list[ind].presetSeed=startSeed;
			return startSeed;
		}

		/**
		 * @brief Modify CA rules slightly
		 * 
		 * @param ind preset index
		 * @param seed seed. Default kaelife::rand() instance seed
		*/
		void randRuleMutate(const uint ind, uint64_t* seed=nullptr){
			uint64_t* seedPtr = kaelife::rand.validSeedPtr(seed);
			uint8_t modif = *seedPtr&0b11; //0=ruleRange 1=randAdd 2=both

			int add=0;
			if((list[ind].stateCount/2)<2){
				add = kaelife::rand(seedPtr)%3-1;
			}else{
				add = kaelife::rand(seedPtr)%(list[ind].stateCount)-list[ind].stateCount/2;
				add/=2;
			}
			
			uint maxLen = list[ind].ruleRange.size() * (modif==0 || modif==2) ;
			for(size_t i=0;i<maxLen;++i){
				add += add==0 ? kaelife::rand(seedPtr)%3-1 : 0;
				list[ind].ruleRange[i]+=add;
			}
			maxLen = list[ind].ruleAdd.size() * (modif==1 || modif==2) ;
			for(size_t i=0;i<maxLen;++i){
				add += add==0 ? kaelife::rand(seedPtr)%3-1 : 0;
				list[ind].ruleAdd[i]+=add;
			}
		}
		

private:

		//
		/**
		 * @brief Calculate max possible neighboring cells sum depending on neigMask and stateCount
		 * 
		 * @param ind preset index
		*/
		int calcMaxNeigsum(const uint ind){
			uint tmpind = ind==UINT_MAX ? index : ind;
			uint maxNeigsum=0;
			for(uint i=0;i< list[tmpind].neigMask.getWidth(); ++i){
				for(uint j=0;j< list[tmpind].neigMask.getHeight(); ++j){
					maxNeigsum+=(uint) (list[tmpind].stateCount-1) * list[tmpind].neigMask[i][j]/UINT8_MAX;
				}
			}
			printf("maxNeigsum %u\n",maxNeigsum);
			return maxNeigsum;
		}
	//EOF Randomizers

	template <typename T1, typename = std::enable_if_t<std::is_same<T1, std::string>::value || std::is_same<T1, uint>::value>>
	/**
	 * @brief Convert 
	 * 
	 * @param charOrInd preset index in list[] OR std::string name
	 * 
	*/
	uint getPresetIndex(T1 charOrInd) {

		uint ind = -1;
		if constexpr (std::is_same<T1, uint>::value) {
			ind=charOrInd;
		}else if constexpr (std::is_same<T1, std::string>::value) {
			//search index by comparing names
			for(uint i=0;i<list.size();++i){
				if( list[i].name == charOrInd ){
					ind=i;
					break;
				}
			}
			if(ind >= list.size()){ind=UINT_MAX;}
		}else{
			static_assert(false, "Invalid dst type");
		}
		return ind;
	}

};
