//TODO: JSON Config parser and MasterConfig struct in ConfigHandler class

#include <iostream>
#include <cmath>
#include <numeric>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <cstring>
#include <string.h>

#include <barrier>
#include <syncstream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <array>

class MasterConfig{
	public:
	MasterConfig(const char* configFolder){
		printf(configFolder);
		printf("\n");
	};

};