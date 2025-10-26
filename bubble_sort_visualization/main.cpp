#include "glad/glad.h"
#include "shader/shader.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <thread>
#include <chrono>

int scrWidth = 900;
int scrHeight = 600;
int cellWidth = 10;
int nRows = scrHeight / cellWidth;
int nCols = scrWidth / cellWidth;

glm::mat4 projection = glm::ortho(0.0f, (float)scrWidth,(float)scrHeight,0.0f); /* left right up down */

struct grids{
	unsigned int VAO = 0;
	unsigned int VBO = 0;
	bool initialized = false;
} grid;

struct block{
	int value;
	unsigned int EBO = 0;
	unsigned int VBO = 0;
	unsigned int VAO = 0;
	bool initialized = false;
};

void framebuffer_size_callback(GLFWwindow* win, int width, int height){
	glViewport(0,0,width,height);
	glm::mat4 projection = glm::ortho(0.0f, (float)width,(float)height,0.0f); // left right up down
}

void processInput(GLFWwindow* win){
	if(glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS){
		glfwSetWindowShouldClose(win,true);
	}
}


void drawBlock(block& n, int posx, int posy){
	if(n.initialized){
		glDeleteVertexArrays(1, &n.VAO);
		glDeleteBuffers(1, &n.VBO);
		glDeleteBuffers(1, &n.EBO);
	}

	float px = posx * cellWidth;
	float py = posy * cellWidth;
	float cellVertices[] = {
		px, py + n.value*cellWidth,
		px + cellWidth, py,
		px, py,
		px + cellWidth, py + cellWidth * n.value
	};
	unsigned int index[] = {
		0, 2, 3,
		1, 2, 3
	};

	glGenBuffers(1, &n.EBO);

	glGenVertexArrays(1, &n.VAO);
	glBindVertexArray(n.VAO);

	glGenBuffers(1, &n.VBO);
	glBindBuffer(GL_ARRAY_BUFFER, n.VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cellVertices), cellVertices, GL_DYNAMIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, n.EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(index), index, GL_DYNAMIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(n.VAO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	n.initialized = true;
}

std::vector<float> createLines(){
	std::vector<float> vertices;
	float ox=0, oy=0, x=0, y=0;
	for(int i=0;i<=nRows;i++){
		vertices.push_back(0.0f);
		vertices.push_back(i*cellWidth);
		vertices.push_back(scrWidth);
		vertices.push_back(i*cellWidth);
	}
	for(int i=0;i<=nCols;i++){
		vertices.push_back(i*cellWidth);
		vertices.push_back(0.0f);
		vertices.push_back(i*cellWidth);
		vertices.push_back(scrHeight);
	}
	return vertices;
}

void gridLines(grids& g){
	if(g.initialized){
		glDeleteVertexArrays(1, &g.VAO);
		glDeleteBuffers(1, &g.VBO);
	}
	std::vector<float> lineVertices = createLines();

	glGenVertexArrays(1, &g.VAO);
	glBindVertexArray(g.VAO);

	glGenBuffers(1, &g.VBO);
	glBindBuffer(GL_ARRAY_BUFFER, g.VBO);
	glBufferData(GL_ARRAY_BUFFER, lineVertices.size() * sizeof(float), lineVertices.data(), GL_DYNAMIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(g.VAO);
	glDrawArrays(GL_LINES, 0, 2*nRows*nCols);
	glLineWidth(1.0f);
	glBindVertexArray(0);

	g.initialized = true;
}

void randGen(std::vector<block>& arr){
	for(int i=0;i<nCols;i++){
		arr[i].value = 1 + rand() % (38-1+1); /* min + rand() % (max-min+1) */
	}
}

void drawArr(std::vector<block>& arr){
	for(int i=0;i<nCols;i++){
		drawBlock(arr[i], i, nRows-arr[i].value);
	}
}

void swap(int& x, int& y){
	int temp = x;
	x = y;
	y = temp;
}


int main(void){
    if(!glfwInit()) { /* failed */ }
	srand(time(0));
    
    GLFWwindow* win = glfwCreateWindow(scrWidth, scrHeight, "sort", nullptr, nullptr);
	if(!win) {
		std::cout << "failed to create windows" << std::endl;
		glfwTerminate();
		return -1;
	}

	/* fps */
	const double fps = 10000.0;
	const double fps_time = 1.0 / fps;
	double startTime;
	double endTime;
	double frameTime;
	double sleepTime;
	double lastTime = glfwGetTime();

	std::vector<block> arr(nCols);
	randGen(arr);

    glfwMakeContextCurrent(win);
	glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    	std::cerr << "Failed to initialize GLAD" << std::endl;
    	return -1;
	}

	glViewport(0,0,scrWidth, scrHeight);
	glfwSetFramebufferSizeCallback(win, framebuffer_size_callback);

	glm::vec3 grayColor = glm::vec3(0.5f, 0.5f, 0.5f);
	glm::vec3 whiteColor = glm::vec3(1.0f, 1.0f, 1.0f);
	glm::vec3 greenColor = glm::vec3(0.0f, 1.0f, 0.0f);

	Shader shader("shader/shader.vs", "shader/shader.fs");

	/* iterators for the bubble sort */
	int i=0, j=0;


    while(!glfwWindowShouldClose(win)){
		startTime = glfwGetTime();

		processInput(win);
		shader.setUProjection("uProjection", projection);
		glClear(GL_COLOR_BUFFER_BIT);
		shader.use();

		shader.setRenderColor("renderColor", whiteColor);
		drawArr(arr);


		if(i < nCols - 1){
			if(j<nCols-i-1){
				if(arr[j].value > arr[j+1].value){
					shader.setRenderColor("renderColor", greenColor);
					drawBlock(arr[j], j, nRows-arr[j].value);
					drawBlock(arr[j+1], j+1, nRows-(arr[j+1].value));
					swap(arr[j].value, arr[j+1].value);

					//std::this_thread::sleep_for(std::chrono::milliseconds(1));
					}
				j++;
			}
			else{
				i++;
				j = 0;
			}
		}


		shader.setRenderColor("renderColor", grayColor);
		gridLines(grid);

		glfwSwapBuffers(win);
		glfwPollEvents();
		endTime = glfwGetTime();
		frameTime = endTime - startTime;
		if(frameTime < fps_time){
			sleepTime = fps_time - frameTime;
			std::this_thread::sleep_for(std::chrono::duration<double>(sleepTime));
		}
    }

    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
