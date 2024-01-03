
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
public:
	std::vector<std::vector<T>> matrix;
	
    WorldMatrix() = default;
	WorldMatrix(std::initializer_list<std::initializer_list<T>> data) {

		// Find the largest row size
		size_t largestRow = 0;
		for (const auto& row : data) {
			largestRow = row.size() > largestRow ? row.size() : largestRow;
		}

		// Resize the matrix to the largest row size
		matrix.resize(largestRow);
		
		for(uint i=0;i<largestRow;i++){
			matrix[i].resize( data.size() );
		}
		
		uint ri=0;
		uint ci;
		for (auto& row : data) {
			ci=0;
			for (auto& elem : row) {
				matrix[ci][matrix[ci].size()-ri-1]=elem; //transpose and invert Y
				ci++;
			}
			ri++;
		}

	}

	//Accessors for left-right elements
	class RowProxy {
	public:
		RowProxy(std::vector<T>& row) : row_(row) {}
		
		T& operator[](size_t col) { //left-right elements
			return row_[col];
		}
		const T& operator[](size_t col) const { //left-right elements
			return row_[col];
		}
		
		size_t wmSize() const { //left-right size
			return row_.size();
		}
		void wmResize(size_t sizeY) { //left-right resize
			row_.resize(sizeY);
		}

	private:
		std::vector<T>& row_;
	};

	RowProxy operator[](size_t i) {
		if (i >= matrix.size()) {return zeroMatrix[0];} //out of range
		return RowProxy(matrix[i]);
	}
    const RowProxy operator[](size_t i) const {
		if (i >= matrix.size()) {return const_cast<std::vector<T>&>(zeroMatrix[0]);} //out of range
        return RowProxy(const_cast<std::vector<T>&>(matrix[i]));
    }

	//get element
	T& operator()(size_t row, size_t col) {
		return matrix[row][col];
	}
	//get element
	const T& operator()(size_t row, size_t col) const {
		return matrix[row][col];
	}

	//get matrix left-right size
	size_t wmSize() const {
		return matrix.size();
	}
	//get matrix[i] down-up size
	size_t wmSize(size_t i) {
		if (i >= matrix.size()) {return 0;} //no rows
		return matrix[i].size();
	}
	
	//resize matrix down-up to sizeX
	void wmResize(uint sizeX) {
		matrix.resize(sizeX);
	}
	
	//resize matrix[i] left-right to sizeY
	void wmResize(size_t i, size_t sizeY) {
		matrix[i].resize(sizeY);
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
	private:
	std::vector<std::vector<T>> zeroMatrix={{0}};

};