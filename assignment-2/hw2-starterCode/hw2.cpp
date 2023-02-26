/*
  CSCI 420 Computer Graphics, USC
  Assignment 2: Roller Coaster

  Student username: Baihua Yang
*/

#include "basicPipelineProgram.h"
//#include "openGLMatrix.h"
#include "imageIO.h"
#include "openGLHeader.h"
#include "glutHeader.h"
#include "Utility.h"

#include <iostream>
#include <cstring>
#include <vector>
#include <filesystem>

#if defined(WIN32) || defined(_WIN32)
#ifdef _DEBUG
#pragma comment(lib, "glew32d.lib")
#else
#pragma comment(lib, "glew32.lib")
#endif
#endif

#if defined(WIN32) || defined(_WIN32)
char shaderBasePath[1024] = SHADER_BASE_PATH;
#else
char shaderBasePath[1024] = "../openGLHelper-starterCode";
#endif

using namespace std;
using namespace glm;





BasicPipelineProgram* milestonePipeline;
BasicPipelineProgram* texturePipeline;
BasicPipelineProgram* skyboxPipeline;


int mousePos[2]; // x,y coordinate of the mouse position

int leftMouseButton = 0; // 1 if pressed, 0 if not 
int middleMouseButton = 0; // 1 if pressed, 0 if not
int rightMouseButton = 0; // 1 if pressed, 0 if not

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROL_STATE;
CONTROL_STATE controlState = ROTATE;

// entities
Entity* worldCamera;
Entity* player;
Entity* ground;
Entity* sky;
Entity* light;

Entity* planetModel;
Entity* modelBase;
Entity* planet0;
Entity* planet1;
Entity* planet2;
Entity* planet3;

// controls
vec3 playerAngles(0);
vec3 worldCameraAngles(0);
vec4 moveInput(0); // w, s, a, d
float verticalMove = 0.0f;
float mouseSensitivity = 5.0f;
float xAngleLimit = 85;
bool isControllingPlayer = true;

// display window
int windowWidth = 1280;
int windowHeight = 720;
char windowTitle[512] = "CSCI 420 homework II";

// images
string shaderDirectory = "/shaders";
string textureDirectory = "/textures";
int currentImageIndex = 0;
int screenshotIndex = 0;


// other params
bool isTakingScreenshot = false;
int delay = 8; // hard coded for recording on 120 fps monitor
int currentFrame = 0;


// ---assignment-2---

// the spline array 
vector<Spline> splines;
// total number of splines 
int numSplines;

// catmull-rom spline
vector<Entity*> rollerCoasters;
int currentCoasterIndex = -1;
float activatableDistance = 5.0f;




int loadSplines(char* argv) {
	char* fileName = (char*)malloc(128 * sizeof(char));;
	FILE* fileList;
	FILE* fileSpline;

	// load the track file 
	fileList = fopen(argv, "r");
	if (fileList == NULL) {
		printf("can't open file\n");
		exit(1);
	}

	// reads through the spline files 
	while (fscanf(fileList, "%s", fileName) != EOF) {
		Spline spline;
		fileSpline = fopen(fileName, "r");

		if (fileSpline == NULL) {
			printf("can't open file\n");
			exit(1);
		}

		float x, y, z;
		// saves the data to the struct
		while (fscanf(fileSpline, "%f %f %f",
					  &x,
					  &y,
					  &z) != EOF) {
			spline.points.push_back(vec3(x, y, z));
		}
		spline.numControlPoints = spline.points.size();
		splines.push_back(spline);
	}
	free(fileName);

	return 0;
}

// write a screenshot to the specified filename
void saveScreenshot() {
	if (screenshotIndex > 999) {
		cout << "Num of screenshots has reached limit 999. " << endl;
		return;
	}

	int digit = to_string(screenshotIndex).length();
	string filename = "./screenshots/";
	for (int i = digit; i < 3; i++) {
		filename += "0";
	}
	filename += to_string(screenshotIndex + 1) + ".jpg";

	unsigned char* screenshotData = new unsigned char[windowWidth * windowHeight * 3];
	glReadPixels(0, 0, windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, screenshotData);

	ImageIO screenshotImg(windowWidth, windowHeight, 3, screenshotData);

	if (screenshotImg.save(filename.c_str(), ImageIO::FORMAT_JPEG) == ImageIO::OK) {
		cout << "File " << filename << " saved successfully." << endl;
		screenshotIndex++;
	}
	else cout << "Failed to save file " << filename << '.' << endl;

	delete[] screenshotData;
}

