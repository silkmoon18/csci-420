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
Entity* light;

// controls
vec3 playerAngles(0);
vec3 worldCameraAngles(0);
vec4 moveInput(0); // w, s, a, d
float mouseSensitivity = 5.0f;
float xAngleLimit= 85;
float moveSpeed = 5.0f;
bool isControllingPlayer = true;

// display window
int windowWidth = 1280;
int windowHeight = 720;
char windowTitle[512] = "CSCI 420 homework II";

// images
vector<string> imagePaths;
string imageDirectory;
int currentImageIndex = 0;
int screenshotIndex = 0;


// physics
float rcSpeed = 1.0f;

// other params
bool isTakingScreenshot = false;
int delay = 8; // hard coded for recording on 120 fps monitor
int currentFrame = 0;


// ---assignment-2---

// the spline array 
Spline* splines;
// total number of splines 
int numSplines;

// catmull-rom spline
vector<Entity*> rollerCoasters;
int currentCoasterIndex = -1;




int loadSplines(char* argv) {
	char* cName = (char*)malloc(128 * sizeof(char));
	FILE* fileList;
	FILE* fileSpline;
	int iType, i = 0, j, iLength;

	// load the track file 
	fileList = fopen(argv, "r");
	if (fileList == NULL) {
		printf("can't open file\n");
		exit(1);
	}

	// stores the number of splines in a global variable 
	fscanf(fileList, "%d", &numSplines);

	splines = (Spline*)malloc(numSplines * sizeof(Spline));

	// reads through the spline files 
	for (j = 0; j < numSplines; j++) {
		i = 0;
		fscanf(fileList, "%s", cName);
		fileSpline = fopen(cName, "r");

		if (fileSpline == NULL) {
			printf("can't open file\n");
			exit(1);
		}

		// gets length for spline file
		fscanf(fileSpline, "%d %d", &iLength, &iType);

		// allocate memory for all the points
		splines[j].points = (Point*)malloc(iLength * sizeof(Point));
		splines[j].numControlPoints = iLength;

		// saves the data to the struct
		while (fscanf(fileSpline, "%lf %lf %lf",
					  &splines[j].points[i].x,
					  &splines[j].points[i].y,
					  &splines[j].points[i].z) != EOF) {
			i++;
		}
	}

	free(cName);

	return 0;
}

