#include "glad/glad.h"
#include "shader/shader.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>
#include <thread>

/* config variables */
int scrWidth = 1920;
int scrHeight = 1001;

/* advances roughly at one hour per frame*/
float time_change = 3600;

glm::mat4 projection = glm::ortho(0.0f, (float)scrWidth, (float)scrHeight, 0.0f); // left right up down
glm::vec2 cameraPos;
float cameraZoom = 0.1f;

/* constants */
double G = 6.674 * std::pow(10, -11);

struct planet{
	std::string name;
	std::vector<float> position = {scrWidth / 2.0f, scrHeight / 2.0f};
	std::vector<double> velocity = {0.0f, 0.0f};
	std::vector<double> accel = {0.0f, 0.0f};
	glm::vec3 color;
	float radius = 20;
	double mass = 2.0f;
	int segments = 100;
	unsigned int VAO = 0;
	unsigned int VBO = 0;
	bool initialized = false;

	void updatePos(float t){
		position[0] += velocity[0] * t;
		position[1] += velocity[1] * t;
	}

};

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	/* Set cameraPos on zoom position */
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	glm::vec2 mworld;
	mworld.x = ((float)xpos + cameraPos.x) / cameraZoom;
	mworld.y = ((float)ypos + cameraPos.y) / cameraZoom;

    /* Increase or decrease zoom by a fraction based on scroll delta */
    cameraZoom *= (1.0f + yoffset * 0.1f);
    /* Cap zoom level so it doesn't get too small or too big */
    if (cameraZoom < 0.00000001f) cameraZoom = 0.0001f;
    if (cameraZoom > 20.0f) cameraZoom = 20.0f;
	cameraPos.x = mworld.x * cameraZoom - (float)xpos;
	cameraPos.y = mworld.y * cameraZoom - (float)ypos;

}

static void key_callback(GLFWwindow* win, int key, int scancode, int action, int mods){
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS){
		glfwSetWindowShouldClose(win, GLFW_TRUE);
    }
    if(key == GLFW_KEY_W && action == GLFW_REPEAT){
	    cameraPos.y += 30;
    }
    if(key == GLFW_KEY_S && action == GLFW_REPEAT){
	    cameraPos.y -= 30;
    }
    if(key == GLFW_KEY_A && action == GLFW_REPEAT){
	    cameraPos.x -= 30;
    }
    if(key == GLFW_KEY_D && action == GLFW_REPEAT){
	    cameraPos.x += 30;
    }
}

/* basically what to do if window is resized */
void framebuffer_size_callback(GLFWwindow* win, int width, int height){
	glViewport(0,0,width,height);
	scrHeight = height;
	scrWidth = width;
	projection = glm::ortho(0.0f, (float)scrWidth,0.0f, (float)scrHeight);
	
}

void processInput(GLFWwindow* win){
	if(glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS){
		glfwSetWindowShouldClose(win,true);
	}
}

std::vector<float> create_circle(planet c){
	std::vector<float> vertices;
	float centerx = (scrWidth / 2), centery = (scrHeight / 2), x, y;
	double pexp = 5.5;

	vertices.push_back(c.position[0]*std::pow(10, -pexp) + centerx); vertices.push_back(c.position[1]*std::pow(10, -pexp) + centery);

	for(int i=0;i<=c.segments;i++){
		float angle = 2.0f * M_PI * i / c.segments;
		x = (c.position[0] * std::pow(10, -pexp) + centerx) + c.radius * std::cos(angle);
		y = (c.position[1] * std::pow(10, -pexp) + centery) + c.radius * std::sin(angle);
		vertices.push_back(x); vertices.push_back(y);
	}
	return vertices;
}

