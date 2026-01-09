#include "glad/glad.h"
#include "shader/shader.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <thread>
#include <chrono>
#include <functional>

typedef uint32_t u32;
typedef uint64_t u64;
typedef uint16_t u16;
typedef uint8_t u8;

int wscr = 1320;
int hscr = 720;
int bsz = 10;
int ncols = wscr/bsz;
int nrows = hscr/bsz;
glm::mat4 projection = glm::ortho(0.0f, (float)wscr, (float)hscr,0.0f); /* left right up down */

struct plane{
	u32 VAO;
	u32 VBO;
	std::vector<float> vertices;
};

struct life{
	u32 VAO;
	u32 VBO;
	u32 VBO_instanced;
	std::vector<u8> game_state;
	std::vector<float> cell;
};

void framebuffer_size_callback(GLFWwindow* win, int width, int height){
	glViewport(0,0,width,height);
	//projection = glm::ortho(0.0f, (float)wscr, (float)hscr,0.0f); /* left right up down */
}

void processInput(GLFWwindow* win){
	if(glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS){
		glfwSetWindowShouldClose(win,true);
	}
}

void init_plane(plane& world){
	for(int i=0;i<nrows;i++){
		world.vertices.push_back(0.0f); world.vertices.push_back(i*bsz);
		world.vertices.push_back(wscr); world.vertices.push_back(i*bsz);
	}
	for(int i=0;i<ncols;i++){
		world.vertices.push_back(i*bsz); world.vertices.push_back(0.0f);
		world.vertices.push_back(i*bsz); world.vertices.push_back(hscr);
	}

	glGenVertexArrays(1, &world.VAO);
	glBindVertexArray(world.VAO);

	glGenBuffers(1, &world.VBO);
	glBindBuffer(GL_ARRAY_BUFFER, world.VBO);
	glBufferData(GL_ARRAY_BUFFER, world.vertices.size() * sizeof(float), world.vertices.data(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
}

int get_alive(std::vector<u8>& game_state){
	int alive = 0;
	for(int i=0;i<game_state.size();i++){
		if(game_state[i] == 1){ alive++; }
	}
	return alive;
}

void update_creatures(life& p, int init = 0){
	int alive = get_alive(p.game_state);
	std::vector<glm::vec2> translations;
	translations.reserve(alive);
	int index = 0;

	for(int i=0;i<p.game_state.size();i++){
		if(p.game_state[i] == 1){
			int px = (i%ncols), py = i/ncols;
			translations.emplace_back(px*bsz, py*bsz);
		}
	}

	glBindBuffer(GL_ARRAY_BUFFER, p.VBO_instanced);

	if(!translations.empty()){ glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * alive, &translations[0], GL_STATIC_DRAW); }
	else{ glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_STATIC_DRAW); }

	glBindVertexArray(p.VAO);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glVertexAttribDivisor(1, 1);

}

void init_life(life& p){
	int ncreature = (wscr*hscr) / (bsz*bsz);
	p.game_state.reserve(ncreature);

	/* cell quad info */
	p.cell.push_back(0); p.cell.push_back(bsz);
	p.cell.push_back(bsz); p.cell.push_back(0);
	p.cell.push_back(0); p.cell.push_back(0);
	p.cell.push_back(bsz); p.cell.push_back(0);
	p.cell.push_back(bsz); p.cell.push_back(bsz);
	p.cell.push_back(0); p.cell.push_back(bsz);

	glGenVertexArrays(1, &p.VAO);
	glGenBuffers(1, &p.VBO);
	glGenBuffers(1, &p.VBO_instanced);

	glBindVertexArray(p.VAO);
	glBindBuffer(GL_ARRAY_BUFFER, p.VBO);
	glBufferData(GL_ARRAY_BUFFER, p.cell.size() * sizeof(float), p.cell.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), (void*)0);
	glBindVertexArray(0);

	/* Define dead or alive for each cell */
	for(int i=0;i<ncreature;i++){
		p.game_state.emplace_back(rand()%2);
	}

	update_creatures(p);
}

void draw_plane(plane& world){
	glBindVertexArray(world.VAO);
	glDrawArrays(GL_LINES, 0, world.vertices.size()/2);
	glLineWidth(1.0f);
	glBindVertexArray(0);
}

