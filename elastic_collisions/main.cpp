#include "glad/glad.h"
#include "shader/shader.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <cmath>

int scrWidth = 1354;
int scrHeight = 724;
glm::mat4 projection = glm::ortho(0.0f, (float)scrWidth, 0.0f, (float)scrHeight); // left right up down

/* lame variables for input control */
bool spawn_ball = 0;

float randFloat(){
	return (float)(rand()) / (float)(RAND_MAX);
}
glm::vec3 randColor(){
	return glm::vec3(randFloat(), randFloat(), randFloat());
}

struct ball{
	std::vector<float> position = {scrWidth / 2.0f, scrHeight / 2.0f};
	std::vector<float> velocity = {0.0f, 0.0f};
	glm::vec3 color;
	float radius = 20;
	float mass = 2.0f;
	int segments = 100;
	void updateAccel(float x, float y){
		velocity[0] += (x / 50.0f); // the 50.0f is for scalling
		velocity[1] -= (y / 50.0f);
	}
	void updatePos(){
		position[0] += velocity[0];
		position[1] -= velocity[1];
	}
	void checkColision(){
		if(position[1] < 0 + radius){
			position[1] = 0 + radius;
			velocity[1] *= -1;
		}
		else if(position[1] > scrHeight - radius){
			position[1] = scrHeight - radius;
			velocity[1] *= -1;
		}
		if(position[0] < 0 + radius){
			position[0] = 0 + radius;
			velocity[0] *= -1;
		}
		else if(position[0] > scrWidth - radius){
			position[0] = scrWidth - radius;
			velocity[0] *= -1;
		}	
	}
	
	void movementMode(int mode, float xm, float ym, float x, float y){
		if(mode == 0){
			/* if mode is 0 (norma) then just use mouse coordinates as velocity vector values and ignore the rest */
			updateAccel(xm,ym);
		}
		else if(mode == 1){
			float xd, yd, distance;
			xd = xm-position[0];
			yd = ym-position[1];
			distance = std::sqrt(xd*xd + yd*yd);
			float xn = xd/ distance, yn = yd/distance;

			float vm = (x*x) + (y*y);

			updateAccel(vm*xn, vm*yn);
			std::cout << "direction x: " << x*xn << ", direction y: " << y*yn << std::endl;
		}
	}
};

void add_ball(std::vector<ball>& p, float mx, float my){
	ball c;
	c.color = randColor();
	c.position[0] = mx; c.position[1] = my;
	p.push_back(c);
}

static void key_callback(GLFWwindow* win, int key, int scancode, int action, int mods){
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS){
		glfwSetWindowShouldClose(win, GLFW_TRUE);
    }
	if(key == GLFW_KEY_ENTER && action == GLFW_PRESS){
		spawn_ball = 1;
	}
}

/* basically what to do if window is resized */
void framebuffer_size_callback(GLFWwindow* win, int width, int height){
	glViewport(0,0,width,height);
	scrHeight = height;
	scrWidth = width;
	projection = glm::ortho(0.0f, (float)scrWidth, 0.0f, (float)scrHeight);
}

void processInput(GLFWwindow* win){
	if(glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS){
		glfwSetWindowShouldClose(win,true);
	}
}

std::vector<float> create_circle(ball c){
	std::vector<float> vertices;
	float x,y;
	/* we're using GL_TRIANGLE_FAN so first two coordinates need to be the center */
	vertices.push_back(c.position[0]); vertices.push_back(c.position[1]);

	for(int i=0;i<=c.segments;i++){
		float angle = 2.0f * M_PI * i / c.segments;
		x = c.position[0] + c.radius * std::cos(angle);
		y = c.position[1] + c.radius * std::sin(angle);
		vertices.push_back(x); vertices.push_back(y);
	}
	
	return vertices;
}

void draw_circle(ball c){
	std::vector<float> circleVertices = create_circle(c);

	unsigned int VAO;
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	unsigned int VBO;
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, circleVertices.size() * sizeof(float), circleVertices.data(), GL_DYNAMIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(VAO);
	glDrawArrays(GL_TRIANGLE_FAN, 0, c.segments+2);
	glBindVertexArray(0);
}

