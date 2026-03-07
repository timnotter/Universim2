#shader VERTEX
// Vertex shader
#version 450 core
layout (location = 0) in dvec3 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec4 aNormal;

uniform dvec3 uCameraPosition;
uniform dvec3 uCameraDirection;
uniform dmat3 uInvCamBaseTransMatrix;

out vec4 vColor;

void main() {
    gl_Position = vec4(vec2(aPos.xy), 0.0, 1.0);
	vColor = aColor;
}

#shader FRAGMENT
// Fragment shader
#version 450 core

in vec4 vColor;

layout (location = 0) out vec4 FragColor;

void main() {
    FragColor = vColor;
    //FragColor = u_Color;
}

