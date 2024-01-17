//kaelifeCACache.hpp
//CA Thread cache and copy

#pragma once

#include <iostream>
#include <vector>
#include <cstdint>
#include <cmath>
#include <limits>


//CAData and its iterate threads caches
class CACache {
public:
	CACache() {
		
	}
	
	//clone parent class variables
	struct ThreadCache{
		__attribute__((aligned(64))) std::vector<uint16_t> updatedCells[2];
		__attribute__((aligned(64))) uint 				 threadId 		= -1; 
		__attribute__((aligned(64))) uint 				 threadCount 	= -1;
		__attribute__((aligned(64))) std::vector<uint8_t> neigMask1d  	= {}; //flattened neigMask
		__attribute__((aligned(64))) bool				 activeBuf	 	= 0; //active cellState. write to !activeBuf read from activeBuf
		__attribute__((aligned(64))) std::vector<int16_t> ruleRange	 	= {0}; //CA add ranges
		__attribute__((aligned(64))) std::vector<int8_t>  ruleAdd	 	= {0,0}; //CA additive values within each range
		__attribute__((aligned(64))) uint	 			 stateCount	 	= {0}; //number of cell states
		__attribute__((aligned(64))) uint8_t 			 maskWidth	 	= 0; //neigMask width
		__attribute__((aligned(64))) uint8_t 			 maskHeight	 	= 0; //neigMask height
		__attribute__((aligned(64))) uint8_t 			 maskRadx	 	= 0; //x length from center of the mask. Center excluded
		__attribute__((aligned(64))) uint8_t 			 maskRady	 	= 0; //y length from center of the mask. Center excluded
		__attribute__((aligned(64))) uint16_t 			 maskElements	= 0; //neigmask elements
		__attribute__((aligned(64))) uint 			 	 tileRows	 	= 1; //wold space X dimension left to right. Can't be 0 or odd
		__attribute__((aligned(64))) uint 			 	 tileCols	 	= 1; //wold space Y dimension down to up. Can't be 0 or odd
		__attribute__((aligned(64))) uint8_t 			 clipTreshold	= 0; //CA rule to discard any neighbors below this value
		__attribute__((aligned(64))) uint				 iterRepeats	= 0; //iteration thread task size
		__attribute__((aligned(64))) size_t				 index		 	= 0; //cache incrementor to check if cache is up to date
	};

    // Copy parent variables to thread cache 
    void copyCache(ThreadCache *dst, const ThreadCache &src ) {
		dst->ruleRange		=	src.ruleRange; 
		dst->ruleAdd		=	src.ruleAdd;	 
		dst->stateCount		=	src.stateCount;	
		dst->tileRows		=	src.tileRows;	 
		dst->tileCols		=	src.tileCols;	  
		dst->clipTreshold	=	src.clipTreshold;

		dst->maskRadx		=	src.maskRadx;	 
		dst->maskRady		=	src.maskRady;	 
		dst->maskWidth		=	src.maskWidth;	 
		dst->maskHeight		=	src.maskHeight;	 
		dst->maskElements	=	src.maskElements;
		dst->index			=	src.index;

		dst->neigMask1d.resize(dst->maskElements);
		
		dst->neigMask1d=src.neigMask1d;
		//dst->neigMaskInd=src.neigMaskInd;
    }

};
