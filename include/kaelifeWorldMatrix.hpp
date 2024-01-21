/**
 * @file kaelifeWorldMatrix.hpp
 * @brief 2D rectangle std::vector in world orientation and transform
*/

#pragma once
#include <iostream>
#include <vector>
#include <cstdint>
#include <cmath>

template <typename T>
/**
 * @brief 2D rectangular vector in world space
 *
 * vector up-left = {0,0}  down-right = {width, height}
 * 
 * WorldMatrix down-left = {0,0}  up-right = {width, height}  transposed
 *  vector[4][5] to WorldMatrix
 * 
 * vector<T>{
 * 	{1,2,3,4}
 * 	{1,2,}
 * 	{1,2,3}
 * 	{1,2,3,4,5}
 * }
 * 
 * Versus
 * 
 * WorldMatrix<T>{
 * 	{1,1,1,1,0}
 * 	{2,2,2,2,0}
 * 	{3,0,3,3,0}
 * 	{4,0,0,4,0}
 * 	{0,0,0,0,5}
 * }
 * 
 * This way the hardcoded vector has same orientation as the world space and potentially eases the mirroring and randomizing the elements
 * WorldMatrix is not recommended to be used in tight loops 
 * left-right = Rows
 * down-up = Columns
 * 
 * Axis Directions: WorldMatrix[Rows][Cols] (Row Major)
 * - left-right = Rows
 * - down-up = Columns
 * 
 * 2D vector in world space
 * Acts like a 2D vector but the code-space down-up axis is flipped then axes are transposed and padded
 * 
 * @tparam T Type of the elements.
 */
class WorldMatrix {
	private:
	std::vector<std::vector<T>> matrix={{0}};
	std::vector<std::vector<T>> zeroMatrix={{0}};
	size_t width =1;
	size_t height=1;

public:
	/**
	 * @brief Null WorldMatrix is converted to 1x1 zero matrix
	*/
	WorldMatrix() : matrix{{0}}, width(1), height(1) {}
	
	/**
	* @brief Construct 2D rectangular WorldMatrix
	*
	* Example 3x2 WorldMatrix
	* @code
	*	const WorldMatrix<uint8_t>& foo =
	*	{
	*		{  0, 110,   0},
	*		{ 32,   0,  73}
	*	},
	* @endcode
	*/
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

	/**
	 * @brief Proxy to access row elements
	 * 
	 * @param row WorldMatrix[row]
	 * @param rowWidth row left-right width
	*/
	class RowProxy {
	public:
		RowProxy(std::vector<T>& row, size_t rowWidth) : row_(row), rowWidth_ (rowWidth) {}

		/**
		 * @return row[col] 
		*/
		T& operator[]( size_t col ) {
			if ( col >= rowWidth_ ) {
				static T nullValue = T(); 
				printf("Invalid WorldMatrix Y [][%lu] \n", col );
				//abort();
				return nullValue;
			}
			return row_[col];
		}

		/**
		 * @return const row[col] 
		*/
		const T& operator[]( size_t col ) const {
			if ( col >= rowWidth_ ) {
				static T nullValue = T(); 
				printf("Invalid WorldMatrix Y [][%lu] \n", col );
				//abort();
				return nullValue;
			}
			return row_[col];
		}

	private:
		std::vector<T>& row_= 0;
		size_t rowWidth_ = 0;
	};

	/**
	 * @brief Get matrix element. WorldMatrix[X][Y] 
	*/
	RowProxy operator[](size_t row) {
		if ( !width || row >= width ) {
			printf("Invalid WorldMatrix X [%lu][%lu] \n", row, height);
			//abort();
			return RowProxy(zeroMatrix[0], height );
		} // out of range
		return RowProxy(matrix[row], height );
	}

	/**
	 * @brief Get matrix const element. WorldMatrix[X][Y] 
	*/
	const RowProxy operator[](size_t row) const {
		if ( !width || row >= width ) {
			static std::vector<T> nullValue = {T()}; 
			printf("Invalid WorldMatrix X [%lu][%lu] \n", row, height);
			//abort();
			return RowProxy(nullValue, height );
		} // out of range
    	return RowProxy(const_cast<std::vector<T>&>(matrix[row]), height );
	}


	/**
	 * @brief Get matrix left-right width
	*/
	size_t getWidth() const {
		return width;
	}
	/**
	 * @brief Get matrix[X] down-up height
	*/
	size_t getHeight() const {
		return height;
	}
	
	/**
	 * @brief Resize matrix[X] width
     *
     * @param newWidth
	*/
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
	
	/**
	 * @brief Resize matrix height
     *
     * @param newHeight
	*/
	void setHeight(size_t newHeight) {
		if(width==0 && newHeight!=0){setWidth(1);}
		for(size_t i=0;i<width;i++){
			matrix[i].resize(newHeight);
		}
		height=newHeight;
	}
	
	/**
	 * @brief Print this 2D WorldVector in hard-coded format 
	*/
	void printCodeSpace() {
		size_t maxDigits = log10(UINT8_MAX)+1;

		size_t rowWidth = matrix.size();
		size_t colHeight = (rowWidth > 0) ? matrix[0].size() : 0;

		for (size_t j = 0; j < colHeight; ++j) {
			printf("{");
			for (size_t i = 0; i < rowWidth; ++i) {
				T num = (j < matrix[i].size()) ? matrix[i][colHeight-j-1] : 0;
				size_t digits = num > 0 ? static_cast<size_t>(log10((double)num) + 1) : 1;
				for (size_t k = 0; k < maxDigits - digits; ++k) { printf(" ");	}
				printf("%d", num);
				if (i == rowWidth - 1) {	printf("}"); }
				if (!(i == rowWidth - 1 && j == colHeight - 1)) { printf(","); }
			}
			printf("\n");
		}
	}
};