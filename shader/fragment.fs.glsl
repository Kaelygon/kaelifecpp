#version 330 core

//#include "./shader/palette.h.glsl" //handled by processShaderSource()

uniform sampler2D textureSampler;
uniform float cellStateScale;
uniform vec2 cursorPos;
uniform vec2 tileDim; // rows cols
uniform float cursorRadius;
uniform float cursorBorder;
uniform float shaderColor;
uniform float shaderHue;
uniform float stateCount;

uniform float colorStagger=float(1);

in vec2 texCoord;
out vec4 fragColor;


void main() {
	vec2 screenCursorPos = cursorPos; // position and texture in tile space
	vec2 screenTexCoord = floor(texCoord * tileDim);

	// Calculate the distance to the closest tile position (considering wrapping)
	vec2 wrappedCursorPos = mod(screenCursorPos, tileDim);
	vec2 wrappedDist = min(abs(screenTexCoord - wrappedCursorPos), tileDim - abs(screenTexCoord - wrappedCursorPos));
	float cursorDist = length(wrappedDist);

	float isCursorNear = float((cursorDist >= cursorRadius - cursorBorder) && (cursorDist < cursorRadius));

	float value = texture(textureSampler, texCoord).r;

	float scaledValue = value * cellStateScale; // [0,1/stateCount] to [0,1]

	vec3 cellColor = vec3(scaledValue, scaledValue, scaledValue);
	vec3 cursorColor = vec3(1.0, 0.0, 1.0);


	// Calculate the normal vector
	float sumSlope_x = 0.0;
	float sumSlope_y = 0.0;

	int maskRadius = 1; // Adjust the mask radius as needed

	for (int s = -maskRadius; s <= maskRadius; ++s) {
		for (int t = -maskRadius; t <= maskRadius; ++t) {
			vec2 offset = vec2(float(s), float(t)) / tileDim;
			float neighborValue = texture(textureSampler, texCoord + offset).r;

			float slope = scaledValue - neighborValue;
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
	
	if(shaderColor>0.0 && shaderColor<=1.0){
		cellColor=normalColor;
	}else if(shaderColor>1.0){
		uint colorIndex=uint(scaledValue*255.0f);
		//colorIndex=(colorIndex*uint(colorStagger));
		colorIndex = uint( float(colorIndex) * float( (stateCount)/(stateCount+1.0f) ) ); //add gap at the end to prevent first and last color being too similar
		colorIndex = (colorIndex*uint(colorStagger)+uint(shaderHue))&uint(0xFF);
		cellColor=palette[colorIndex]/255.0f;
	}

	cellColor = mix(cellColor, cursorColor, isCursorNear * 0.5); // mix if within border
	

	fragColor = vec4(cellColor, 1.0);
}
