#pragma once

#include "kaelifeWorldMatrix.hpp"
#include "kaelRandom.hpp"

#include <iostream>
#include <vector>
#include <cstdint>
#include <cmath>
#include <limits>
#include <cstring>
#include <algorithm>
#include <mutex>

class CAPreset {

public:
	std::mutex indexMutex;

	CAPreset() {
		setPreset(0);
        for (auto& automata : list) {
            if (automata.presetSeed != UINT64_MAX) {
                uint ind = getPresetIndex(automata.name);
                uint64_t* seedPtr = &automata.presetSeed;
                randAll(ind, seedPtr);
            }
			if(strcmp(automata.name.c_str(),"\0")==0){
				automata.name=unsetName;
			}
        }
	}
	
	static constexpr const size_t maxNameLength = 32;
	static constexpr const char* unsetName = "UNNAMED";
	struct PresetList {
    	std::string name;
		uint stateCount;
		std::vector<int16_t> ruleRange;
		std::vector<int8_t> ruleAdd;
		uint8_t clipTreshold;
		WorldMatrix<uint8_t> neigMask;
		uint64_t presetSeed;

		PresetList(
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

	//current preset index
	uint index=0;
	//current preset
	const PresetList* current() const {
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

		//add to preset and return its index
		uint addPreset(PresetList preset){
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

			list[list.size()-1].name=preset.name;
			list.push_back(preset);
			return list.size()-1;
		}

		template <typename T1, typename T2,
				typename = std::enable_if_t<std::is_same<T1, std::string>::value || std::is_same<T1, uint>::value>,
				typename = std::enable_if_t<std::is_same<T2, std::string>::value || std::is_same<T2, uint>::value>>
		//{ uint OR std::string dst, uint OR std::string src, bool keepName=true } 
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

		//{ uint OR std::string, std::string } 
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
		//Returns -1 if no preset was not found by index
		//Returns seed if no preset was not found by name
		//{ uint OR char* } 
		template <typename T1, typename = std::enable_if_t<std::is_same<T1, std::string>::value || std::is_same<T1, uint>::value>>
		uint64_t seedFromName(T1 nameOrId){
			uint ind = getPresetIndex(nameOrId);
			uint64_t interpSeed;
			if constexpr (std::is_same<T1, uint>::value) { //if preset by index
				if(ind==UINT_MAX){ //no preset found by index
					return ind;
				}
				interpSeed = kaelRand.hashCstr<uint64_t>(list[ind].name); //previously reinterpret_cast
			}else{ //if preset by name
				interpSeed = kaelRand.hashCstr<uint64_t>(nameOrId);
			}
			uint64_t *seedPtr = &interpSeed;

			if(ind!=UINT_MAX){ //if a preset was found by name
				interpSeed = randAll(ind, seedPtr); 
			}

			return interpSeed;
		}
	//EOF preset managing func

	//BOF Printers
	
		//print ruleRange[]
		void printRuleRange(const uint ind=UINT_MAX){
			uint tmpind = ind==UINT_MAX ? index : ind;
			printf(".ruleRange {");
			for(uint i=0;i<list[tmpind].ruleRange.size();++i){
				printf("%d",list[tmpind].ruleRange[i]);
				if(i!=list[tmpind].ruleRange.size()-1){
					printf(",");
				}
			}
			printf("},\n");
		}

		//print ruleAdd[]
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

		void printRuleMask(const uint ind=UINT_MAX){
			uint tmpind = ind==UINT_MAX ? index : ind;
			printf(".neigMask{\n");
			list[tmpind].neigMask.printCodeSpace();
			printf("},\n");
		}

		void printPreset(const uint ind=UINT_MAX){
			uint tmpind = ind==UINT_MAX ? index : ind;
			printf("\n");
			printf(list[tmpind].name.c_str());
			printf("\n");
			printRuleRange	(tmpind);
			printRuleAdd	(tmpind);
			printRuleMask	(tmpind);
			printf("State Count: %d\n",list[tmpind].stateCount);
			if(list[tmpind].presetSeed!=UINT64_MAX){printf("Seed: %lu\n",list[tmpind].presetSeed);}
		}
	//EOF Printers

	//BOF Randomizers
		//randomize neigMask[!activeBuf]. Not thread safe
		void randRuleMask( const uint ind, uint64_t* seed=nullptr ){
			if(list[ind].stateCount==0){return;}
			uint64_t* seedPtr = kaelRand.getSeedPtr(seed);

			//symmetrize
			bool symX=kaelRand(seed)%4;//75% chance
			bool symY=kaelRand(seed)%4;

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
					
					uint maskValue = ceil ( (((float)(kaelRand(seedPtr)%list[ind].stateCount)) / list[ind].stateCount) * UINT8_MAX ); //randomized mask value
					maskValue=maskValue*(mwDelta*mhDelta)/(maxDelta)*multi/divid; //scale lower the further from mask center
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

		//randomize ruleRange[]
		void randRuleRange(const uint ind, uint16_t minValue, uint16_t maxValue=0, uint64_t* seed=nullptr ) {
			maxValue = maxValue ? maxValue :  calcMaxNeigsum(ind);
			uint64_t* seedPtr = kaelRand.getSeedPtr(seed);
			uint rangeSize=list[ind].ruleRange.size();
			list[ind].ruleRange.clear();

			uint current=minValue;
			uint i;
			for(i=0; i<rangeSize; ++i){
				uint16_t addMax = 2*(((maxValue)-current+1)/(rangeSize-i)); //divide maxValue, current delta by remaining elements
				addMax+=addMax==0; //prevent divide by 0
				current+= kaelRand(seedPtr)%addMax; //add to next range
				list[ind].ruleRange.push_back(current);
				if(current>maxValue){current=maxValue; break;}  //any element above maxValue isn't used
			}
		}

		//randomize ruleAdd[]
		void randRuleAdd(const uint ind, int8_t minValue, int8_t maxValue, uint64_t* seed=nullptr ) {
			uint64_t* seedPtr = kaelRand.getSeedPtr(seed);
			int randRange = maxValue-minValue+1;

			for(uint i=0;i<list[ind].ruleAdd.size();++i){
				list[ind].ruleAdd[i]=kaelRand(seedPtr)%randRange+minValue;
			}
		}

		//randomize everything
		uint64_t randAll(const uint ind, uint64_t* seed=nullptr){
			uint64_t* seedPtr = kaelRand.getSeedPtr(seed);
			uint64_t startSeed=*seedPtr;
			
			list[ind].stateCount=kaelRand(seedPtr)%(UINT8_MAX-1)+2;
			uint maxMask=7;
			uint newMaskX=kaelRand(seedPtr)%maxMask+1;
			uint newMaskY=kaelRand(seedPtr)%maxMask+1;

			list[ind].neigMask.setWidth(newMaskX);
			list[ind].neigMask.setHeight(newMaskY);

			randRuleMask(ind,seedPtr);

			uint maxNeigSum = calcMaxNeigsum(ind);
			
			uint maxRules=kaelRand(seedPtr)%(std::min(maxNeigSum,(uint)16)+1)+1;
			list[ind].ruleRange.resize(maxRules+1);
			list[ind].ruleAdd.resize(maxRules+2);

			int rr= (kaelRand(seedPtr)%list[ind].stateCount)/2;
			rr=std::clamp(rr,1,127);
			randRuleAdd(ind,-rr,rr,seedPtr);
			randRuleRange(ind,0,maxNeigSum,seedPtr);
			
			list[ind].presetSeed=startSeed;
			return startSeed;
		}

		void randRuleMutate(const uint ind, uint64_t* seed=nullptr){
			uint64_t* seedPtr = kaelRand.getSeedPtr(seed);
			uint8_t modif = *seedPtr&0b11; //0=ruleRange 1=randAdd 2=both

			int add=0;
			if((list[ind].stateCount/2)<2){
				add = kaelRand(seedPtr)%3-1;
			}else{
				add = kaelRand(seedPtr)%(list[ind].stateCount)-list[ind].stateCount/2;
				add/=2;
			}
			
			uint maxLen = list[ind].ruleRange.size() * (modif==0 || modif==2) ;
			for(size_t i=0;i<maxLen;++i){
				add += add==0 ? kaelRand(seedPtr)%3-1 : 0;
				list[ind].ruleRange[i]+=add;
			}
			maxLen = list[ind].ruleAdd.size() * (modif==1 || modif==2) ;
			for(size_t i=0;i<maxLen;++i){
				add += add==0 ? kaelRand(seedPtr)%3-1 : 0;
				list[ind].ruleAdd[i]+=add;
			}
		}
		

		//max possible neighboring cells sum
		int calcMaxNeigsum(const uint ind){
			uint maxNeigsum=0;
			for(uint i=0;i< list[ind].neigMask.getWidth(); ++i){
				for(uint j=0;j< list[ind].neigMask.getHeight(); ++j){
					maxNeigsum+=(uint) (list[ind].stateCount-1) * list[ind].neigMask[i][j]/UINT8_MAX;
				}
			}
			#if KAELIFE_DEBUG
				printf("maxNeigsum %d\n",maxNeigsum);
			#endif
			return maxNeigsum;
		}
	//EOF Randomizers

private:

	//Maybe a different config file for this list. json maybe
	/*List of automata presets

	*/
	std::vector<PresetList> list = 
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
			11548797965274506362UL
		}
	}; 

	template <typename T1, typename = std::enable_if_t<std::is_same<T1, std::string>::value || std::is_same<T1, uint>::value>>
	uint getPresetIndex(T1 charOrInd) {

		uint ind = -1;
		if constexpr (std::is_same<T1, uint>::value) {
			ind=charOrInd;
		}else if constexpr (std::is_same<T1, std::string>::value) {
			//search index by comparing names
			for(uint i=0;i<list.size();++i){
				if(strcmp(list[i].name.c_str(), charOrInd.c_str())==0){
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
