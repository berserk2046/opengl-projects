#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "glad/glad.h"
#include "shader/shader.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <thread>
#include <chrono>
#include <functional>
#include "stb_image_write.h"

int wscr = 1354;
int hscr = 717;
glm::mat4 projection = glm::ortho(0.0f, (float)wscr, (float)hscr,0.0f); /* left right up down */

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

struct plane{
	unsigned int VAO = 0;
	unsigned int VBO = 0;
	int plane_type = 0;
	int partitions = 20;
	float xorigin, yorigin;
	std::vector<float> vertices;

	void plane_configuration(){
		if(plane_type == 0){
			/* y axis */
			vertices[0] = ((float)wscr/2); vertices[1] = ((float)hscr);
			vertices[2] = ((float)wscr/2); vertices[3] = (0.0f);
			/* x axis */
			vertices[4] = (0.0f); vertices[5] = ((float)hscr/2);
			vertices[6] = (float)wscr; vertices[7] = ((float)hscr/2);

			xorigin = (float)(hscr/2);
			yorigin = (float)(wscr/2);
		}
	}

	void clean(){
		if(VAO){ glDeleteVertexArrays(1, &VAO); }
		if(VBO){ glDeleteBuffers(1, &VBO); }
		VAO = 0; VBO = 0;
		vertices.clear();
	}
};

struct graph{
	std::vector<float> vertices;
	unsigned int VBO = 0;
	unsigned int VAO = 0;
	bool initialized = false;

	void clean(){
		if(VAO){ glDeleteVertexArrays(1, &VAO); }
		if(VBO){ glDeleteBuffers(1, &VBO); }
		VAO = 0; VBO = 0;
		initialized = false;
		vertices.clear();
	}
};

void framebuffer_size_callback(GLFWwindow* win, int width, int height){
	glViewport(0,0,width,height);
	wscr = width;
	hscr = height;
	projection = glm::ortho(0.0f, (float)wscr, (float)hscr,0.0f); /* left right up down */
}

void processInput(GLFWwindow* win){
	if(glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS){
		glfwSetWindowShouldClose(win,true);
	}
}

void graph_mark_lines(plane& xy){
	double col_width = wscr / xy.partitions;
	double nrows = hscr / col_width, ncols = wscr / col_width;
	for(int i=-(xy.partitions/2);i<=(xy.partitions/2);i++){
		xy.vertices.push_back(xy.yorigin + i*col_width);
		xy.vertices.push_back(xy.xorigin);
		xy.vertices.push_back(xy.yorigin + i*col_width);
		xy.vertices.push_back(xy.xorigin + 5);
	}

	double ierrmargin = nrows - (int)nrows;
	double erroffset = ierrmargin * col_width;

	for(int i=-(hscr/2);i<=hscr/2;i+=col_width){
		xy.vertices.push_back(xy.yorigin);
		xy.vertices.push_back(xy.xorigin - (i+erroffset/2));
		xy.vertices.push_back(xy.yorigin + 5);
		xy.vertices.push_back(xy.xorigin - (i+erroffset/2));
	}
}

