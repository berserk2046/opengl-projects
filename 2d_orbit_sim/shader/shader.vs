#version 330 core
layout (location = 0) in vec2 aPos;
out vec3 planetColor;

uniform mat4 uProjection;

void main(){
	gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
	planetColor = vec3(1.0, 1.0, 1.0);
}
