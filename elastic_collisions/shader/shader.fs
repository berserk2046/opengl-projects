#version 330 core
out vec4 FragColor;
uniform vec3 ballColor;

void main(){
	FragColor = vec4(ballColor, 1.0);
}