void HandleMouseInput(int mousePosDelta[2]) {
	float lookStep = mouseSensitivity * Timer::getInstance()->getDeltaTime();
	Entity* target;
	vec3* angles;
	if (isControllingPlayer) {
		target = player;
		angles = &playerAngles;

	}
	else {
		target = worldCamera;
		angles = &worldCameraAngles;
	}

	float x = -mousePosDelta[1] * lookStep;
	float y = -mousePosDelta[0] * lookStep;
	angles->x += x;
	angles->y += y;
	if (angles->x > xAngleLimit) {
		x -= angles->x - xAngleLimit;
		angles->x = xAngleLimit;
	}
	else if (angles->x < -xAngleLimit) {
		x -= angles->x + xAngleLimit;
		angles->x = -xAngleLimit;
	}
	target->transform->rotateAround(y, worldUp, true);
	target->transform->rotateAround(x, worldRight, false);
}
void HandleMoveInput() {
	if (isControllingPlayer) {
		player->getComponent<PlayerController>()->moveOnGround(moveInput);
	}
	else {
		worldCamera->getComponent<PlayerController>()->move(moveInput, verticalMove);
	}
}
void HandleJump() {
	if (isControllingPlayer) {
		Physics* physics = player->getComponent<Physics>();
		if (physics->isOnGround) {
			physics->velocity.y = 5;
		}
	}
	else {
		worldCamera->getComponent<PlayerController>()->move(moveInput, verticalMove);
	}
}

void createSplineObjects() {
	Entity* coaster = SceneManager::getInstance()->createEntity("RollerCoaster");
	coaster->addComponent(new Renderer(new VertexArrayObject(milestonePipeline), GL_TRIANGLES));
	coaster->addComponent(new RollerCoaster(splines, true));
	coaster->getComponent<RollerCoaster>()->render(vec3(0, 1, 0));
	coaster->transform->setPosition(vec3(0, 0, -50), true);

	rollerCoasters.push_back(coaster);
}

void rideCoaster(RollerCoaster* coaster) {
	player->getComponent<Physics>()->setActive(false);
	player->getComponent<PlayerController>()->setActive(false);
	player->setParent(coaster->seat);
	player->transform->setPosition(vec3(0, 0, 0), false);
}
void unrideCoaster() {
	player->getComponent<Physics>()->setActive(true);
	player->getComponent<PlayerController>()->setActive(true);
	player->setParent(nullptr);

	RollerCoaster* coaster = rollerCoasters[currentCoasterIndex]->getComponent<RollerCoaster>();
	player->transform->setPosition(coaster->getStartPosition(), true);
	coaster->reset();
	currentCoasterIndex = -1;
}
void TryActivateNearestRollerCoaster() {
	float minDistance = INT_MAX;
	int nearest = 0;
	for (int i = 0; i < rollerCoasters.size(); i++) {
		float d = distance(rollerCoasters[i]->getComponent<RollerCoaster>()->spline.points[0], player->transform->getPosition(true));
		if (d < minDistance) {
			minDistance = d;
			nearest = i;
		}
	}
	RollerCoaster* coaster = rollerCoasters[nearest]->getComponent<RollerCoaster>();
	if (minDistance <= activatableDistance) {
		rideCoaster(coaster);
		coaster->start();
		currentCoasterIndex = nearest;
		printf("Roller coaster No.%i activated. \n", nearest + 1);
	}
}

void updatePlanetModel() {
	vec3 pivot = planet0->transform->getPosition(true);
	planet1->transform->rotateAround(pivot, 0.1, vec3(0, 1, -1));
	planet2->transform->rotateAround(pivot, -0.15, vec3(0, 1, 3));
	planet3->transform->rotateAround(pivot, -0.3, vec3(0, 1, 0));
}

