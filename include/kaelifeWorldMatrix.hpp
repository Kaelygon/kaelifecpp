
#pragma once
#include <iostream>
#include <vector>
#include <cstdint>
#include <cmath>

//vector up-left = {0,0}  down-right = {width, height}
//Compared to
//WorldMatrix down-left = {0,0}  up-right = {width, height}  transposed
/* vector[4][5] to WorldMatrix
vector<T>{
	{1,2,3,4}
	{1,2,}
	{1,2,3}
	{1,2,3,4,5}
}

WorldMatrix<T>{
	{1,1,1,1,0}
	{2,2,2,2,0}
	{3,0,3,3,0}
	{4,0,0,4,0}
	{0,0,0,0,5}
}
//This way the hardcoded vector has same orientation as the world space and potentially eases the mirroring and randomizing the elements
//WorldMatrix is not recommended to be used in tight loops 
//left-right = Rows
//down-up = Columns
*/
//2D vector in world space
//Acts like a 2D vector but the code-space down-up axis is flipped then axes are transposed and padded
template <typename T>
class WorldMatrix {
	private:
	std::vector<std::vector<T>> matrix={{0}};
	std::vector<std::vector<T>> zeroMatrix={{0}};
	size_t width =1;
	size_t height=1;

public:
	WorldMatrix() : matrix{{0}}, width(1), height(1) {}
	
	WorldMatrix(std::initializer_list<std::initializer_list<T>> data) {

		//Find the widest row
		width = 0;
		for (const auto& row : data) {
			width = row.size() > width ? row.size() : width;
		}

		height = data.size();
		matrix.resize(width);
		for(size_t i=0;i<width;i++){
			matrix[i].resize( height );
		}
		
		size_t ri=0;
		size_t ci;
		for (auto& row : data) {
			ci=0;
			for (auto& elem : row) {
				matrix[ci][matrix[ci].size()-ri-1]=elem; //transpose and invert Y
				ci++;
			}
			ri++;
		}
	}

	//get down-up elements
	class RowProxy {
	public:
		RowProxy(std::vector<T>& row, size_t rowSize) : row_(row), rowSize_ (rowSize) {}

		T& operator[]( size_t col ) {
			if ( col >= rowSize_ ) {
				static T nullValue = T(); 
				printf("Invalid WorldMatrix Y [][%lu] \n", col );
				//abort();
				return nullValue;
			}
			return row_[col];
		}

		const T& operator[]( size_t col ) const {
			if ( col >= rowSize_ ) {
				static T nullValue = T(); 
				printf("Invalid WorldMatrix Y [][%lu] \n", col );
				//abort();
				return nullValue;
			}
			return row_[col];
		}

	private:
		std::vector<std::vector<T>> zeroRow = {{0}};
		std::vector<T>& row_= 0;
		size_t rowSize_ = 0;
	};

	RowProxy operator[](size_t row) {
		if ( !width || row >= width ) {
			printf("Invalid WorldMatrix X [%lu][%lu] \n", row, height);
			//abort();
			return RowProxy(zeroMatrix[0], height );
		} // out of range
		return RowProxy(matrix[row], height );
	}

	const RowProxy operator[](size_t row) const {
		if ( !width || row >= width ) {
			static std::vector<T> nullValue = {T()}; 
			printf("Invalid WorldMatrix X [%lu][%lu] \n", row, height);
			//abort();
			return RowProxy(nullValue, height );
		} // out of range
    	return RowProxy(const_cast<std::vector<T>&>(matrix[row]), height );
	}




	//get matrix left-right size
	size_t getWidth() const {
		return width;
	}
	//get matrix[i] down-up size
	size_t getHeight() const {
		return height;
	}
	
	//resize matrix down-up to sizeX
	void setHeight(size_t newHeight) {
		if(width==0 && newHeight!=0){setWidth(1);}
		for(size_t i=0;i<width;i++){
			matrix[i].resize(newHeight);
		}
		height=newHeight;
	}
	
	//resize matrix[i] left-right to sizeY
	void setWidth(size_t newWidth) {
		matrix.reserve(newWidth);
		if(newWidth>width){
			std::vector<T> addCol(width,0);
			for(int i=0;i<(int)newWidth-(int)width;i++){
				matrix.push_back(addCol);
			}
		}else{
			for(int i=0;i<(int)width-(int)newWidth;i++){
				matrix.pop_back();
			}
		}
		width=newWidth;
	}
	
	//Print 2D WorldVector in hard-coded format  
	void printCodeSpace() {
		size_t maxDigits = log10(UINT8_MAX)+1;

		size_t numRows = matrix.size();
		size_t numCols = (numRows > 0) ? matrix[0].size() : 0;

		for (size_t j = 0; j < numCols; ++j) {
			printf("{");
			for (size_t i = 0; i < numRows; ++i) {
				T num = (j < matrix[i].size()) ? matrix[i][numCols-j-1] : 0;
				size_t digits = num > 0 ? static_cast<size_t>(log10((double)num) + 1) : 1;
				for (size_t k = 0; k < maxDigits - digits; ++k) { printf(" ");	}
				printf("%d", num);
				if (i == numRows - 1) {	printf("}"); }
				if (!(i == numRows - 1 && j == numCols - 1)) { printf(","); }
			}
			printf("\n");
		}
	}
};