void ball_collision(ball& x, ball& y){
	float x_distance = x.position[0] - y.position[0];
	float y_distance = x.position[1] - y.position[1];
	float distance = std::sqrt(x_distance*x_distance + y_distance*y_distance);
	float center_distance = x.radius + y.radius;
	if(distance < center_distance){
		float xdirection = x_distance / distance;
		float ydirection = y_distance / distance;

		float overlap = center_distance - distance;
		float separationX = xdirection * overlap / 2.0f;
        float separationY = ydirection * overlap / 2.0f;

        // Move balls apart to resolve overlap
        x.position[0] += separationX;
        x.position[1] += separationY;
        y.position[0] -= separationX;
        y.position[1] -= separationY;

		// componente normal
		std::vector<float> n = {xdirection, ydirection};
		float v1n = (x.velocity[0] * n[0]) + (x.velocity[1] * n[1]);
		float v2n = (y.velocity[0] * n[0]) + (y.velocity[1] * n[1]);


		//componente tangencial
		std::vector<float> t = {-n[1],n[0]};

		float v1t = (x.velocity[0] * t[0]) + (x.velocity[1] * t[1]);
		float v2t = (y.velocity[0] * t[0]) + (y.velocity[1] * t[1]);

		// choque elastico
		v1n = (((x.mass - y.mass)*v1n) + (2*y.mass*v2n)) / (x.mass + y.mass);
		v2n = (((y.mass - x.mass)*v2n) + (2*x.mass*v1n)) / (x.mass + y.mass);

		x.velocity = { ((v1n*n[0]) + (v1t*t[0])), ((v1n*n[1]) + (v1t*t[1])) };
		y.velocity = { ((v2n*n[0]) + (v2t*t[0])), ((v2n*n[1]) + (v2t*t[1])) };
	}
}

int main(void){
    if(!glfwInit()) { /* failed */ }
	srand(time(NULL));
    
    GLFWwindow* win = glfwCreateWindow(scrWidth, scrHeight, "elastic collision", NULL, NULL);
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

	glViewport(0,0,scrWidth, scrHeight);


    glfwSetKeyCallback(win, key_callback);
	glfwSetFramebufferSizeCallback(win, framebuffer_size_callback); /* callback if win has been resized */

	float centerx = scrWidth / 2.0f, centery = scrHeight/2.0;
	std::cout << scrWidth << "x" << scrHeight << std::endl;
	std::cout << centerx << "x" << centery<< std::endl;

	ball c1, c2, c3, c4;
	c1.color = randColor();
	c2.color = randColor();
	c3.color = randColor();
	c4.color = randColor();
	c1.position[1] = scrHeight - c1.radius;
	c3.position[1] = c1.position[1]; c3.position[0] = scrWidth - c3.radius;
	c4.position[0] = c4.radius; c4.position[1] = scrHeight - c4.radius;

	std::vector<ball> balls = {c1, c2, c3, c4};
	Shader shader("shader/shader.vs", "shader/shader.fs");

	double mousex, mousey;
	float mx, my;
	int gravity_mode = 0;

    while(!glfwWindowShouldClose(win)){
		/* proccessInput only does basic things*/
		processInput(win);

		if(spawn_ball){
			add_ball(balls, mx, my);
			spawn_ball = 0;
		}
		if(glfwGetKey(win, GLFW_KEY_SPACE) == GLFW_PRESS){
			if(gravity_mode == 0){
				gravity_mode = 1;
			}
			else gravity_mode = 0;
		}

		glfwGetCursorPos(win, &mousex, &mousey);
		mx = static_cast<float>(mousex);
		my = static_cast<float>(mousey);
		my = scrHeight - my;

		shader.setUProjection("uProjection", projection);

		glClear(GL_COLOR_BUFFER_BIT);

		shader.use();
		for(int i=0;i<balls.size();i++){
			shader.setBallColor("ballColor", balls[i].color);
			draw_circle(balls[i]);
			balls[i].checkColision();
			balls[i].updatePos();

			if(gravity_mode == 0){
				balls[i].movementMode(0,0, -9.81, 0,0);
			}
			else if(gravity_mode == 1){
				balls[i].movementMode(1, mx, my, 2,-8.2);
			}

			for(int j=i+1;j<balls.size();j++){
				ball_collision(balls[i], balls[j]);
			}
		}

		glfwSwapBuffers(win);
		glfwPollEvents();
    }

    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
