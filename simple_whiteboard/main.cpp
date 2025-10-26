#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "glad/glad.h"
#include "shader/shader.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include <vector>
#include <thread>
#include <chrono>
#include "stb_image_write.h"

int scrWidth = 1354;
int scrHeight = 717;

glm::mat4 projection = glm::ortho(0.0f, (float)scrWidth, (float)scrHeight, 0.0f);

struct FBO{
	unsigned int fbo;
	unsigned int texture;
};

struct canvas{
	unsigned int VAO;
	unsigned int VBO;
	unsigned int EBO;
};

struct line_construct{
	std::vector<float> vertices;
	std::vector<std::vector<float>> last_state;
	unsigned int VAO = 0;
	unsigned int VBO = 0;
	int smoothness = 5; /* This means 5 iterations of the chaikin algorithm*/
	float thickness = 1.0f;
	float dm, em = 30;
	bool initialized = false;

	void clean(){
		if(VAO){
			glDeleteVertexArrays(1, &VAO);
		}
		if(VBO){
			glDeleteBuffers(1, &VBO);
		}

		VAO=0; VBO=0;
		initialized=false;
		vertices.clear();
	}
};


void framebuffer_size_callback(GLFWwindow* win, int width, int height){
	glViewport(0,0,width,height);
	scrWidth = width;
	scrHeight = height;
	projection = glm::ortho(0.0f, (float)scrWidth, (float)scrHeight, 0.0f);
	std::cout << "Window Changed " << std::endl;
}

void save_png(const std::string& fname, int width, int height){
	const int channels = 3; /* RGB, place 4 if u want RGBA */
	const int npixels = width * height * channels;
	unsigned char* pixels = new unsigned char[npixels]; /* new instead of just pixels[size] to support window resizing */

	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadBuffer(GL_FRONT);
	glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);
	
	/* flip pixels to write img */
	unsigned char* fpixels = new unsigned char[npixels];
	for(int i=0;i<height;i++){
		memcpy(fpixels + (height - 1 - i) * width * channels, pixels + i * width * channels, width * channels);
	}

	stbi_write_png(fname.c_str(), width, height, channels, fpixels, width * channels);

	delete[] pixels;
	delete[] fpixels;

	std::cout << "Saved " << fname << std::endl;
}

void processInput(GLFWwindow* win){
	if(glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS){
		glfwSetWindowShouldClose(win,true);
	}
}

void init_line_construct(line_construct& x){
	if(x.initialized) return;
	x.VAO = 1;
	x.VBO = 1;

	glGenVertexArrays(1, &x.VAO);
	glBindVertexArray(x.VAO);

	glGenBuffers(1, &x.VBO);
	glBindBuffer(GL_ARRAY_BUFFER, x.VBO);
	glBufferData(GL_ARRAY_BUFFER, x.vertices.size() * sizeof(float), x.vertices.data(), GL_DYNAMIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	x.initialized = true;
}

std::vector<float> chaikin_algorithm(line_construct& x){
	std::vector<float> new_points;
	int vertices_size = x.vertices.size();

	new_points.push_back(x.vertices[0]);
	new_points.push_back(x.vertices[1]);

	for(int i=0; i<x.vertices.size()-3;i=i+2){
		/* r */
		new_points.push_back( (0.75 * x.vertices[i]) + (0.25 * x.vertices[i+2]));
		new_points.push_back( (0.75 * x.vertices[i+1]) + (0.25 * x.vertices[i+3]));
		/* q */
		new_points.push_back( (0.25 * x.vertices[i]) + ((0.75) * x.vertices[i+2]));
		new_points.push_back( (0.25 * x.vertices[i+1]) + ((0.75) * x.vertices[i+3]));
	}


	return new_points;
}

void draw_static_line_construct(line_construct& x){
	glBindVertexArray(x.VAO);
	glDrawArrays(GL_LINE_STRIP, 0, x.vertices.size() / 2);
	glLineWidth(x.thickness);
	glBindVertexArray(0);
}

void draw_dynamic_line_construct(line_construct& x){
	/* size_t checker for x line construct so it doesn't allocate bufferdata every frame if there are no changes */
	static size_t last_xsize = 0;

	if(x.vertices.size() != last_xsize){
		glBindBuffer(GL_ARRAY_BUFFER, x.VBO);
		glBufferData(GL_ARRAY_BUFFER, x.vertices.size() * sizeof(float), x.vertices.data(), GL_DYNAMIC_DRAW);
		last_xsize = x.vertices.size();
	}

	glBindVertexArray(x.VAO);
	glDrawArrays(GL_LINE_STRIP, 0, x.vertices.size() / 2);
	glLineWidth(x.thickness);
	glBindVertexArray(0);
}

void waitm(int m){
	std::this_thread::sleep_for(std::chrono::milliseconds(m));
}

glm::vec2 convert_ndc(double xpos, double ypos, const glm::mat4& vp){
	float x_ndc = (2.0f * static_cast<float>(xpos)) / scrWidth - 1.0f;
	float y_ndc = 1.0f - (2.0f * static_cast<float>(ypos)) / scrHeight;
	glm::vec4 ndc = glm::vec4(x_ndc, y_ndc, 0.0f, 1.0f);

	glm::mat4 fvp = glm::inverse(vp);
	glm::vec4 positions = fvp * ndc;
	return glm::vec2(positions.x, positions.y);
}

void init_fbo(FBO& layer){
	glGenFramebuffers(1, &layer.fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, layer.fbo);

	/* texture */
	glGenTextures(1, &layer.texture);
	glBindTexture(GL_TEXTURE_2D, layer.texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1354, 717, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);

	// attach it to currently bound framebuffer object
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, layer.texture, 0);

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);  
}