int initTexture(const char* imageFilename, GLuint textureHandle) {
	// read the texture image
	ImageIO img;
	ImageIO::fileFormatType imgFormat;
	ImageIO::errorType err = img.load(imageFilename, &imgFormat);

	if (err != ImageIO::OK) {
		printf("Loading texture from %s failed.\n", imageFilename);
		return -1;
	}

	// check that the number of bytes is a multiple of 4
	if (img.getWidth() * img.getBytesPerPixel() % 4) {
		printf("Error (%s): The width*numChannels in the loaded image must be a multiple of 4.\n", imageFilename);
		return -1;
	}

	// allocate space for an array of pixels
	int width = img.getWidth();
	int height = img.getHeight();
	unsigned char* pixelsRGBA = new unsigned char[4 * width * height]; // we will use 4 bytes per pixel, i.e., RGBA

	// fill the pixelsRGBA array with the image pixels
	memset(pixelsRGBA, 0, 4 * width * height); // set all bytes to 0
	for (int h = 0; h < height; h++)
		for (int w = 0; w < width; w++) {
			// assign some default byte values (for the case where img.getBytesPerPixel() < 4)
			pixelsRGBA[4 * (h * width + w) + 0] = 0; // red
			pixelsRGBA[4 * (h * width + w) + 1] = 0; // green
			pixelsRGBA[4 * (h * width + w) + 2] = 0; // blue
			pixelsRGBA[4 * (h * width + w) + 3] = 255; // alpha channel; fully opaque

			// set the RGBA channels, based on the loaded image
			int numChannels = img.getBytesPerPixel();
			for (int c = 0; c < numChannels; c++) // only set as many channels as are available in the loaded image; the rest get the default value
				pixelsRGBA[4 * (h * width + w) + c] = img.getPixel(w, h, c);
		}

	// bind the texture
	glBindTexture(GL_TEXTURE_2D, textureHandle);

	// initialize the texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelsRGBA);

	// generate the mipmaps for this texture
	glGenerateMipmap(GL_TEXTURE_2D);

	// set the texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// query support for anisotropic texture filtering
	GLfloat fLargest;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
	printf("Max available anisotropic samples: %f\n", fLargest);
	// set anisotropic texture filtering
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 0.5f * fLargest);

	// query for any errors
	GLenum errCode = glGetError();
	if (errCode != 0) {
		printf("Texture initialization error. Error code: %d.\n", errCode);
		return -1;
	}

	// de-allocate the pixel array -- it is no longer needed
	delete[] pixelsRGBA;

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
	//quat pitch = angleAxis(radians(angles->x), worldRight);
	//quat yaw = angleAxis(radians(angles->y), worldUp);
	//player->transform->setRotation(yaw * pitch, true);

	target->transform->rotateAround(y, worldUp, true);
	target->transform->rotateAround(x, worldRight, false);
}
void createSplineObjects() {
	for (int i = 0; i < numSplines; i++) {
		Spline spline = splines[i];

		Entity* coaster = SceneManager::getInstance()->createEntity("RollerCoaster_" + to_string(rollerCoasters.size()));
		coaster->addComponent(new Renderer(new VertexArrayObject(milestonePipeline)));
		coaster->addComponent(new RollerCoaster(spline));
		coaster->getComponent<RollerCoaster>()->render();
		rollerCoasters.push_back(coaster);
	}

	if (rollerCoasters.size() > 0) {
		currentCoasterIndex = 0;
	}
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

void HandleMoveInput() {
	float step = moveSpeed * Timer::getInstance()->getDeltaTime();
	if (isControllingPlayer) {
		player->getComponent<PlayerController>()->moveOnGround(moveInput, step);
	}
	else {
		float z = moveInput.x - moveInput.y;
		float x = moveInput.w - moveInput.z;
		if (z == 0 && x == 0) return;

		vec3 move = normalize(worldCamera->transform->getForwardVector(false) * z + worldCamera->transform->getRightVector(false) * x) * step;
		vec3 position = worldCamera->transform->getPosition(false) + move;
		worldCamera->transform->setPosition(position, false);
	}
}
Entity* test;
vec3 r(0);
void displayFunc() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	HandleMoveInput();

	// update entities
	SceneManager::getInstance()->update();

	glutSwapBuffers();
}

void HandleJump() {
	Physics* physics = player->getComponent<Physics>();
	if (physics->isOnGround) {
		physics->velocity.y = 5;
	}
}
void keyboardFunc(unsigned char key, int x, int y) {
	switch (key) {
		case 27: // ESC key
			exit(0); // exit the program
			break;

		case ' ':
			if (!isControllingPlayer) return;
			HandleJump();
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
			// toggle screenshots recording
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
	milestonePipeline = SceneManager::getInstance()->createPipelineProgram(shaderBasePath);

	//texturePipeline = new BasicPipelineProgram;
	//int ret = texturePipeline->Init(shaderBasePath);
	//if (ret != 0) abort();

	cout << "\nGL error: " << glGetError() << endl;
}

void initObjects() {
	worldCamera = SceneManager::getInstance()->createEntity("WorldCamera");
	worldCamera->transform->setPosition(vec3(20, 20, 20), true);
	worldCamera->addComponent(new Camera());
	worldCamera->transform->faceTo(vec3(0));
	worldCameraAngles = worldCamera->transform->getEulerAngles(true);

	player = SceneManager::getInstance()->createEntity("Player");
	player->addComponent(new Camera());
	player->getComponent<Camera>()->setCurrent();
	player->addComponent(new PlayerController());
	player->addComponent(new Physics(0.75f));
	player->transform->setPosition(vec3(0, 0, 5), true);
	playerAngles = player->transform->getEulerAngles(true);

	ground = SceneManager::getInstance()->createEntity("Ground");
	ground->addComponent(new Renderer(milestonePipeline, Renderer::Shape::Cube));
	ground->transform->setPosition(vec3(0, -1, 0), true);
	ground->transform->setScale(vec3(100, 1, 100), true);

	light = SceneManager::getInstance()->createEntity("Light");
	light->addComponent(new Light());
	light->transform->setPosition(vec3(0, 3, 0), true);

	//auto* light2 = SceneManager::getInstance()->createEntity("Light");
	//light2->addComponent(new Light());
	//light2->transform->setPosition(vec3(0, 3, 50), true);

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


