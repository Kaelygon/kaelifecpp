//kaelifeCABacklog.hpp
//Backlog thread critical tasks and execute them later

#pragma once

#include "kaelife.hpp"
#include "kaelifeCAPreset.hpp"
#include "kaelifeCACache.hpp"
#include "kaelifeCADraw.hpp"
#include "kaelifeCAData.hpp"

#include <iostream>
#include <cmath>
#include <numeric>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <cstring>
#include <array>

class CABacklog {
public:
	CABacklog(CAData& inCaData);

	void doBacklog();
	void add(const char* keyword);

private:
	//function wrapper
	typedef bool (CABacklog::*CAB_func)();

	struct funcMap {
		const char* id;
		CAB_func func;
	};

	std::mutex mtx;
	std::vector<const char*> list; // tasks to do that are not thread safe

	CAData& caData;
	CAPreset& kaePreset;
	CADraw& kaeDraw;

	bool CAB_cloneBuffer();
	bool CAB_loadPreset();
	bool CAB_cursorDraw();
	bool CAB_randAll();
	bool CAB_randAdd();
	bool CAB_randRange();
	bool CAB_randMask();
	bool CAB_randMutate();

	std::vector<funcMap> keywordMap = {
		{"cloneBuffer", &CABacklog::CAB_cloneBuffer	},
		{"loadPreset", 	&CABacklog::CAB_loadPreset	},
		{"cursorDraw", 	&CABacklog::CAB_cursorDraw	},
		{"randAll", 	&CABacklog::CAB_randAll		},
		{"randAdd", 	&CABacklog::CAB_randAdd		},
		{"randRange", 	&CABacklog::CAB_randRange	},
		{"randMask", 	&CABacklog::CAB_randMask	},
		{"randMutate", 	&CABacklog::CAB_randMutate	}
	};
};


//constructor
CABacklog::CABacklog(
		CAData& inCaData
	) : 
		caData(inCaData),
		kaePreset(inCaData.kaePreset),
		kaeDraw(inCaData.kaeDraw) {	}

//Functions
//Each function returns boolean wether cloneBuffer needs to be called
bool CABacklog::CAB_cloneBuffer(){
	return true;
}
bool CABacklog::CAB_loadPreset(){
	caData.loadPreset();
	kaePreset.printPreset();
	return true;
}
bool CABacklog::CAB_cursorDraw(){
	bool didCopy = kaeDraw.copyDrawBuf(caData.cellState[!caData.mainCache.activeBuf], caData.mainCache); 
	return didCopy;
}
bool CABacklog::CAB_randAll(){
	auto copyIndex = kaePreset.copyPreset((std::string)"RANDOM",kaePreset.index);
	kaePreset.setPreset(copyIndex[0]);
	uint64_t randSeed = kaePreset.randAll(copyIndex[0]);
	caData.randState(kaePreset.current()->stateCount);
	caData.loadPreset();
	kaePreset.printPreset();
	printf("RANDOM seed: %lu\n",(uint64_t)randSeed);
	return true;
}
bool CABacklog::CAB_randAdd(){
	auto copyIndex = kaePreset.copyPreset((std::string)"RANDOM",kaePreset.index);
	kaePreset.setPreset(copyIndex[0]);
	kaePreset.randRuleAdd(kaePreset.index);
	caData.loadPreset();
	kaePreset.printRuleAdd(kaePreset.index);
	return true;
}
bool CABacklog::CAB_randRange(){
	auto copyIndex = kaePreset.copyPreset((std::string)"RANDOM",kaePreset.index);
	kaePreset.setPreset(copyIndex[0]);
	kaePreset.randRuleRange(kaePreset.index,0);
	caData.loadPreset();
	kaePreset.printRuleRange(kaePreset.index);	
	return true;
}
bool CABacklog::CAB_randMask(){
	auto copyIndex = kaePreset.copyPreset((std::string)"RANDOM",kaePreset.index);
	kaePreset.setPreset(copyIndex[0]);
	kaePreset.randRuleMask(kaePreset.index);
	caData.loadPreset();
	kaePreset.printRuleMask(kaePreset.index);
	return true;
}
bool CABacklog::CAB_randMutate(){
	auto copyIndex = kaePreset.copyPreset((std::string)"RANDOM",kaePreset.index);
	kaePreset.setPreset(copyIndex[0]);
	kaePreset.randRuleMutate(kaePreset.index);
	caData.loadPreset();
	kaePreset.printRuleRange(kaePreset.index);
	kaePreset.printRuleAdd(kaePreset.index);
	return true;
}


//execute before cloneBuffer not-thread safe functions backlog
//writes must happen in !activeBuf
void CABacklog::add(const char* keyword) {
	std::lock_guard<std::mutex> lock(mtx);

	// Check if already backlogged
	auto it = std::find(list.begin(), list.end(), keyword);
	if (it == list.end()) {
		list.push_back(keyword);
	}
}

void CABacklog::doBacklog() {
	std::lock_guard<std::mutex> lock(mtx);

	if (list.empty()) {
		return;
	}

	// Flag to prevent cloning buffer every single keyword
	uint cloneBufferRequest = 0;

	do {
		const char* keyword = list.back();
		list.pop_back();

		// Find the waveform function pointers based on the string identifier
		CAB_func selectedFunction = NULL;
		for (size_t i = 0; i < keywordMap.size(); ++i) {
			if (strcmp(keyword, keywordMap[i].id) == 0) {
				selectedFunction = keywordMap[i].func;
				break;
			}
		}
		if(selectedFunction==NULL){
			printf("Invalid backlog key!\n");
			continue;
		}
		cloneBufferRequest = (this->*selectedFunction)();

	}while (!list.empty());

	list.clear();

	if (cloneBufferRequest) {
		caData.cloneBuffer();
	}
}