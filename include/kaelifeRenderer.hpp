#pragma once

#include <iostream>
#include <GL/glew.h>
#include <fstream>
#include <sstream>
#include <cstring>
#include "kaelifeControls.hpp"

// Function to read a shader file and return the content as a string
std::string readShaderFile(const std::string& filePath) {
    std::ifstream file(filePath);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Function to replace a substring in a string
void replaceAll(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
}

// Function to process shader source and resolve includes
std::string processShaderSource(const std::string& source) {
    std::string processedSource = source;

    // Define a special marker for includes
    const std::string includeMarker = "//#include";

    // Find and process includes
    size_t pos = processedSource.find(includeMarker);
    while (pos != std::string::npos) {
        // Find the end of the line
        size_t endOfLine = processedSource.find("\n", pos);
        if (endOfLine != std::string::npos) {
            // Extract the file path from the include directive
            std::string includeDirective = processedSource.substr(pos, endOfLine - pos);
            size_t filePathStart = includeDirective.find("\"") + 1;
            size_t filePathEnd = includeDirective.rfind("\"");
            std::string filePath = includeDirective.substr(filePathStart, filePathEnd - filePathStart);

            // Read the content of the included file
            std::string includedContent = readShaderFile(filePath);

            // Replace the include directive with the file content
            replaceAll(processedSource, includeDirective, includedContent);
        }

        // Find the next include
        pos = processedSource.find(includeMarker, pos + 1);
    }

    return processedSource;
}

// Function to compile a shader
GLuint compileShader(GLenum type, const std::string& source) {
    GLuint shader = glCreateShader(type);

    // Process shader source to resolve includes
    std::string processedSource = processShaderSource(source);
    const char* sourceCStr = processedSource.c_str();

    glShaderSource(shader, 1, &sourceCStr, nullptr);
    glCompileShader(shader);

    // Check compilation status
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
        std::cerr << "Shader compilation error:\n" << infoLog << std::endl;
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

// Function to create a shader program and link shaders
GLuint createShaderProgram(const std::string& vertexShaderSource, const std::string& fragmentShaderSource) {
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    // Create shader program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

	GLint success;
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success) {
		GLchar infoLog[512];
		glGetProgramInfoLog(shaderProgram, sizeof(infoLog), nullptr, infoLog);
		std::cerr << "Shader program linking error: " << infoLog << std::endl;
        glDeleteProgram(shaderProgram);
        return 0;
    }

    // Delete individual shaders, as they are now part of the program
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

// In initialization code
GLuint textureID;
GLuint shaderProgram;


// Function to create shader program from file paths
GLuint createShader(const std::string& vertexShaderPath, const std::string& fragmentShaderPath) {
    std::string vertexShaderSource = readShaderFile(vertexShaderPath);
    std::string fragmentShaderSource = readShaderFile(fragmentShaderPath);
    return createShaderProgram(vertexShaderSource, fragmentShaderSource);
}

void initPixelMap(const CAData& cellData) {
    shaderProgram = createShader("./shader/vertex.vs.glsl", "./shader/fragment.fs.glsl");

    glUseProgram(shaderProgram);

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, cellData.mainCache.tileCols, cellData.mainCache.tileRows, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "initPixelMap: " << error << std::endl;
    }
}


void updateTexture(const CAData& cellData) {
    uint activeRenderBuf = cellData.mainCache.activeBuf; //ensure buffer doesn't change during render
    glBindTexture(GL_TEXTURE_2D, textureID);

    //reduce from 2D to 1D
    std::vector<uint8_t> pixelData;
    pixelData.resize(cellData.mainCache.tileCols * cellData.mainCache.tileRows);

    for (uint i = 0; i < cellData.mainCache.tileRows; ++i) {
        const auto& row = cellData.stateBuf[activeRenderBuf][i];
        std::memcpy(pixelData.data() + i * cellData.mainCache.tileCols, row.data(), cellData.mainCache.tileCols);
    }

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, cellData.mainCache.tileCols, cellData.mainCache.tileRows, GL_LUMINANCE, GL_UNSIGNED_BYTE, pixelData.data());

    glBindTexture(GL_TEXTURE_2D, 0);


}






static inline void renderCells(const CAData& cellData, const InputHandler& kaeInput, SDL_Window *&SDLWindow) {
	const GLfloat quadVertices[] = {
		-1.0f, -1.0f,
		1.0f, -1.0f,
		1.0f,  1.0f,
		-1.0f,  1.0f
	};

	auto offsetScale = InputHandler::getWorldTransform(cellData, SDLWindow);
	
	glViewport(static_cast<GLint>(offsetScale[0]), static_cast<GLint>(offsetScale[1]), static_cast<GLsizei>(offsetScale[2]), static_cast<GLsizei>(offsetScale[3]));

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, cellData.renderWidth, 0, cellData.renderHeight, -1, 1); // Set an orthographic projection


	float cursorBorder=2; //cursor outline in tiles
	auto worldCursorPos = InputHandler::getWorldCursorPos(cellData, SDLWindow); 

    //normalize variables for shader
	float shaderCursorPos[2];
	shaderCursorPos[0]=worldCursorPos[0];
	shaderCursorPos[1]=worldCursorPos[1];
    uint numStates=cellData.kaePreset.current()->stateCount;
	float shaderCellStateScale = 255.0 / (numStates - 1);
	float shaderDrawRadius  = (float)kaeInput.drawRadius;
	float shaderCursorBorder= (float)cursorBorder;
    float shaderShaderColor = kaeInput.shaderColor;
    float shaderShaderHue = kaeInput.shaderHue;
	float shaderStateCount = numStates;
	float shaderColorStagger = kaeInput.colorStagger;
    

    float shaderTileDim[2];
    shaderTileDim[0]=cellData.mainCache.tileRows;
    shaderTileDim[1]=cellData.mainCache.tileCols;

	// Pass shader variables
	glUniform2f (glGetUniformLocation(shaderProgram, "tileDim"		    ), shaderTileDim[1], shaderTileDim[0] ); //flip coordinates in glew
	glUniform2f (glGetUniformLocation(shaderProgram, "cursorPos"		), shaderCursorPos[1], shaderCursorPos[0] );
    glUniform1f (glGetUniformLocation(shaderProgram, "cellStateScale"	), shaderCellStateScale);
	glUniform1f (glGetUniformLocation(shaderProgram, "cursorRadius"		), shaderDrawRadius);
	glUniform1f (glGetUniformLocation(shaderProgram, "cursorBorder"		), shaderCursorBorder);
	glUniform1f (glGetUniformLocation(shaderProgram, "shaderColor"		), shaderShaderColor);
	glUniform1f (glGetUniformLocation(shaderProgram, "shaderHue"		), shaderShaderHue);
	glUniform1f (glGetUniformLocation(shaderProgram, "stateCount"	    ), shaderStateCount);
	glUniform1f (glGetUniformLocation(shaderProgram, "colorStagger"	    ), shaderColorStagger);

    //copy stateBuf to texture
	updateTexture(cellData);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glBegin(GL_QUADS);
    for (int i = 0; i < 4; ++i) {
        glVertex2f(quadVertices[2 * i], quadVertices[2 * i + 1]);
    }
    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);

}