#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aOffset;
out vec3 renderColor;

uniform mat4 uProjection;

void main(){
	gl_Position = uProjection * vec4(aPos + aOffset, 0.0, 1.0);
	renderColor = vec3(1.0, 1.0, 1.0);
}
