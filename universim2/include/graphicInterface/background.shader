#shader VERTEX
// Vertex shader
#version 450 core
layout (location = 0) in dvec3 aPos;
layout (location = 1) in dvec3 aDistance;
layout (location = 2) in vec4 aColor;
layout (location = 3) in float aRadius;

uniform dvec3 uCameraPosition;
uniform dvec3 uCameraDirection;
uniform dmat3 uInvCamBaseTransMatrix;

out vec4 vColor;

void main() {
	// TODO: Make this calculation work.
	// Keep in mind that 0/0 is in the 
	// centre of the opengl window

	dvec3 p_view = uInvCamBaseTransMatrix * aDistance;
    gl_Position = vec4(
		float(p_view.x / p_view.z),
		float(p_view.y / p_view.z),
		float(p_view.z), 
		1.0);
	vColor = aColor;
}

#shader FRAGMENT
// Fragment shader
#version 450 core

in vec4 vColor;

layout (location = 0) out vec4 FragColor;

void main() {
    FragColor = vColor;
    //FragColor = uColor;
}