void idleFunc() {
	// calculate delta time
	Timer::getInstance()->setCurrentTime(glutGet(GLUT_ELAPSED_TIME));

	// save 15 screenshots per second
	if (isTakingScreenshot && currentFrame % 8 == 0) {
		saveScreenshot();
	}

	// make the screen update 
	glutPostRedisplay();
	currentFrame++;
}

void displayFunc() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	HandleMoveInput();

	updatePlanetModel();
	// update entities
	SceneManager::getInstance()->update();

	glutSwapBuffers();
}

void keyboardFunc(unsigned char key, int x, int y) {
	switch (key) {
		case 27: // ESC key
			exit(0); // exit the program
			break;
		case ' ':
			if (isControllingPlayer) HandleJump();
			else verticalMove = 1;
			break;
		case 'w':
			moveInput.x = 1;
			break;
		case 's':
			moveInput.y = 1;
			break;
		case 'a':
			moveInput.z = 1;
			break;
		case 'd':
			moveInput.w = 1;
			break;
		case 'c':
			verticalMove = -1;
			break;
			// toggle screenshots recording
		case 'e':
			if (currentCoasterIndex == -1) {
				TryActivateNearestRollerCoaster();
			}
			else {
				unrideCoaster();
			}
			break;
		case 'x':
			isTakingScreenshot = !isTakingScreenshot;
			break;
		case 'p':
			if (isControllingPlayer) {
				worldCamera->getComponent<Camera>()->setCurrent();
				isControllingPlayer = false;
			}
			else {
				player->getComponent<Camera>()->setCurrent();
				isControllingPlayer = true;
			}
			break;
	}
}

void keyboardUpFunc(unsigned char key, int x, int y) {
	switch (key) {
		case 'w':
			moveInput.x = 0;
			break;
		case 's':
			moveInput.y = 0;
			break;
		case 'a':
			moveInput.z = 0;
			break;
		case 'd':
			moveInput.w = 0;
			break;
		case ' ':
			verticalMove = 0;
			break;
		case 'c':
			verticalMove = 0;
			break;
	}
}

void mouseButtonFunc(int button, int state, int x, int y) {
	// a mouse button has has been pressed or depressed

	// keep track of the mouse button state, in leftMouseButton, middleMouseButton, rightMouseButton variables
	switch (button) {
		case GLUT_LEFT_BUTTON:
			leftMouseButton = (state == GLUT_DOWN);
			break;

		case GLUT_MIDDLE_BUTTON:
			middleMouseButton = (state == GLUT_DOWN);
			break;

		case GLUT_RIGHT_BUTTON:
			rightMouseButton = (state == GLUT_DOWN);
			break;
	}

	// keep track of whether CTRL and SHIFT keys are pressed
	//switch (glutGetModifiers()) {
	//	case GLUT_ACTIVE_CTRL:
	//		controlState = TRANSLATE;
	//		break;

	//	case GLUT_ACTIVE_SHIFT:
	//		controlState = SCALE;
	//		break;

	//		// if CTRL and SHIFT are not pressed, we are in rotate mode
	//	default:
	//		controlState = ROTATE;
	//		break;
	//}

	// store the new mouse position
	mousePos[0] = x;
	mousePos[1] = y;
}

