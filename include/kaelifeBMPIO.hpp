//kaelifeBMPIO.hpp
//TODO: Import and export world state to bitmap

#pragma once
#include <iostream>
#include "kaelifeWorldMatrix.hpp"
#include "kaelifeCAData.hpp"
#include "kaelife.hpp"

namespace kaelife {
	//eventually make this bitmap import export
	void placeHolderDraw(CAData &cellData){
	
		uint rows = cellData.mainCache.tileRows;
		uint cols = cellData.mainCache.tileCols;

			//draw 2 fliers
			WorldMatrix<uint8_t> flier={
				{0,0,1,0},
				{2,2,2,0},
				{0,2,2,3},
				{2,2,2,0},
				{0,3,3,0},
				{0,1,1,0}
			};

			uint posx=rows/2-1;
			uint posy=cols/2-2;
			for(size_t i=0;i<flier.getWidth();i++){
				for(size_t j=0;j<flier.getHeight();j++){
					uint ai = i + posx   ; 
					uint aj = j + posy   ;
					uint bi = i + posx-12;
					uint bj = j + posy-1 ;
					ai=(ai+rows)%rows;
					aj=(aj+cols)%cols;
					bi=(bi+rows)%rows;
					bj=(bj+cols)%cols;
					cellData.cellState[!cellData.mainCache.activeBuf][ai][aj] = flier[i][j]; //write to inactive buffer
					cellData.cellState[!cellData.mainCache.activeBuf][bi][bj] = flier[i][j];
				}
			}

			WorldMatrix<uint8_t> diagonalFlier={
				{2,3,3,0},
				{3,3,1,0},
				{0,2,2,1},
				{3,2,2,3},
				{0,2,2,0},
				{0,3,3,0},
				{0,1,1,0}
			};

			posx=cellData.mainCache.tileRows/2 + 16;
			posy=cellData.mainCache.tileCols/2;
			for(size_t k=0;k<3;k++){
				for(size_t i=0;i<diagonalFlier.getWidth();i++){
					for(size_t j=0;j<diagonalFlier.getHeight();j++){
						int ofx=18-(-4);
						int ofy= 4+(-4);
						ofx=ofx*k+(2*(k==2)-(3)*(k==0));
						ofy=ofy*k-(2*(k==2)-(3)*(k==0));
						ofx=((i+posx+ofx)+rows)%rows;
						ofy=((j+posy+ofy)+cols)%cols;
						cellData.cellState[!cellData.mainCache.activeBuf][ofx][ofy] = diagonalFlier[i][j]; //maybe possible to keep them alive together
					}
				}
			}

		//border test
	//	cellData.cellState[!cellData.mainCache.activeBuf][0][0]=1;
	//	cellData.cellState[!cellData.mainCache.activeBuf][cellData.mainCache.tileRows-1][0]=2;
	//	cellData.cellState[!cellData.mainCache.activeBuf][cellData.witmainCache.tileRows-1][cellData.mainCache.tileCols-1]=3;

		cellData.backlog->add("cloneBuffer");
		cellData.backlog->doBacklog();
	}
}