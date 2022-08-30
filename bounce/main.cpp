#include<iostream>

#include<glad/glad.h>
#include<GLFW/glfw3.h>

#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>

#include"shader.h"
#include"camera.h"
#include"rigidbody.h"


const int SCR_WIDTH = 800;
const int SCR_HEIGHT = 600;

Camera camera(glm::vec3(-5,5,5));
ObjManager objManager;

float lastX = SCR_WIDTH/2.0f, lastY = SCR_HEIGHT/2;
bool firstMouse = true;

float deltaTime = 0;
float lastFrame = 0;


const float gravity = 9.8f;
float force = 100; // m/s^2 * m(N)


void mouseCallback(GLFWwindow* window, double xpos, double ypos);
void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void framebufferSizeCallback(GLFWwindow* window, int width, int height);

void processInput(GLFWwindow* window);
void setWall();
void makeBall();

using namespace std;

int main() {
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "bounce_ball", NULL, NULL);

	if (window == NULL) {
		cout << "FAILED TO CREATE GLFW WINDOW" << endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		cout << "FAILED TO INITIALIZE GLAD" << endl;
		return -1;
	}

	glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
	glEnable(GL_DEPTH_TEST);

	glfwSetCursorPosCallback(window, mouseCallback);
	glfwSetScrollCallback(window, scrollCallback);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	setWall();	// set wall and floor.
	

	while (!glfwWindowShouldClose(window)) {

		// TODO deltaTime이 계속 다르니까 고정시켜버리면 되는거 아닌가?
		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		processInput(window);

//		TODO update; ball pos, vel, acc update.. collision...
		objManager.update();
//		draw objects. walls, balls... 
		objManager.draw();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();

	return 0;
}


void mouseCallback(GLFWwindow* window, double xpos, double ypos) {

	if (firstMouse) {
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;

	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}
void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
	camera.ProcessMouseScroll(yoffset);
}
void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}
void processInput(GLFWwindow* window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);

	// mouse input
	static int MOUSE_CLICK_OLDSTATE = GLFW_RELEASE;
	int MOUSE_CLICK_NEWSTATE = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
	if (MOUSE_CLICK_NEWSTATE == GLFW_RELEASE && MOUSE_CLICK_OLDSTATE == GLFW_PRESS) 
		makeBall();
	MOUSE_CLICK_OLDSTATE = MOUSE_CLICK_NEWSTATE;

}
void makeBall() {
	float initialVelocity = ((double)rand() / (RAND_MAX))*7.0f + 2.0f;
	float randRadius = ((double)rand() / (RAND_MAX)) * 0.7f + 0.1f;

	vec3 color(0.3, 0.3, 0.3);
	if (randRadius <= 0.8f / 3)
		color += vec3(0.3f, 0, 0);
	else if (randRadius >= 0.8f * 2 / 3)
		color += vec3(0, 0, 0.3f);
	else
		color += vec3(0, 0.3f, 0);

	Ball* ball = new Ball(camera.Position + (2.0f + randRadius)*camera.Front,initialVelocity* camera.Front, vec3(0,-gravity,0),
		582.0f, randRadius, 5,color);
	ball->setupDraw();
	objManager.addThing(ball);
}

void setWall() {		// wall never move... so just set state of wall here...
	float vertices[18];

	const int numVert = 6;

	glm::vec3 vecArr[numVert] = {
		glm::vec3(5, 0, 5),
		glm::vec3(5, 10, -5),
		glm::vec3(5, 0, -5),
		glm::vec3(5, 10, -5),
		glm::vec3(5, 0, 5),
		glm::vec3(5, 10, 5)
	};
	glm::vec3 pos = glm::vec3(5, 5, 0);
	glm::vec3 normal = normalize(cross(vecArr[1] - vecArr[0], vecArr[2] - vecArr[1]));
	for (int j = 0; j < 2; j++) {			// set walls.
		for (int i = 0; i < numVert; i++) {
			vertices[3 * i] = vecArr[i].x;
			vertices[3 * i + 1] = vecArr[i].y;
			vertices[3 * i + 2] = vecArr[i].z;
		}
		Wall* wall = new Wall(pos,normal, vertices,vec3(1.0f*sin(radians(45.0f * j)), 1.0f * cos(radians(45.0f * j)), 0.0f));
		wall->setupDraw();
		objManager.addThing(wall);

		for (int i = 0; i < numVert; i++) {
			vecArr[i] = glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0, 1, 0)) * vec4(vecArr[i],1));
		}
		pos = glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0, 1, 0)) * vec4(pos, 1));
		normal = glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0, 1, 0)) * vec4(normal, 1));
	}
	
	// set floors.
	for (int i = 0; i < numVert; i++) {
		vecArr[i] = glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0, 0, 1))
			* glm::translate(glm::mat4(1.0f), glm::vec3(5.0f, -5.0f, 0))
			* vec4(vecArr[i], 1));
	}
	pos = glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0, 0, 1))
		* glm::translate(glm::mat4(1.0f), glm::vec3(5.0f, -5.0f, 0))
		* vec4(pos, 1));

	normal = vec3(0,1,0);
	for (int i = 0; i < numVert; i++) {
		vertices[3 * i] = vecArr[i].x;
		vertices[3 * i + 1] = vecArr[i].y;
		vertices[3 * i + 2] = vecArr[i].z;
	}
	Wall* wall = new Wall(pos,normal, vertices, vec3(1.0f * sin(radians(45.0f * 2)), 1.0f * cos(radians(45.0f * 2)), 0.0f));
	wall->setupDraw();
	objManager.addThing(wall);

} 
