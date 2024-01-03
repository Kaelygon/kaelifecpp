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

class CAPreset {
public:
	CAPreset() {
		setPreset(0);
        for (auto& automata : list) {
            if (automata.presetSeed != UINT64_MAX) {
                uint ind = getPresetIndex(automata.name);
                uint64_t* seedPtr = &automata.presetSeed;
                randAll(ind, seedPtr);
            }
        }
	}

	struct PresetList {
		const char* name;
		uint stateCount;
		std::vector<int16_t> ruleRange;
		std::vector<int8_t> ruleAdd;
		uint8_t clipTreshold;
		WorldMatrix<uint8_t> neigMask;
		uint64_t presetSeed;

		// Constructor to set clipTreshold to half of stateCount
		PresetList(
				const char* n = "UNNAMED", 
				uint sc = 0, 
				const std::vector<int16_t>& rr = {0},
				const std::vector<int8_t>& ra = {0, 0},
				const WorldMatrix<uint8_t>& m =
				{
					{UINT8_MAX, UINT8_MAX, UINT8_MAX},
					{UINT8_MAX, 		0, UINT8_MAX},
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
		uint setPreset(uint ind=0){
			if(ind>=list.size()){
				index=list.size()-1+list.empty();
			}else if(ind>=UINT64_MAX/2){
				index=0;
			}else{
				index=ind;
			}
			return index;
		}
		uint nextPreset(){
			index++;
			return setPreset(index);
		}
		uint prevPreset(){
			index--;
			return setPreset(index);
		}

		//add to preset and return its index
		uint addPreset(PresetList preset){
			list.push_back(preset);
			return list.size()-1;
		}

		template <typename T1, typename T2,
				typename = std::enable_if_t<std::is_same<T1, const char*>::value || std::is_same<T1, uint>::value>,
				typename = std::enable_if_t<std::is_same<T2, const char*>::value || std::is_same<T2, uint>::value>>
		//{ uint OR const char* dst, uint OR const char* src, bool keepName=true } 
		std::array<uint, 2> copyPreset(T1 dst, T2 src, bool keepName=true) {

			uint srcInd = -1;
			uint dstInd = -1;
			
			dstInd=getPresetIndex(dst);
			srcInd=getPresetIndex(src);

			if( srcInd==UINT_MAX || dstInd==UINT_MAX ){ //null check
				printf("Destination or source preset doesn't exist!\n");
				return {srcInd,dstInd};
			}

			const char* presetName=list[srcInd].name;
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
	//EOF preset managing func

		//{ uint OR const char*, const char* } 
		template <typename T1, typename = std::enable_if_t<std::is_same<T1, const char*>::value || std::is_same<T1, uint>::value>>
		uint renamePreset(T1 presetNameOrId, const char* presetName="UNNAMED"){
			uint ind = -1;
				ind=getPresetIndex(presetNameOrId);
			if( ind==UINT_MAX ){ //null check
				printf("Preset doesn't exist!\n");
				return ind;
			}
			list[ind].name=presetName;
			return ind;
		}

	//BOF Printers
	
		//print ruleRange[]
		void printRuleRange(uint ind=UINT_MAX){
			ind = ind==UINT_MAX ? index : ind;
			printf("ruleRange {");
			for(uint i=0;i<list[ind].ruleRange.size();i++){
				printf("%d",list[ind].ruleRange[i]);
				if(i!=list[ind].ruleRange.size()-1){
					printf(",");
				}
			}
			printf("},\n");
		}

		//print ruleAdd[]
		void printRuleAdd(uint ind=UINT_MAX){
			ind = ind==UINT_MAX ? index : ind;
			printf(".ruleAdd {");
			for(uint i=0;i<list[ind].ruleAdd.size();i++){
				printf("%d",list[ind].ruleAdd[i]);
				if(i!=list[ind].ruleAdd.size()-1){
					printf(",");
				}
			}
			printf("},\n");
		}

		void printRuleMask(uint ind=UINT_MAX){
			ind = ind==UINT_MAX ? index : ind;
			printf(".neigMask{\n");
			list[ind].neigMask.printCodeSpace();
			printf("},\n");
		}

		void printPreset(uint ind=UINT_MAX){
			ind = ind==UINT_MAX ? index : ind;
			printf("\n");
			printf(list[ind].name);
			printf("\n");
			printRuleRange	(ind);
			printRuleAdd	(ind);
			printRuleMask	(ind);
			printf("State Count: %d\n",list[ind].stateCount);
			if(list[ind].presetSeed!=UINT64_MAX){printf("Seed: %lu\n",list[ind].presetSeed);}
		}
	//EOF Printers

	//BOF Randomizers
		//randomize neigMask[!activeBuf]. Not thread safe
		void randRuleMask( uint ind, uint64_t* seed=nullptr ){
			if(list[ind].stateCount==0){return;}
			uint64_t* seedPtr = kaelRand.getSeedPtr(seed);
			for(uint i=0;i<list[ind].neigMask.wmSize();i++){
				for(uint j=0;j<list[ind].neigMask[i].wmSize();j++){
					uint8_t maskValue = ceil ( (((float)(kaelRand(seedPtr)%list[ind].stateCount)) / list[ind].stateCount) * UINT8_MAX );
					list[ind].neigMask[i][j]=maskValue;
				}
			}
		}

		//randomize ruleRange[]
		void randRuleRange(uint ind, uint16_t minValue, uint16_t maxValue=0, uint64_t* seed=nullptr ) {
			maxValue = maxValue ? maxValue :  calcMaxNeigsum(index);
			uint64_t* seedPtr = kaelRand.getSeedPtr(seed);
			uint rangeSize=list[ind].ruleRange.size();
			list[ind].ruleRange.clear();

			for(uint i=0;i< rangeSize; i++){
				uint16_t randnum = kaelRand(seedPtr)%(maxValue-minValue+1)+minValue;
				auto it = std::lower_bound(list[ind].ruleRange.begin(), list[ind].ruleRange.end(), randnum); //sorted random 
				list[ind].ruleRange.insert(it, randnum);
			}
		}

		//randomize ruleAdd[]
		void randRuleAdd(uint ind, int8_t minValue, int8_t maxValue, uint64_t* seed=nullptr ) {
			uint64_t* seedPtr = kaelRand.getSeedPtr(seed);
			int randRange = maxValue-minValue+1;

			for(uint i=0;i<list[ind].ruleAdd.size();i++){
				list[ind].ruleAdd[i]=kaelRand(seedPtr)%randRange+minValue;
			}
		}

		//randomize everything
		uint64_t randAll(uint ind, uint64_t* seed=nullptr){
			uint64_t* seedPtr = kaelRand.getSeedPtr(seed);
			uint64_t startSeed=*seedPtr;
			
			list[ind].stateCount=kaelRand(seedPtr)%(UINT8_MAX-1)+2;
			bool symmetryX=kaelRand(seedPtr)%4;//75% chance
			bool symmetryY=kaelRand(seedPtr)%4;
			uint maxMask=7;
			uint newMaskX=kaelRand(seedPtr)%maxMask+1;
			uint newMaskY=kaelRand(seedPtr)%maxMask+1;

			list[ind].neigMask.wmResize(newMaskX);
			for(uint i=0;i<newMaskX;i++){
				list[ind].neigMask[i].wmResize(newMaskY);
			}

			randRuleMask(ind,seedPtr);

			if(symmetryX){
				for(uint i=0;i<newMaskX/2;i++){
					for(uint j=0;j<newMaskY;j++){
						list[ind].neigMask[i][j]=list[ind].neigMask[newMaskX-i-1][j];
					}
				}
			}
			if(symmetryY){
				for(uint i=0;i<newMaskX;i++){
					for(uint j=0;j<newMaskY/2;j++){
						list[ind].neigMask[i][j]=list[ind].neigMask[i][newMaskY-j-1];
					}
				}
			}
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

		void randRuleMutate(uint ind, uint64_t* seed=nullptr){
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
			for(size_t i=0;i<maxLen;i++){
				add += add==0 ? kaelRand(seedPtr)%3-1 : 0;
				list[ind].ruleRange[i]+=add;
			}
			maxLen = list[ind].ruleAdd.size() * (modif==1 || modif==2) ;
			for(size_t i=0;i<maxLen;i++){
				add += add==0 ? kaelRand(seedPtr)%3-1 : 0;
				list[ind].ruleAdd[i]+=add;
			}
		}
		

		//max possible neighboring cells sum
		int calcMaxNeigsum(uint ind){
			uint maxNeigsum=0;
			for(uint i=0;i< list[ind].neigMask.wmSize(); i++){
				for(uint j=0;j< list[ind].neigMask[i].wmSize(); j++){
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
			"maskTest",
			4,
			{6,9,11,24},
			{-1,1,-1,0,-1},
			{
				{000,128,128,000,},
				{128,255,255,128,},
				{128,064,064,128,},
				{128,255,255,128,},
				{000,128,128,000 }
			}
		},{//5
			"none",
			0,
			{},
			{},
			{{}}
		},{//6
			"random",
			255,
			{172,179,416,648,834,962,1384,1453,1465},
			{31,-91,31,-43,113,-107,7,-9,63,-31},
			{
				{ 255 },
				{ 000 },
				{ 000 },
				{ 000 },
				{ 000 },
				{ 000 },
				{ 000 },
				{ 000 },
				{ 255,255 }
			}
		},{//7
			"Set seed",
			0,
			{},
			{},
			{{}},
			17055218352662962746UL
		}
	}; 

	template <typename T1, typename = std::enable_if_t<std::is_same<T1, const char*>::value || std::is_same<T1, uint>::value>>
	uint getPresetIndex(T1 charOrInd) {

		uint ind = -1;
		if constexpr (std::is_same<T1, uint>::value) {
			ind=charOrInd;
		}else if constexpr (std::is_same<T1, const char*>::value) {
			//search index by comparing names
			for(uint i=0;i<list.size();i++){
				if(strcmp(list[i].name, charOrInd)==0){
					ind=i;
					break;
				}
			}
			if(ind >= list.size()){ind=-1;}
		}else{
			static_assert(false, "Invalid dst type");
		}
		return ind;
	}

};