void init_quad(canvas& canva){
		std::vector<float> vertices = {
			// Positions
			1.0f,  1.0f,    1.0f, 1.0f,
			1.0f, -1.0f,    1.0f, 0.0f,
			-1.0f,-1.0f,    0.0f, 0.0f,
			-1.0f, 1.0f,    0.0f, 1.0f
        };

        unsigned int indices[] = {
			0, 1, 3,
			1, 2, 3
        };


        glGenVertexArrays(1, &canva.VAO);
        glGenBuffers(1, &canva.EBO);
        glGenBuffers(1, &canva.VBO);

        glBindVertexArray(canva.VAO);

        glBindBuffer(GL_ARRAY_BUFFER, canva.VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, canva.EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_DYNAMIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)(2*sizeof(float)));
        glEnableVertexAttribArray(1);
}

int main(void){
    if(!glfwInit()) { /* failed */ }
    
    GLFWwindow* win = glfwCreateWindow(scrWidth, scrHeight, "Whiteboard", NULL, NULL);
	if(!win) {
		std::cout << "failed to create windows" << std::endl;
		glfwTerminate();
		return -1;
	}

    glfwMakeContextCurrent(win);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    	std::cerr << "Failed to initialize GLAD" << std::endl;
    	return -1;
	}

	glViewport(0,0, 1354, 717);

	glfwSetFramebufferSizeCallback(win, framebuffer_size_callback); /* callback if win has been resized */

	/* mouse */
	double lastx, lasty, xpos, ypos;

	/* fps */
	const double fps = 60;
	const double fps_time = 1.0/ fps;
	double start_time;
	double end_time;
	double frame_time;
	double sleep_time;
	double last_time = glfwGetTime();

	/* Drawing Colors */
	glm::vec3 grayColor = glm::vec3(0.5f, 0.5f, 0.5f);
	glm::vec3 whiteColor = glm::vec3(1.0f, 1.0f, 1.0f);
	glm::vec3 greenColor = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 blueColor = glm::vec3(0.55f, 0.75f, 0.85f);
	glm::vec3 redColor = glm::vec3(1.0f, 0.0f, 0.0f);
	glm::vec3 blackColor = glm::vec3(0.0f, 0.0f, 0.0f);


	/* initial line_constructs */
	bool is_drawing = false;
	bool eraser = false;
	line_construct x;

	FBO layer;
	canvas canva;
	init_fbo(layer);
	init_quad(canva);

	Shader shader("shader/shader.vs", "shader/shader.fs");
	Shader text_shader("shader/tshader.vs", "shader/tshader.fs");

	init_line_construct(x);
	glfwGetCursorPos(win, &lastx, &lasty);

	glm::mat4 view, vp;

    while(!glfwWindowShouldClose(win)){
		shader.setUProjection("uProjection", projection);

		start_time = glfwGetTime();
		processInput(win);

		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		shader.use();

		/* Keyboard Input */
		if(glfwGetKey(win, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS && glfwGetKey(win, GLFW_KEY_Z) == GLFW_PRESS){
			if(x.last_state.size() > 0){
				glBindFramebuffer(GL_FRAMEBUFFER, layer.fbo);
				glViewport(0, 0, 1354, 717);
				shader.use();
				shader.setUProjection("uProjection", projection);
				shader.setRenderColor("renderColor", whiteColor);
				x.vertices = x.last_state.back();
				draw_dynamic_line_construct(x);
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				x.last_state.pop_back();
				x.vertices.clear();
				waitm(150);

			}
		}

		if(glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS){
			if(eraser){
				eraser = false;
			}
			else{
				eraser = true;
			}
		}

		if(glfwGetKey(win, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS && glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS){
			const std::string fname = "screenshot.png";
			save_png(fname, scrWidth, scrHeight);
		}

		if(glfwGetKey(win, GLFW_KEY_KP_ADD) == GLFW_PRESS){
			if(x.thickness < 40){
				x.thickness += 0.5f;
				waitm(50);
			}
		}
		if(glfwGetKey(win, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS){
			if(x.thickness > 0){
				x.thickness -= 0.5f;
				waitm(50);
			}
		}

		/* Mouse Input */
		if(glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS){
		 	is_drawing = true;
		 	glfwGetCursorPos(win, &xpos, &ypos);
		 	x.vertices.push_back(xpos); x.vertices.push_back(ypos);
		}

		if(glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE && is_drawing){
		 	is_drawing = false;
		 	if(x.vertices.size() > 4){
		 		for(int i=0;i<x.smoothness;i++){
					x.vertices = chaikin_algorithm(x);
		 		}

				glBindFramebuffer(GL_FRAMEBUFFER, layer.fbo);
				glViewport(0, 0, 1354, 717);
				shader.use();
				shader.setUProjection("uProjection", projection);
				if(eraser == false){
					shader.setRenderColor("renderColor", blackColor);
				}
				else{
					shader.setRenderColor("renderColor", whiteColor);
				}


				draw_dynamic_line_construct(x);
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				x.last_state.push_back(x.vertices);
				x.vertices.clear();
			}
		}

		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		text_shader.use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, layer.texture);
		text_shader.setInt("text", 0);
		glBindVertexArray(canva.VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		if(is_drawing){
			shader.use(); // Use the line shader
			shader.setUProjection("uProjection", projection);
			draw_dynamic_line_construct(x);
		}	

		glfwSwapBuffers(win);
		glfwPollEvents();

		/* frames management */
		end_time = glfwGetTime();
		frame_time = end_time - start_time;
		if(frame_time < fps_time){
			sleep_time = fps_time - frame_time;
			std::this_thread::sleep_for(std::chrono::duration<double>(sleep_time));
		}
    }

	x.clean();
    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