void mouseMotionDragFunc(int x, int y) {
	// mouse has moved and one of the mouse buttons is pressed (dragging)
	int mousePosDelta[2] = { x - mousePos[0], y - mousePos[1] };
	HandleMouseInput(mousePosDelta);

	//switch (controlState) {
	//	// translate the landscape
	//	case TRANSLATE:
	//		if (leftMouseButton) {
	//			// control x,y translation via the left mouse button
	//			landTranslate[0] += mousePosDelta[0] * translateSpeed;
	//			landTranslate[1] -= mousePosDelta[1] * translateSpeed;
	//		}
	//		if (middleMouseButton) {
	//			// control z translation via the middle mouse button
	//			landTranslate[2] += mousePosDelta[1] * translateSpeed;
	//		}
	//		break;

	//		// rotate the landscape
	//	case ROTATE:
	//		if (leftMouseButton) {
	//			// control x,y rotation via the left mouse button
	//			landRotate[0] += mousePosDelta[1] * rotateSpeed;
	//			landRotate[1] += mousePosDelta[0] * rotateSpeed;
	//		}
	//		if (middleMouseButton) {
	//			// control z rotation via the middle mouse button
	//			landRotate[2] += mousePosDelta[1] * rotateSpeed;
	//		}
	//		break;

	//		// scale the landscape
	//	case SCALE:
	//		if (leftMouseButton) {
	//			// control x,y scaling via the left mouse button
	//			landScale[0] *= 1.0f + mousePosDelta[0] * scaleSpeed;
	//			landScale[1] *= 1.0f - mousePosDelta[1] * scaleSpeed;
	//		}
	//		if (middleMouseButton) {
	//			// control z scaling via the middle mouse button
	//			landScale[2] *= 1.0f - mousePosDelta[1] * scaleSpeed;
	//		}
	//		break;
	//}

	// store the new mouse position
	mousePos[0] = x;
	mousePos[1] = y;
}

void mouseMotionFunc(int x, int y) {
	// mouse has moved
	// store the new mouse position
	mousePos[0] = x;
	mousePos[1] = y;
}

void reshapeFunc(int w, int h) {
	glViewport(0, 0, w, h);

	Camera::currentCamera->setPerspective(Camera::currentCamera->fieldOfView,
										  (float)w / (float)h,
										  Camera::currentCamera->zNear,
										  Camera::currentCamera->zFar);
}

void initScene() {
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glEnable(GL_DEPTH_TEST);

	// polygon offset
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(1, 1);

	// restart index
	glEnable(GL_PRIMITIVE_RESTART);
	glPrimitiveRestartIndex(RESTARTINDEX);

	glEnable(GL_NORMALIZE);

	// pipeline
	milestonePipeline = SceneManager::getInstance()->createPipelineProgram(shaderDirectory + "/milestone");
	texturePipeline = SceneManager::getInstance()->createPipelineProgram(shaderDirectory + "/texture");
	skyboxPipeline = SceneManager::getInstance()->createPipelineProgram(shaderDirectory + "/skybox");

	cout << "\nGL error: " << glGetError() << endl;
}

