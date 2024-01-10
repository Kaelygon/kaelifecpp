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
	typedef struct{
		std::vector<uint16_t> updatedCells[2];
		uint 				 threadId 		= -1; 
		uint 				 threadCount 	= -1;
		std::vector<uint8_t> neigMask1d  	= {}; //flattened neigMask
		//std::vector<size_t> neigMaskInd 	= 0; //non zero neigMask1d indices. slower even if 9 of 25 elements are zero.
		bool				 activeBuf	 	= 0; //active cellState. write to !activeBuf read from activeBuf
		std::vector<int16_t> ruleRange	 	= {0}; //CA add ranges
		std::vector<int8_t>  ruleAdd	 	= {0,0}; //CA additive values within each range
		uint	 			 stateCount	 	= {0}; //number of cell states
		uint8_t 			 maskWidth	 	= 0; //neigMask width
		uint8_t 			 maskHeight	 	= 0; //neigMask height
		uint8_t 			 maskRadx	 	= 0; //x length from center of the mask. Center excluded
		uint8_t 			 maskRady	 	= 0; //y length from center of the mask. Center excluded
		uint16_t 			 maskElements	= 0; //neigmask elements
		uint 			 	 tileRows	 	= 1; //wold space X dimension left to right. Can't be 0 or odd
		uint 			 	 tileCols	 	= 1; //wold space Y dimension down to up. Can't be 0 or odd
		uint8_t 			 clipTreshold	= 0; //CA rule to discard any neighbors below this value
		uint				 iterRepeats	= 0; //iteration thread task size
		size_t				 index		 	= 0; //cache incrementor to check if cache is up to date
	}ThreadCache;

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