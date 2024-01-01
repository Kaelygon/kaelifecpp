#version 330 core

layout(location = 0) in vec2 inPosition;
out vec2 texCoord;

void main() {
    gl_Position = vec4(inPosition.y, inPosition.x, 0.0, 1.0);
    texCoord = inPosition * 0.5 + 0.5;  // Adjust texture coordinates if needed
}
