

#include "kaelifeWorldMatrix.hpp"

class WMData {
public:

	uint listIndex=0;

	struct DataList{
		WorldMatrix<uint8_t> mask;
	};

	std::vector<DataList> list={
		{
			.mask{{0,1},{1,2}}
		},{
			.mask={{0,1,2},{3,4,5},{6,7,8}}		
		},{
			.mask={}		
		}
	};

	void setCurrent(uint ind) {
		listIndex=ind;
		listIndex%=list.size();
	}

	DataList* current() {
		return &list[listIndex];
	}

	const DataList* current() const {
		return &list[listIndex];
	}

	const DataList* getList(uint ind) const {
		return &list[ind];
	}

	void next(){
		listIndex++;
		listIndex%=list.size();
	}

	void addList(DataList input){
		list.push_back(input);
	}

	void setValue(uint ind, uint x, uint y, uint value){
		list[ind].mask[x][y]=value;
	};
	
};

int main(){

	WMData dataObject;
	
	WMData::DataList wm ={
		.mask={	{1,2,3}, {4,5,6}, {7,8,9} }
	};

	dataObject.addList(wm);

	dataObject.setCurrent(1);
	dataObject.current()->mask.getHeight();
	
	printf("%d\n",dataObject.current()->mask[0][0]);

	dataObject.setValue(1,611,231,73);

	printf("%d\n",dataObject.getList(2)->mask[0][0]);
	dataObject.setValue(2,0,0,73);

	uint abc = dataObject.current()->mask[112][131];
	printf("%d\n",abc);
	dataObject.current()->mask[112][131]=69;
	printf("%d\n",abc);
	abc=3;
	printf("%d\n",abc);
	


	WorldMatrix<uint8_t> mat=
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
			
	};

	mat.setWidth(7);
	mat.setHeight(14);

	mat.setWidth(2);
	mat.setHeight(9);

	printf("%d\n", mat[0][0]);
	printf("%d\n", mat[1][0]);
	printf("%d\n", mat[1][8]);
	printf("%d\n", mat[0][8]);
	printf("%d\n", mat[0][9]); //should fail


	return 0;
}