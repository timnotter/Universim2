#shader VERTEX
// Vertex shader
#version 460 core
layout (location = 0) in vec2 aPos;

void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
}

#shader FRAGMENT
// Fragment shader
#version 460 core
out vec4 FragColor;

void main() {
    FragColor = vec4(1.0, 1.0, 1.0, 0.5); // red
}

