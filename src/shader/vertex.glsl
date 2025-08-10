#version 450 core

layout (location = 0) in vec2 aPos;

out vec2 TexCoord;

void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    // Convert from [-1,1] to [0,1] for texture coordinates
    TexCoord = (aPos + 1.0) * 0.5;
}