void draw_life(life& n){
	glBindVertexArray(n.VAO);
	glDrawArraysInstanced(GL_TRIANGLES, 0, 6, get_alive(n.game_state));
	glBindVertexArray(0);
}

u8 get_neighbors(std::vector<u8> state, int position){
	u8 neighbors = 0;
	u16 px = (position%ncols), py = position/ncols;

	for(int8_t dy=-1;dy<=1;dy++){
		for(int8_t dx=-1;dx<=1;dx++){
			if(dx==0 && dy==0){ continue; }
			u32 x = px + dx;
			u32 y = py + dy;
			if(x >= 0 && x < ncols && y >= 0 && y < nrows){
				neighbors += state[y*ncols + x];
			}
		}
	}
	return neighbors;
}

void update_game_state(life& n){
	std::vector<u8> new_game_state(n.game_state.size());
	for(int i=0;i<n.game_state.size();i++){
		/*neighborhood*/
		u8 neighbors = get_neighbors(n.game_state, i);
		/* Rules */
		if(n.game_state[i] == 1 && neighbors < 2){ new_game_state[i] = 0; } /* underpopulation */
		if(n.game_state[i] == 1 && (neighbors == 2 || neighbors == 3)){ new_game_state[i] = 1; } /* survival */
		if(n.game_state[i] == 1 && neighbors > 3){ new_game_state[i] = 0; } /* overpopulation */
		if(n.game_state[i] == 0 && neighbors == 3){ new_game_state[i] = 1; } /* overpopulation */
	}
	n.game_state = new_game_state;
	update_creatures(n);
}

void waitm(int m){
	std::this_thread::sleep_for(std::chrono::milliseconds(m));
}

int main(void){
    if(!glfwInit()) { /* failed */ }
	srand(time(0));
    
    GLFWwindow* win = glfwCreateWindow(wscr, hscr, "plot", nullptr, nullptr);
	if(!win) {
		std::cout << "failed to create windows" << std::endl;
		glfwTerminate();
		return -1;
	}

	/* Mouse coords */
	double xpos, ypos;

	/* fps */
	const double fps = 60.0;
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

	glm::vec3 whiteColor = glm::vec3(1.0f, 1.0f, 1.0f);
	glm::vec3 grayColor = glm::vec3(0.5f, 0.5f, 0.5f);

	Shader shader("shader/shader.vs", "shader/shader.fs");

	plane world;
	init_plane(world);
	life conway;
	init_life(conway);
	int load = 0;

    while(!glfwWindowShouldClose(win)){
		startTime = glfwGetTime();

		processInput(win);
		shader.setUProjection("uProjection", projection);

		glClear(GL_COLOR_BUFFER_BIT);
		shader.use();
		/* keyboard */
		if(glfwGetKey(win, GLFW_KEY_SPACE) == GLFW_PRESS){
			if(load){ load=0; }
			else{ load=1; }
			waitm(250);
		}
		if(glfwGetKey(win, GLFW_KEY_R) == GLFW_PRESS){
			for(int i=0;i<conway.game_state.size();i++){
				conway.game_state[i] = 0;
			}
			update_creatures(conway);
			waitm(250);
		}
		if(glfwGetKey(win, GLFW_KEY_G) == GLFW_PRESS){
			for(int i=0;i<conway.game_state.size();i++){
				conway.game_state[i] = rand() % 2;
			}
			update_creatures(conway);
			waitm(250);
		}

		/* mouse */
		if(glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS){
			glfwGetCursorPos(win, &xpos, &ypos);
			int px = static_cast<int>(xpos/bsz), py = static_cast<int>(ypos/bsz);
			if(conway.game_state[py*ncols + px] == 1){ conway.game_state[py*ncols + px] = 0; }
			else{ conway.game_state[py*ncols + px] = 1; }
			update_creatures(conway);
			waitm(250);
		}
		
		/* drawing */
		shader.setRenderColor("renderColor", whiteColor);
		draw_life(conway);
		/*shader.setRenderColor("renderColor", grayColor);
		draw_plane(world); */
		if(load) { update_game_state(conway); }

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

    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