void initPlanetModel() {
	planetModel = SceneManager::getInstance()->createEntity("PlanetModel");
	planetModel->transform->setPosition(vec3(0, 0, 0), true);

	modelBase = SceneManager::getInstance()->createEntity("ModelBase");
	modelBase->addComponent(new Renderer(milestonePipeline, makeTetrahedron(5, 4)));
	modelBase->setParent(planetModel);

	planet0 = SceneManager::getInstance()->createEntity("Planet0");
	planet0->transform->setPosition(vec3(0, 50, 0), false);
	planet0->addComponent(new Renderer(milestonePipeline, makeSphere(10)));
	planet0->setParent(modelBase);

	planet1 = SceneManager::getInstance()->createEntity("Planet1");
	planet1->transform->setPosition(vec3(15, 0, 0), false);
	planet1->addComponent(new Renderer(milestonePipeline, makeSphere(3)));
	planet1->setParent(planet0);

	planet2 = SceneManager::getInstance()->createEntity("Planet2");
	planet2->transform->setPosition(vec3(20, 0, 0), false);
	planet2->addComponent(new Renderer(milestonePipeline, makeSphere(4)));
	planet2->setParent(planet0);

	planet3 = SceneManager::getInstance()->createEntity("Planet3");
	planet3->transform->setPosition(vec3(30, 0, 0), false);
	planet3->addComponent(new Renderer(milestonePipeline, makeSphere(1.5)));
	planet3->setParent(planet0);
}
void initObjects() {
	worldCamera = SceneManager::getInstance()->createEntity("WorldCamera");
	worldCamera->transform->setPosition(vec3(20, 20, 20), true);
	worldCamera->addComponent(new Camera());
	worldCamera->addComponent(new PlayerController());
	worldCamera->transform->faceTo(vec3(0));
	worldCameraAngles = worldCamera->transform->getEulerAngles(true);

	player = SceneManager::getInstance()->createEntity("Player");
	player->transform->setPosition(vec3(0, 0, 100), true);
	player->addComponent(new Camera());
	player->getComponent<Camera>()->setCurrent();
	player->addComponent(new PlayerController());
	player->addComponent(new Physics(true, 1.75f));
	playerAngles = player->transform->getEulerAngles(true);

	ground = SceneManager::getInstance()->createEntity("Ground");
	Renderer* groundRenderer = new Renderer(texturePipeline, makePlane(2000, 2000));
	groundRenderer->setTexture(textureDirectory + "/ground.jpg");
	ground->addComponent(groundRenderer);
	ground->transform->setPosition(vec3(0, 0, 0), true);

	string skyboxImages[6] = {
		textureDirectory + "/right.jpg",
		textureDirectory + "/left.jpg",
		textureDirectory + "/bottom.jpg",
		textureDirectory + "/top.jpg",
		textureDirectory + "/front.jpg",
		textureDirectory + "/back.jpg", };
	sky = SceneManager::getInstance()->createSkybox(skyboxPipeline, skyboxImages);

	light = SceneManager::getInstance()->createEntity("Light");
	Light* directionalLight = new Light();
	directionalLight->setDirectional();
	light->addComponent(directionalLight);
	light->transform->setPosition(vec3(0, 3, 0), true);

	//auto* light2 = SceneManager::getInstance()->createEntity("Light");
	//light2->addComponent(new Light());
	//light2->transform->setPosition(vec3(0, 3, 50), true);

	//Entity* test = SceneManager::getInstance()->createEntity("test");
	//test->transform->setPosition(vec3(0, 0, 5), true);
	//Renderer* testRenderer = new Renderer(milestonePipeline, makeTetrahedron(10, 10 * 3 / 4.0));
	//test->addComponent(testRenderer);


	SceneManager::getInstance()->isLightingEnabled = true;

	initPlanetModel();
	createSplineObjects();
}

int main(int argc, char* argv[]) {
	if (argc < 2) {
		printf("usage: %s <trackfile>\n", argv[0]);
		exit(0);
	}

	// load the splines from the provided filename
	loadSplines(argv[1]);

	printf("Loaded %d spline(s).\n", numSplines);
	for (int i = 0; i < numSplines; i++)
		printf("Num control points in spline %d: %d.\n", i, splines[i].numControlPoints);


	cout << "Initializing GLUT..." << endl;
	glutInit(&argc, argv);

	cout << "Initializing OpenGL..." << endl;

#ifdef __APPLE__
	glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
#else
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
#endif

	glutInitWindowSize(windowWidth, windowHeight);
	glutInitWindowPosition(0, 0);
	glutCreateWindow(windowTitle);

	cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
	cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << endl;
	cout << "Shading Language Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

#ifdef __APPLE__
	// This is needed on recent Mac OS X versions to correctly display the window.
	glutReshapeWindow(windowWidth - 1, windowHeight - 1);
#endif

	// perform animation inside idleFunc
	glutIdleFunc(idleFunc);

	// tells glut to use a particular display function to redraw 
	glutDisplayFunc(displayFunc);

	// callback for pressing the keys on the keyboard
	glutKeyboardFunc(keyboardFunc);
	glutKeyboardUpFunc(keyboardUpFunc);

	// callback for mouse button changes
	glutMouseFunc(mouseButtonFunc);

	// callback for mouse drags
	glutMotionFunc(mouseMotionDragFunc);
	// callback for idle mouse movement
	glutPassiveMotionFunc(mouseMotionFunc);

	// callback for resizing the window
	glutReshapeFunc(reshapeFunc);

	// init glew
#ifdef __APPLE__
  // nothing is needed on Apple
#else
  // Windows, Linux
	GLint result = glewInit();
	if (result != GLEW_OK) {
		cout << "error: " << glewGetErrorString(result) << endl;
		exit(EXIT_FAILURE);
}
#endif

	// do initialization
	initScene();
	initObjects();

	// sink forever into the glut loop
	glutMainLoop();
}


