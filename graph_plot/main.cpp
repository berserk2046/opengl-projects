#include "glad/glad.h"
#include "shader/shader.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <thread>
#include <chrono>
#include <functional>

int wscr = 1354;
int hscr = 717;
glm::mat4 projection = glm::ortho(0.0f, (float)wscr, (float)hscr,0.0f); /* left right up down */

struct plane{
	unsigned int VAO = 0;
	unsigned int VBO = 0;
	int plane_type = 0;
	/* partitions decide the size and quality of the graph */
	int partitions = 6;
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
		if(plane_type == 1){
			/* y axis */
			vertices[0] = 0.0f; vertices[1] = (float)hscr;
			vertices[2] = 0.0f; vertices[3] = 0.0f;
			/* x axis */
			vertices[4] = (0.0f); vertices[5] = 0.0f;
			vertices[6] = (float)wscr; vertices[7] = 0.0f;

			xorigin = (0.0f);
			yorigin = (0.0f);
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

/*sets the lines partitions lines along the x and y axis for visual purposes*/
void graph_mark_lines(plane& xy){
	float col_width = wscr / xy.partitions;
	float nrows = hscr / col_width, ncols = wscr / col_width;

	/* plane_type regulations */
	double xmarklines_lb, xmarklines_ub;
	double ymarklines_lb, ymarklines_ub;
	if(xy.plane_type == 0){
		xmarklines_lb = -(xy.partitions/2);
		xmarklines_ub = xy.partitions/2;
		ymarklines_lb = -(hscr/2);
		ymarklines_ub = hscr/2;
	}
	else if(xy.plane_type == 1){
		xmarklines_lb = 0.0f;
		xmarklines_ub = xy.partitions;
		ymarklines_lb = 0.0f;
		ymarklines_ub = hscr;
	}

	/* mark lines vertices */
	/* # of pixels mark lines uses, this only takes even numbers. */
	int mlsize = 6;

	/* X axis mark lines */
	for(double i=xmarklines_lb;i<=xmarklines_ub;i++){
		xy.vertices.push_back(xy.yorigin + i*col_width);
		xy.vertices.push_back(xy.xorigin - 3);
		xy.vertices.push_back(xy.yorigin + i*col_width);
		xy.vertices.push_back(xy.xorigin + 3);
	}

	/* Y axis mark lines NOTE: This should be in only 1 for loop ideally
	 * should work with margin offsets but decimal precision is kinda tricky */
	for(double i=0;i<=ymarklines_ub;i+=col_width){
		xy.vertices.push_back(xy.yorigin - (mlsize/2));
		xy.vertices.push_back(xy.xorigin - (i));
		xy.vertices.push_back(xy.yorigin + (mlsize/2));
		xy.vertices.push_back(xy.xorigin - (i));
	}
	for(double i=0;i>=ymarklines_lb;i-=col_width){
		xy.vertices.push_back(xy.yorigin - (mlsize/2));
		xy.vertices.push_back(xy.xorigin - (i));
		xy.vertices.push_back(xy.yorigin + (mlsize/2));
		xy.vertices.push_back(xy.xorigin - (i));
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
	else if(xy.plane_type == 1){
		/* y axis */
		xy.vertices.push_back(3.0f); xy.vertices.push_back((float)hscr);
		xy.vertices.push_back(3.0f); xy.vertices.push_back(0.0f);
		/* x axis */
		xy.vertices.push_back(0.0f); xy.vertices.push_back((float)hscr-3);
		xy.vertices.push_back((float)wscr); xy.vertices.push_back((float)hscr-3);

		xy.xorigin = (float)hscr-3;
		xy.yorigin = 3.0f;
	}

	graph_mark_lines(xy);
	std::cout << "Plane vertices size: " << xy.vertices.size() << std::endl;

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

void graphf(graph& f, plane xy, const std::function<double(double)>& fn){
	double col_width = wscr / xy.partitions;
	double nrows = hscr / col_width;
	double graph_precision = 0.001;

	/* plane_type regulations */
	double xlb, xub;
	if(xy.plane_type == 0){
		xlb = -((xy.partitions/2) - graph_precision);
		xub = xy.partitions/2;
	}
	else if(xy.plane_type == 1){
		xlb = graph_precision;
		xub = xy.partitions;
	}
	/* initial vertices */
	double ox = xlb - graph_precision, oy = fn(ox), y;


	for(double x=xlb; x<=xub ;x+=graph_precision){
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

void draw_graph(graph& f){
	glBindVertexArray(f.VAO);
	glDrawArrays(GL_LINES, 0, f.vertices.size()/2);
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
	xy.plane_type = 0;
	init_plane(xy);

	auto E = [](auto x){
		return (0.1 + std::pow(std::sin(x),2) + 0.35 * std::pow(2.7182, (-1/2)*std::pow(x/0.4, 2)));
	};
	auto H = [](auto x){
		return std::cos(21*x);
	};
	auto G = [](auto x){
		return std::pow(2.7182, (-1/2)*std::pow((x-0.15)/0.10, 2));
	};

	auto AM = [E, H, G](auto x){
		return E(x) * H(x) * (1 - ((1/2)*G(x))) + (1/6)*G(x);
	};

	graph f1;
	graphf(f1, xy, AM);
	init_graph(f1);


    while(!glfwWindowShouldClose(win)){
		startTime = glfwGetTime();

		processInput(win);
		shader.setUProjection("uProjection", projection);

		glClear(GL_COLOR_BUFFER_BIT);
		shader.use();

		shader.setRenderColor("renderColor", whiteColor);
		draw_plane(xy);

		/* Put your functions here */
		shader.setRenderColor("renderColor", greenColor);
		draw_graph(f1);

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

    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
