#pragma once

#include "./kaelifeWorldMatrix.hpp"
#include "./kaelRandom.hpp"

#include <iostream>
#include <vector>
#include <cstdint>
#include <cmath>
#include <cmath>
#include <limits>
#include <cstring>
#include <algorithm>

class CAPreset {
public:
	CAPreset() {
		setPreset(0);
	}
	
	typedef struct{
		const char* name;
		uint stateCount = 0;
		std::vector<int16_t> ruleRange={0};
		std::vector<int8_t> ruleAdd={0,0};

		WorldMatrix<uint8_t> neigMask={
			{UINT8_MAX, UINT8_MAX, UINT8_MAX},
			{UINT8_MAX,   		0, UINT8_MAX},
			{UINT8_MAX, UINT8_MAX, UINT8_MAX},
		};
	}PresetList;

	//current preset index
	uint index;
	//current preset
	const PresetList* current() const {
		return &list[index];
	}

	//BOF preset managing func
		uint setPreset(uint ind=0){
			index = (ind + list.size()) % list.size();
			return index;
		}
		uint nextPreset(){
			index = (index+1) % list.size();
			return index;
		}
		uint prevPreset(){
			index = (index-1 + list.size()) % list.size();
			return index;
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

			if( srcInd==(uint)-1 || dstInd==(uint)-1 ){ //null check
				printf("Destination or source preset doesn't exist!");
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
			if( ind==(uint)-1 ){ //null check
				printf("Preset doesn't exist!");
				return ind;
			}
			list[ind].name=presetName;
			return ind;
		}

	//BOF Printers
	
		//print ruleRange[]
		void printRuleRange(uint ind=(uint)-1){
			ind = ind==(uint)-1 ? index : ind;
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
		void printRuleAdd(uint ind=(uint)-1){
			ind = ind==(uint)-1 ? index : ind;
			printf(".ruleAdd {");
			for(uint i=0;i<list[ind].ruleAdd.size();i++){
				printf("%d",list[ind].ruleAdd[i]);
				if(i!=list[ind].ruleAdd.size()-1){
					printf(",");
				}
			}
			printf("},\n");
		}

		void printRuleMask(uint ind=(uint)-1){
			ind = ind==(uint)-1 ? index : ind;
			printf(".neigMask{\n");
			list[ind].neigMask.printCodeSpace();
			printf("},\n");
		}

		void printPreset(uint ind=(uint)-1){
			ind = ind==(uint)-1 ? index : ind;
			printf("\n");
			printf(list[ind].name);
			printf("\n");
			printRuleRange	(ind);
			printRuleAdd	(ind);
			printRuleMask	(ind);
			printf("State Count: %d\n",list[ind].stateCount);
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
			
			return startSeed;
		}

		void randRuleMutate(uint ind, uint64_t* seed=nullptr){
			uint64_t* seedPtr = kaelRand.getSeedPtr(seed);
			int add=0;
			if((list[ind].stateCount/2)<2){
				add = kaelRand(seedPtr)%3-1;
			}else{
				add = kaelRand(seedPtr)%(list[ind].stateCount)-list[ind].stateCount/2;
				add/=2;
			}
			for(size_t i=0;i<list[ind].ruleRange.size();i++){
				list[ind].ruleRange[i]+=add;
			}
			for(size_t i=0;i<list[ind].ruleAdd.size();i++){
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

	std::vector<PresetList> list = 
	{
		{//0
			.name = "Kaelife",
			.stateCount = 4,
			.ruleRange = {6,9,11,24},
			.ruleAdd = {-1,1,-1,0,-1},
		},{//1
			.name = "Conway",
			.stateCount = 2,
			.ruleRange = {2,3,4},
			.ruleAdd = {-1,0,1,-1},
		},{//2
			.name = "Hexagon",
			.stateCount = 256,
			.ruleRange {22,101,102,108,176,211,277,337,405,569,679,820,1180,1289,1411,1442},
			.ruleAdd {-68,-50,17,124,111,-62,-30,86,-19,-2,-73,62,-106,75,70,-76,-79},
			.neigMask{
				{  0, 16,255, 16,  0},
				{255, 16,  0, 16,255},
				{ 16,  0,  0,  0, 16},
				{255, 16,  0, 16,255},
				{  0, 16,255, 16,  0}
			}
		},{//3
			.name = "Conway2Bit",
			.stateCount = 4,
			.ruleRange = {4,6,8},
			.ruleAdd = {-1,0,1,-1},
		},{//4
			.name = "maskTest",
			.stateCount = 4,
			.ruleRange = {6,9,11,24},
			.ruleAdd = {-1,1,-1,0,-1},
			.neigMask{
				{000,128,128,000,},
				{128,255,255,128,},
				{128,064,064,128,},
				{128,255,255,128,},
				{000,128,128,000 }
			}
		},{//5
			.name = "none",
			.stateCount = 0,
			.ruleRange = {0},
			.ruleAdd = {0,0},
			.neigMask{
				{0}
			}
		},{//6
			.name = "random",
			.stateCount = 255,
			.ruleRange = {172,179,416,648,834,962,1384,1453,1465},
			.ruleAdd = {31,-91,31,-43,113,-107,7,-9,63,-31},
			.neigMask{
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
		}
	}; 
	
	template <typename T1, typename = std::enable_if_t<std::is_same<T1, const char*>::value || std::is_same<T1, uint>::value>>
	uint getPresetIndex(T1 charOrInd) {

		uint ind = -1;
		if constexpr (std::is_same<T1, uint>::value) {
			ind=charOrInd;
		}else if constexpr (std::is_same<T1, const char*>::value) {
			auto it = std::find_if(
				std::begin(list), std::end(list),
				[charOrInd](const PresetList& elem) {
					return strcmp(elem.name, charOrInd) == 0;
				}
			);

			if (it != std::end(list)) {
				ind=std::distance(std::begin(list), it);
			} else {
				ind=-1;
			}				
		}else{
			static_assert(false, "Invalid dst type");
		}
		return ind;
	}

};