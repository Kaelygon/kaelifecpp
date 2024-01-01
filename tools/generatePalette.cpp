
#include <algorithm>
#include <cmath>
#include <array>
#include <vector>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>      // std::rand, std::srand
#include <random>       // std::default_random_engine

using Color = std::array<uint8_t, 3>;
std::vector<Color> palette = {};

uint NUM_COLS=255;

// Function to calculate the 3D hypotenuse length for a color in the palette
double calculateHypotenuse(const Color& color) {
	return std::sqrt(static_cast<double>(color[0] * color[0] + color[1] * color[1] + color[2] * color[2]));
}

// Custom comparison function for sorting based on 3D hypotenuse length
bool compareColors(const Color& color1, const Color& color2) {
	return calculateHypotenuse(color1) < calculateHypotenuse(color2);
}

double calcLum(const Color& color) {
    // Normalize color components to the range [0, 1]
    double normalizedR = color[0] / (double)NUM_COLS;
    double normalizedG = color[1] / (double)NUM_COLS;
    double normalizedB = color[2] / (double)NUM_COLS;
	
    // Calculate luminosity using the formula
    double luminosity = 0.299 * normalizedR + 0.587 * normalizedG + 0.114 * normalizedB;

    return luminosity;
}

bool compLum (const Color& color1, const Color& color2) {
	return calcLum(color1) < calcLum(color2);
}

double calcHue(const Color& color) {
    // Normalize RGB values to the range [0, 1]
    double r = color[0] / (double)NUM_COLS;
    double g = color[1] / (double)NUM_COLS;
    double b = color[2] / (double)NUM_COLS;

    // Find the maximum and minimum values among RGB components
    double maxVal = std::max({r, g, b});
    double minVal = std::min({r, g, b});

    // Calculate the difference between max and min
    double delta = maxVal - minVal;

    double hue = 0.0;

    // Calculate hue based on the position of the max component
    if (delta != 0.0) {
        if (maxVal == r) {
            hue = 60.0 * fmod(((g - b) / delta), 6.0);
        } else if (maxVal == g) {
            hue = 60.0 * (((b - r) / delta) + 2.0);
        } else if (maxVal == b) {
            hue = 60.0 * (((r - g) / delta) + 4.0);
        }
    }

    // Ensure hue is in the range [0, 360)
    if (hue < 0.0) {
        hue += 360.0;
    }

    return hue;
}

bool compHue (const Color& color1, const Color& color2) {
	return calcHue(color1) < calcHue(color2);
}

void generatePalette(){


	bool printCol=false;
	std::vector<std::vector<uint8_t>> weight[]={
		{//kael palette
			{ 27, 63, 105,/*136,*/166, 201, 248 },
			{ 14, 57,  71,/*114,*/157, 213, 238 },
			{ 31, 47,  90,  125,  168, 201, 250	}
		},
		{//even 6-6-7
			{ 0, 42, 84,      169, 212, 255 },
			{ 0, 42, 84,      169, 212, 255 },
			{ 0, 36, 72, 109, 146, 219, 255	}
		},
		{//4-16-4
			{  0, 96, 192, 255 },
			{  0, 16, 32, 48, 64, 80, 96, 112, 144, 160, 176, 192, 208, 224, 238, 255 },
			{  0, 96, 192, 255 },
		},
		{//8-8-4
			{ 0x00, 0x24, 0x49, 0x6D, 0x92, 0xB6, 0xDB, 0xFF },
			{ 0x00, 0x24, 0x49, 0x6D, 0x92, 0xB6, 0xDB, 0xFF },
			{ 0x00, 0x55, 0xAA, 0xFF }
		}
	};
	uint palIndex=0;
	bool pastel=1;

	uint count=0;
	for(uint i=0;i<weight[palIndex][0].size();i+=1){
		for(uint j=0;j<weight[palIndex][1].size();j+=1){
			for(uint k=0;k<weight[palIndex][2].size();k+=1){
				++count;
				uint8_t x,y,z;
				x=weight[palIndex][0][i];
				y=weight[palIndex][1][j];
				z=weight[palIndex][2][k];

                if (pastel) {

					// pastel effect scaled by luminosity
					Color minCol = Color{weight[palIndex][0].front(),weight[palIndex][1].front(),weight[palIndex][2].front()};
					Color maxCol = Color{weight[palIndex][0].back() ,weight[palIndex][1].back() ,weight[palIndex][2].back() };
					double minLum=calcLum( minCol );
					double maxLum=calcLum( maxCol );
					double luminosity = (  calcLum({x, y, z}) - minLum  ) / ( maxLum - minLum ) ; //normalize

					double scale = ( (1.0 - 2*(abs(luminosity-0.5))) );
					scale*=0.2; //20%

                    x = (uint8_t)(std::round( ( (1.0-scale) * x + (scale) * (NUM_COLS - x) ) ));
                    y = (uint8_t)(std::round( ( (1.0-scale) * y + (scale) * (NUM_COLS - y) ) ));
                    z = (uint8_t)(std::round( ( (1.0-scale) * z + (scale) * (NUM_COLS - z) ) ));
                }
				Color bufCol={x,y,z};

				palette.push_back(bufCol);
			}
		}
	}
	printf("generated size %d\n",count);

}


// Print the  palette
void printPalette(){
	for(int i=0;i<palette.size();i++){
		Color bufCol=palette[i];
		std::cout << "vec3(" 
			<< static_cast<int>(bufCol[0]) << ", "
			<< static_cast<int>(bufCol[1]) << ", "
			<< static_cast<int>(bufCol[2]) << ")," << std::endl;
	}
};


void writePalette() {

    // Clear file
    std::ofstream ofs;
    ofs.open("palette.ppm", std::ofstream::out | std::ofstream::trunc);
    ofs.close();

    // Save the palette as an image using ImageMagick
    std::ofstream file("palette.ppm", std::ios_base::app | std::ios::binary);
	uint squareSize= ceil(sqrt(palette.size())) ;
    file << "P6\n";
    file << squareSize << " " << squareSize << "\n";
    file << NUM_COLS << "\n";

	while(palette.size()!=squareSize*squareSize){
		palette.push_back(Color{0,0,0});
	}

    for (int i = 0; i < palette.size(); i++) {
        Color bufCol = palette[i];

        file << (uint8_t)(bufCol[0]) << (uint8_t)(bufCol[1]) << (uint8_t)(bufCol[2]);
    }

    file.close();

    // Convert the PPM image to PNG using ImageMagick
    system("magick palette.ppm palette.png");
}

int main() {
	// Sort the palette vector using the custom comparison function

	generatePalette();
//	generatePaletteCGPT();

	if(palette.size()==252){//fill 6-6-7s
		palette.push_back(Color{ 31,  5, 43}); //tieblu
		palette.push_back(Color{166, 73, 90}); //tiemaw
		palette.push_back(Color{177,157,112}); //arkmid
		palette.push_back(Color{235,238,220}); //arkwhi
	}

  	//std::shuffle (palette.begin(), palette.end(), std::default_random_engine(1));

    // Sort the palette based on luminance
    std::sort(palette.begin(), palette.end(), [](const Color& a, const Color& b) {
        double lumA = calcLum(a);
        double lumB = calcLum(b);
        double hueA = calcHue(a);
        double hueB = calcHue(b);
		
        if (lumA != lumB) {
            return lumA < lumB;
        }
        return hueA < hueB;
    });
	
	printPalette();

	writePalette();
	printf("size %lu\n",palette.size());

	return 0;
}