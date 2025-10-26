#version 330 core
layout (location = 0) in vec2 aPos;
out vec3 renderColor;

uniform mat4 uProjection;

void main(){
	gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
	renderColor = vec3(0.0, 0.0, 0.0);
}