void init_plane(plane& xy){
	/* initial plane type configuration */
	if(xy.plane_type == 0){
		/* y axis */
		xy.vertices.push_back((float)wscr/2); xy.vertices.push_back((float)hscr);
		xy.vertices.push_back((float)wscr/2); xy.vertices.push_back(0.0f);
		/* x axis */
		xy.vertices.push_back(0.0f); xy.vertices.push_back((float)hscr/2);
		xy.vertices.push_back(wscr); xy.vertices.push_back((float)hscr/2);

		xy.xorigin = (float)(hscr/2);
		xy.yorigin = (float)(wscr/2);
	}

	graph_mark_lines(xy);

	xy.VAO = 1;
	xy.VBO = 1;

	glGenVertexArrays(1, &xy.VAO);
	glBindVertexArray(xy.VAO);

	glGenBuffers(1, &xy.VBO);
	glBindBuffer(GL_ARRAY_BUFFER, xy.VBO);
	glBufferData(GL_ARRAY_BUFFER, xy.vertices.size() * sizeof(float), xy.vertices.data(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
}

/* function for 2 variable graphs */
void graphxy(graph& f, plane xy, const std::function<bool(double, double)>& fn){
	float col_width = wscr / xy.partitions;
	float nrows = hscr / col_width;
	double graph_precision = 0.005;
	f.vertices.reserve(500000);
	/* estimate time that it will take to do the computation */
	double d = 115 * std::pow((xy.partitions/graph_precision), 2);
	double clock_speed = 3.5; /* core speed (3.5ghz)*/
	double seconds = d / (clock_speed * std::pow(10, 9));
	std::cout << "Vertices calculations will take " << seconds / 3600 << "~ hours or " << seconds / 60 << "~ minutes." << std::endl;

	for(double x=-((xy.partitions/2)); x<=(xy.partitions/2) ;x+=graph_precision){
		for(double y=-(nrows/2);y<=(nrows/2);y+=graph_precision){
			if(fn(x,y)){
				f.vertices.push_back(xy.yorigin + x*col_width);
				f.vertices.push_back(xy.xorigin - y*col_width);
			}
		}
	}
	std::cout << "Graph vertices size: " << f.vertices.size() << std::endl;
}

/* function for single variable graphs */
void graphf(graph& f, plane xy, const std::function<double(double)>& fn){
        float col_width = wscr / xy.partitions;
        float nrows = hscr / col_width;

        /* initial vertice*/
        double ox = -(xy.partitions/2), oy = fn(ox);
        double graph_precision = 0.0001, y;

        for(double x=-((xy.partitions/2) - graph_precision); x<=(xy.partitions/2) ;x+=graph_precision){
                /* distance between vertices */
                y = fn(x);
                double dx = x - ox, dy = y - oy;
                double distance = std::hypot(dx, dy);
                /* for functions with asymptotes this removes inaccuracy */
                if(distance > (nrows)*0.95){
                        ox = x; oy = y;
                        continue;
                }

                f.vertices.push_back(xy.yorigin + ox*col_width);
                f.vertices.push_back(xy.xorigin - oy*col_width);

                f.vertices.push_back(xy.yorigin + x*col_width);
                f.vertices.push_back(xy.xorigin - y*col_width);

                ox = x; oy = y;
        }
        std::cout << "Graph vertices size: " << f.vertices.size() << std::endl;
}

void init_graph(graph& f){
	if(f.initialized) return;

	f.VAO = 1;
	f.VBO = 1;

	glGenVertexArrays(1, &f.VAO);
	glBindVertexArray(f.VAO);

	glGenBuffers(1, &f.VBO);
	glBindBuffer(GL_ARRAY_BUFFER, f.VBO);
	glBufferData(GL_ARRAY_BUFFER, f.vertices.size() * sizeof(float), f.vertices.data(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	f.initialized = true;
}

void draw_plane(plane& xy){
	if(xy.vertices[6] != wscr || xy.vertices[1] != hscr){
		std::cout << "If Called, resolution changed" << std::endl;
		xy.plane_configuration();
		glBindBuffer(GL_ARRAY_BUFFER, xy.VBO);
		glBufferSubData(GL_ARRAY_BUFFER, 0, xy.vertices.size() * sizeof(float), xy.vertices.data());
	}

	glBindVertexArray(xy.VAO);
	glDrawArrays(GL_LINES, 0, xy.vertices.size()/2);
	glLineWidth(1.0f);
	glBindVertexArray(0);

}

void draw_graph(graph& f, int type = 1){
	glBindVertexArray(f.VAO);

	/* Use type = 2 for two variable graphs, GL_LINES doesn't work well with them */
	if(type == 1){
		glDrawArrays(GL_LINES, 0, f.vertices.size()/2);
	}
	else if(type==2){
		glDrawArrays(GL_POINTS, 0, f.vertices.size());
	}

	glLineWidth(1.0f);
	glBindVertexArray(0);
}

int main(void){
    if(!glfwInit()) { /* failed */ }
    
    GLFWwindow* win = glfwCreateWindow(wscr, hscr, "plot", nullptr, nullptr);
	if(!win) {
		std::cout << "failed to create windows" << std::endl;
		glfwTerminate();
		return -1;
	}

	/* fps */
	const double fps = 20.0;
	const double fps_time = 1.0 / fps;
	double startTime;
	double endTime;
	double frameTime;
	double sleepTime;
	double lastTime = glfwGetTime();


    glfwMakeContextCurrent(win);
	glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    	std::cerr << "Failed to initialize GLAD" << std::endl;
    	return -1;
	}

	glViewport(0,0, wscr, hscr);
	glfwSetFramebufferSizeCallback(win, framebuffer_size_callback);

	glm::vec3 grayColor = glm::vec3(0.5f, 0.5f, 0.5f);
	glm::vec3 whiteColor = glm::vec3(1.0f, 1.0f, 1.0f);
	glm::vec3 greenColor = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 blueColor = glm::vec3(0.55f, 0.75f, 0.85f);
	glm::vec3 redColor = glm::vec3(1.0f, 0.0f, 0.0f);

	Shader shader("shader/shader.vs", "shader/shader.fs");

	plane xy;
	init_plane(xy);
	std::vector<graph> functions;

	auto cool = [](auto x, auto y){
		/* this return equals the equation: sqrt(x^2 + y^2) = cos(xy), since we cannot use == for decimal points 
		 * we calculate it this way using a error margin of 0.02 (points that fulfill this inequation will be rendered)*/
		return std::abs((std::sin((x*x + y*y)) - std::cos(x*y))) < 0.02;
	};

	auto cool_square = [](auto x, auto y){
		return std::abs((std::sin((x*x + y*y)) - cos(2*x*y))) < 0.02;
	};

	auto circle = [](auto x, auto y){
		return std::abs((x*x + y*y) - 9) < 0.1;
	};

	graph f1;
	graphxy(f1, xy, cool);
	init_graph(f1);

    while(!glfwWindowShouldClose(win)){
		startTime = glfwGetTime();

		processInput(win);
		shader.setUProjection("uProjection", projection);

		glClear(GL_COLOR_BUFFER_BIT);
		shader.use();

		/* keyboard input */
		if(glfwGetKey(win, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS && glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS){
			const std::string fname = "screenshot.png";
			save_png(fname, wscr, hscr);
		}

		shader.setRenderColor("renderColor", whiteColor);
		draw_plane(xy);

		/* Put your functions here */
		shader.setRenderColor("renderColor", redColor);
		draw_graph(f1, 2);

		glfwSwapBuffers(win);
		glfwPollEvents();

		/* fps controlling block */
		endTime = glfwGetTime();
		frameTime = endTime - startTime;
		if(frameTime < fps_time){
			sleepTime = fps_time - frameTime;
			std::this_thread::sleep_for(std::chrono::duration<double>(sleepTime));
		}
    }

	xy.clean();
	for(int i=0;i<functions.size();i++){
		functions[i].clean();
	}

    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
