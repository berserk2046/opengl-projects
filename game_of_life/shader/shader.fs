#version 330 core
out vec4 FragColor;
uniform vec3 renderColor;

void main(){
	FragColor = vec4(renderColor,1.0);
}
