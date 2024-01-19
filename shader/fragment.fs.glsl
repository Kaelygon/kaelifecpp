#version 330 core
#define M_PI 3.14159265358979323846
#define M_TAU 6.28318530717958647693
#define UINT8_MAX 255

//example of header file that would be handled by processShaderSource() '//#include "path/to/source.h.glsl"'

uniform sampler2D textureSampler;
uniform vec2 cursorPos;
uniform vec2 tileDim; // rows cols
uniform float cursorRadius;
uniform float cursorBorder;
uniform float shaderColor;
uniform float shaderHue;
uniform float stateCount;

uniform float colorStagger;

in vec2 texCoord;
out vec4 fragColor;

vec3 calculateNormal(float normalizedCellState){	
	// Calculate the normal vector
	float sumSlope_x = 0.0;
	float sumSlope_y = 0.0;

	int maskRadius = 1; // Adjust the mask radius as needed

	for (int s = -maskRadius; s <= maskRadius; ++s) {
		for (int t = -maskRadius; t <= maskRadius; ++t) {
			vec2 offset = vec2(float(s), float(t)) / tileDim;
			float neighborValue = texture(textureSampler, texCoord + offset).r;

			float slope = normalizedCellState - neighborValue;
			sumSlope_x += slope * offset.x;
			sumSlope_y += slope * offset.y;
		}
	}

	// Normalize the normal vector
	float length = sqrt(sumSlope_x * sumSlope_x + sumSlope_y * sumSlope_y);
	if (length > 0.0) {
		sumSlope_x /= length;
		sumSlope_y /= length;
	}

	// Map the normalized components to color values (assuming positive values)
	vec3 normalColor = 0.5 * (vec3(sumSlope_x, sumSlope_y, 1.0) + 1.0);

	return normalColor;
}

vec3 hslToRgb(vec3 hsl) {
    float hue = hsl.x;
    float saturation = hsl.y;
    float lightness = hsl.z;

    float chroma = (1.0 - abs(2.0 * lightness - 1.0)) * saturation;
    float huePrime = hue * 6.0;
    float x = chroma * (1.0 - abs(mod(huePrime, 2.0) - 1.0));

    vec3 rgb;

    if (huePrime < 1.0) {
        rgb = vec3(chroma, x, 0.0);
    } else if (huePrime < 2.0) {
        rgb = vec3(x, chroma, 0.0);
    } else if (huePrime < 3.0) {
        rgb = vec3(0.0, chroma, x);
    } else if (huePrime < 4.0) {
        rgb = vec3(0.0, x, chroma);
    } else if (huePrime < 5.0) {
        rgb = vec3(x, 0.0, chroma);
    } else {
        rgb = vec3(chroma, 0.0, x);
    }

    float m = lightness - 0.5 * chroma;
    return rgb + m;
}

//https://www.desmos.com/calculator/1yswqyau5v
//calculate hue from gray scale
vec3 calculateFalseColors(float hue){
		float originalHue=hue;

		//Since hue[0] == hue[2 pi] we scale hue by n/(n+1.0). Otherwise lowest and highest cell states would be mapped to identical hue 
		hue = hue * float( stateCount / (stateCount+1.0) );

		//adjust hue distance between cell states
		hue = hue*(1+colorStagger*(stateCount+1));

		//shift hue
		hue += shaderHue;
		hue = mod( hue, 1.0 ); //wrap around

		//Lower states should be darker and less saturated
		float saturation = (        hue*0.900+0.100);
		float lightness  = (originalHue*0.900+0.100);

		return hslToRgb(vec3(hue, saturation, lightness));
}

void main() {
	vec2 screenCursorPos = cursorPos;  // position and texture in tile space
	vec2 screenTexCoord = floor(texCoord * tileDim);

	//overflow wrapping is done at getWorldCursorPos
	vec2  cursorDelta = min(abs(screenTexCoord - screenCursorPos), tileDim - abs(screenTexCoord - screenCursorPos));
	float cursorDist = length(cursorDelta);

	float isCursorNear = float((cursorDist >= cursorRadius - cursorBorder) && (cursorDist < cursorRadius));

	float cellState = texture(textureSampler, texCoord).r;

	float normalizedCellState = cellState*UINT8_MAX/(stateCount-1); // [0,stateCount/255] to [0,1]

	vec3 cellColor = vec3(normalizedCellState, normalizedCellState, normalizedCellState);
	vec3 cursorColor = vec3(1.0, 0.0, 1.0);
	
	if (shaderColor > 0.0) {
		cellColor = calculateFalseColors(normalizedCellState);
	}

	cellColor = mix(cellColor, cursorColor, isCursorNear * 0.5); // mix if within border
	

	fragColor = vec4(cellColor, 1.0);
}