void init_planet(planet& c){
	if(c.initialized) return;

	std::vector<float> circleVertices = create_circle(c);

	c.VAO = 1;
	c.VBO = 1;
	glGenVertexArrays(1, &c.VAO);
	glBindVertexArray(c.VAO);

	glGenBuffers(1, &c.VBO);
	glBindBuffer(GL_ARRAY_BUFFER, c.VBO);
	glBufferData(GL_ARRAY_BUFFER, circleVertices.size() * sizeof(float), circleVertices.data(), GL_DYNAMIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(c.VAO);
	glDrawArrays(GL_TRIANGLE_FAN, 0, c.segments+2);
	glBindVertexArray(0);
	c.initialized = true;
}

void draw_circle(planet& c){
	std::vector<float> circleVertices = create_circle(c);

	glBindBuffer(GL_ARRAY_BUFFER, c.VBO);
	glBufferData(GL_ARRAY_BUFFER, circleVertices.size() * sizeof(float), circleVertices.data(), GL_DYNAMIC_DRAW);

	glBindVertexArray(c.VAO);
	glDrawArrays(GL_TRIANGLE_FAN, 0, c.segments+2);
	glBindVertexArray(0);
}

void planet_orbit(planet& p1, planet& p2){
	/* distances */
	double x = p2.position[0] - p1.position[0], y = p2.position[1] - p1.position[1];
	std::vector<double> r = {x,y};
	double v_length = std::sqrt(x*x + y*y);
	std::vector<double> rdir = {x / v_length, y / v_length};

	/* magnitude */
	double force_magnitude = (G * p1.mass * p2.mass) / std::pow(v_length, 2);
	std::vector<double> force_direction = {force_magnitude*rdir[0], force_magnitude*rdir[1]};
	std::vector<double> a1 = { force_direction[0]/p1.mass, force_direction[1]/p1.mass };

	/* apply velocity */
	p1.velocity[0] += a1[0] * time_change;
	p1.velocity[1] += a1[1] * time_change;
}

void waitm(int m){
	std::this_thread::sleep_for(std::chrono::milliseconds(m));
}

int main(void){
    if(!glfwInit()) { /* failed */ }
    
    GLFWwindow* win = glfwCreateWindow(scrWidth, scrHeight, "orbit", NULL, NULL);
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
	glfwSetScrollCallback(win, scroll_callback);
	glfwSetFramebufferSizeCallback(win, framebuffer_size_callback); /* callback if win has been resized */

	planet earth, venus, sun, mercury, mars, jupiter, saturn, uranus, neptune, moon;

	mercury.color = glm::vec3(0.412f, 0.412f, 0.412f);
	venus.color = glm::vec3(0.5f, 0.5f, 0.5f);
	earth.color = glm::vec3(0.0f, 0.5f, 0.0f);
	moon.color = glm::vec3(0.5f, 0.5f, 0.5f);
	mars.color = glm::vec3(0.737f, 0.153f, 0.196f);
	sun.color = glm::vec3(1.0f, 1.0f, 0.0f);
	jupiter.color = glm::vec3(0.85f, 0.65f, 0.45f);
	saturn.color = glm::vec3(0.95f, 0.85f, 0.55f);
	uranus.color = glm::vec3(0.55f, 0.75f, 0.85f);
	neptune.color = glm::vec3(0.3f, 0.45f, 0.85f);

	sun.name = "Sun";
	sun.mass = 1.9885 * std::pow(10,30);

	/*Variable name guides
	 * smAxis = Distance to the sun
	 * Perihelion = point in a planets orbit where it's closest to the sun
	 * E = Elipse excentricity
	 * initv = initial velocity*/

	mercury.name = "Mercury";
	mercury.mass = 3.30 * std::pow(10, 23);
	double mercury_smAxis= 57910000000;
	double mercuryE = 0.2056;
	double mercuryPerihelion = mercury_smAxis*(1-mercuryE);
	double mercury_initv = std::sqrt(G*sun.mass * ((2/mercuryPerihelion)-(1/mercury_smAxis)));
	mercury.position[0] = sun.position[0] - mercuryPerihelion;

	venus.name = "Venus";
	venus.mass = 4.867 * std::pow(10, 24);
	double venus_smAxis = 108200000000;
	double venusE = 0.0068;
	double venusPerihelion = venus_smAxis*(1-venusE);
	double venus_initv = std::sqrt(G*sun.mass * ((2/venusPerihelion)-(1/venus_smAxis)));
	venus.position[0] = sun.position[0] - venusPerihelion;

	earth.name = "Earth";
	earth.mass = 5.972 * std::pow(10, 24);
	double earth_smAxis = 149600000000;
	double earthE = 0.0167;
	double earthPerihelion = earth_smAxis*(1-earthE);
	double earth_initv = std::sqrt(G*sun.mass * ((2/earthPerihelion)-(1/earth_smAxis)));
	earth.position[0] = sun.position[0] - earthPerihelion;

	moon.name = "Moon";
	moon.mass = 7.348 * std::pow(10, 22);
	double moon_emAxis = 384400000;
	double moonE = 0.0549;
	double moonPerigee = moon_emAxis*(1-moonE);
	double moon_initv = std::sqrt(G*earth.mass * ((2/moonPerigee)-(1/moon_emAxis)));
	moon.position[0] = earth.position[0] - moonPerigee;

	mars.name = "Mars";
	mars.mass = 6.42 * std::pow(10,23);
	double mars_smAxis = 227900000000;
	double marsE = 0.0934;
	double marsPerihelion = mars_smAxis*(1-marsE);
	double mars_initv = std::sqrt(G*sun.mass * ((2/marsPerihelion)-(1/mars_smAxis)));
	mars.position[0] = sun.position[0] - marsPerihelion;

	jupiter.name = "Jupiter";
	jupiter.mass = 1.896 * std::pow(10,27);
	double jupiter_smAxis = 778600000000;
	double jupiterE = 0.0489;
	double jupiterPerihelion = jupiter_smAxis*(1-jupiterE);
	double jupiter_initv = std::sqrt(G*sun.mass * ((2/jupiterPerihelion)-(1/jupiter_smAxis)));
	jupiter.position[0] = sun.position[0] - jupiterPerihelion;

	saturn.name = "Saturn";
	saturn.mass = 5.683 * std::pow(10,26);
	double saturn_smAxis = 1433500000000;
	double saturnE = 0.0565;
	double saturnPerihelion = saturn_smAxis*(1-saturnE);
	double saturn_initv = std::sqrt(G*sun.mass * ((2/saturnPerihelion)-(1/saturn_smAxis)));
	saturn.position[0] = sun.position[0] - saturnPerihelion;

	uranus.name = "Uranus";
	uranus.mass = 8.681 * std::pow(10,25);
	double uranus_smAxis = 2872500000000;
	double uranusE = 0.0457;
	double uranusPerihelion = uranus_smAxis*(1-uranusE);
	double uranus_initv = std::sqrt(G*sun.mass * ((2/uranusPerihelion)-(1/uranus_smAxis)));
	uranus.position[0] = sun.position[0] - uranusPerihelion;

	neptune.name = "Neptune";
	neptune.mass = 1.024 * std::pow(10,26);
	double neptune_smAxis = 4495100000000;
	double neptuneE = 0.0113;
	double neptunePerihelion = neptune_smAxis*(1-neptuneE);
	double neptune_initv = std::sqrt(G*sun.mass * ((2/neptunePerihelion)-(1/neptune_smAxis)));
	neptune.position[0] = sun.position[0] - neptunePerihelion;

	sun.radius = 696340 / std::pow(10, 1.2);
	/* other planets are on a different scale than the sun for visual purposes */
	float pS = 1;
	mercury.radius = 2439.7 / std::pow(10, pS);
	venus.radius = 6051.8 / std::pow(10, pS);
	earth.radius = 6371 / std::pow(10, pS);
	moon.radius = 1737.5 / std::pow(10, pS);
	mars.radius = 3389.5 / std::pow(10, pS);
	jupiter.radius = 69911 / std::pow(10, pS);
	saturn.radius = 58232 / std::pow(10, pS);
	uranus.radius = 25362 / std::pow(10, pS);
	neptune.radius = 24622 / std::pow(10, pS);

	mercury.velocity = {0.0, mercury_initv};
	venus.velocity = {0.0, venus_initv};
	earth.velocity = {0.0, earth_initv + ((-moon_initv * moon.mass)/earth.mass)};
	moon.velocity = {0.0, moon_initv+earth_initv};
	mars.velocity = {0.0, mars_initv};
	jupiter.velocity = {0.0, jupiter_initv};
	saturn.velocity = {0.0, saturn_initv};
	uranus.velocity = {0.0, uranus_initv};
	neptune.velocity = {0.0, neptune_initv};
	sun.velocity = {0.0, ((-earth_initv * earth.mass)/sun.mass)
		+ ((-venus_initv*venus.mass)/sun.mass) 
		+ ((-mercury_initv*mercury.mass)/sun.mass)
		+ ((-mars_initv*mars.mass)/sun.mass)
		+ ((-jupiter_initv*jupiter.mass)/sun.mass)
		+ ((-saturn_initv*saturn.mass)/sun.mass)
		+ ((-uranus_initv*uranus.mass)/sun.mass)
		+ ((-neptune_initv*neptune.mass)/sun.mass)};

	std::vector<planet> solar_system;
	solar_system.push_back(sun);
	solar_system.push_back(mercury);
	solar_system.push_back(venus);
	solar_system.push_back(earth);
	solar_system.push_back(moon);
	solar_system.push_back(mars);
	solar_system.push_back(jupiter);
	solar_system.push_back(saturn);
	solar_system.push_back(uranus);
	solar_system.push_back(neptune);

	for(int i=0;i<solar_system.size();i++){
		init_planet(solar_system[i]);
	}
	
	Shader shader("shader/shader.vs", "shader/shader.fs");

	glm::mat4 view;
	glm::mat4 vp;
	double mx, my;

    while(!glfwWindowShouldClose(win)){
		view = glm::translate(glm::mat4(1.0f), glm::vec3(-cameraPos, 0.0f));
		view = glm::scale(view, glm::vec3(cameraZoom, cameraZoom, 1.0f));
		vp = projection * view;

		if(glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT)==GLFW_PRESS){
			double nmx, nmy;
			glfwGetCursorPos(win,&mx,&my);
			waitm(10);
			glfwGetCursorPos(win,&nmx,&nmy);
			cameraPos.x += mx-nmx;
			cameraPos.y -= my-nmy;
		}

		processInput(win);
		shader.setUProjection("uProjection", vp);
		glClear(GL_COLOR_BUFFER_BIT);
		shader.use();

		for(int i=0;i<solar_system.size();i++){
			for(int j=0;j<solar_system.size();j++){
				if(j!=i){
					planet_orbit(solar_system[i], solar_system[j]);
				}
			}
			shader.setPlanetColor("planetColor", solar_system[i].color);
			draw_circle(solar_system[i]);
			solar_system[i].updatePos(time_change);
		}

		glfwSwapBuffers(win);
		glfwPollEvents();
    }

    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}

