/*
  CSCI 420 Computer Graphics, USC
  Assignment 2: Roller Coaster

  Student username: Baihua Yang
*/

#include "basicPipelineProgram.h"
#include "openGLMatrix.h"
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
Entity* mainCamera;
Entity* player;
Entity* worldEye;
Entity* ground;

// controls
vec3 playerAngles(0);
vec3 worldCameraAngles(0);
vec4 moveInput(0); // w, s, a, d
float mouseSensitivity = 5.0f;
float xAngleLimit= 85;
float moveSpeed = 5.0f;
bool isControllingPlayer = true;
//float translateSpeed = 0.1f;
//float rotateSpeed = 0.5f;
//float scaleSpeed = 0.01f;


// lighting 
vec4 lightPosition(0, 0, 0, 1);
vec4 lightAmbient(1, 1, 1, 1);
vec4 lightDiffuse(.8, .8, .8, 1);
vec4 lightSpecular(1, 1, 1, 1);
vec4 ambientCoef(0.5, 0.5, 0.5, 1);
vec4 diffuseCoef(1, 1, 1, 1);
vec4 specularCoef(.9, .9, .9, 1);
float materialShininess = 1;

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

void setUniforms() {
	// set lightings
	GLuint loc = glGetUniformLocation(milestonePipeline->GetProgramHandle(), "lightPosition");
	glUniform4f(loc, lightPosition[0], lightPosition[1], lightPosition[2], lightPosition[3]);

	loc = glGetUniformLocation(milestonePipeline->GetProgramHandle(), "ambientCoef");
	glUniform4f(loc, ambientCoef[0], ambientCoef[1], ambientCoef[2], ambientCoef[3]);
	loc = glGetUniformLocation(milestonePipeline->GetProgramHandle(), "diffuseCoef");
	glUniform4f(loc, diffuseCoef[0], diffuseCoef[1], diffuseCoef[2], diffuseCoef[3]);
	loc = glGetUniformLocation(milestonePipeline->GetProgramHandle(), "specularCoef");
	glUniform4f(loc, specularCoef[0], specularCoef[1], specularCoef[2], specularCoef[3]);
	loc = glGetUniformLocation(milestonePipeline->GetProgramHandle(), "materialShininess");
	glUniform1f(loc, materialShininess);

	loc = glGetUniformLocation(milestonePipeline->GetProgramHandle(), "lightAmbient");
	glUniform4f(loc, lightAmbient[0], lightAmbient[1], lightAmbient[2], lightAmbient[3]);
	loc = glGetUniformLocation(milestonePipeline->GetProgramHandle(), "lightDiffuse");
	glUniform4f(loc, lightDiffuse[0], lightDiffuse[1], lightDiffuse[2], lightDiffuse[3]);
	loc = glGetUniformLocation(milestonePipeline->GetProgramHandle(), "lightSpecular");
	glUniform4f(loc, lightSpecular[0], lightSpecular[1], lightSpecular[2], lightSpecular[3]);
}




void HandleMouseInput(int mousePosDelta[2]) {
	float lookStep = mouseSensitivity * Timer::getInstance()->getDeltaTime();
	Entity* target;
	Entity* camera;
	vec3* angles;
	if (isControllingPlayer) {
		target = player;
		camera = mainCamera;
		angles = &playerAngles;

	}
	else {
		target = worldEye;
		camera = worldCamera;
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

	player->transform->rotateAround(y, worldUp, true);
	player->transform->rotateAround(x, worldRight, false);
}
void createSplineObjects() {
	for (int i = 0; i < numSplines; i++) {
		Spline spline = splines[i];

		Entity* coaster = EntityManager::getInstance()->createEntity("RollerCoaster_" + rollerCoasters.size());
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

		vec3 move = normalize(worldCamera->getForwardVector(false) * z + worldCamera->getRightVector(false) * x) * step;
		vec3 position = worldCamera->transform->getPosition(false) + move;
		worldCamera->transform->setPosition(position, false);
	}
}
Entity* test;
vec3 r(0);
void displayFunc() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// set uniforms
	setUniforms();

	HandleMoveInput();

	// update entities
	EntityManager::getInstance()->update();

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
				mainCamera->getComponent<Camera>()->setCurrent();
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

	// pipeline
	milestonePipeline = new BasicPipelineProgram;
	int ret = milestonePipeline->Init(shaderBasePath);
	if (ret != 0) abort();

	//texturePipeline = new BasicPipelineProgram;
	//int ret = texturePipeline->Init(shaderBasePath);
	//if (ret != 0) abort();

	cout << "\nGL error: " << glGetError() << endl;
}

void initObjects() {
	worldEye = EntityManager::getInstance()->createEntity("WorldEye");
	worldEye->transform->setPosition(vec3(20, 20, 20), true);

	worldCamera = EntityManager::getInstance()->createEntity("WorldCamera");
	worldCamera->addComponent(new Camera());
	worldCameraAngles = worldCamera->transform->getEulerAngles(true);
	worldCamera->setParent(worldEye);

	player = EntityManager::getInstance()->createEntity("Player");
	player->addComponent(new PlayerController());
	player->addComponent(new Physics(0.75f));
	player->transform->setPosition(vec3(0, 0, 5), true);
	playerAngles = player->transform->getEulerAngles(true);

	mainCamera = EntityManager::getInstance()->createEntity("MainCamera");
	mainCamera->addComponent(new Camera());
	mainCamera->getComponent<Camera>()->setCurrent();
	mainCamera->setParent(player);

	ground = EntityManager::getInstance()->createEntity("Ground");
	ground->addComponent(new Renderer(milestonePipeline, Renderer::Shape::Cube));
	ground->transform->setPosition(vec3(0, -0.5, 0), true);
	ground->transform->setScale(vec3(1000, 1, 1000), true);



	//debug 
	ground->transform->setPosition(vec3(0, 1, 0), true);
	ground->transform->setEulerAngles(vec3(30, 30, 30), true);
	ground->transform->setScale(vec3(1, 1, 1), true);

	player->getComponent<Physics>()->setActive(false);

	cout << "---debug---" << endl;
    test = EntityManager::getInstance()->createEntity("Test");
	test->addComponent(new Renderer(milestonePipeline, Renderer::Shape::Cube, vec4(240, 84, 79, 255)));

	test->setParent(ground);
	test->transform->setPosition(vec3(3, 0, 0), true);
	//test->transform->setEulerAngles(vec3(0, 90, 30), false);
	//test->transform->rotateAround(0, worldRight, false);
	//test->transform->rotateAround(90, worldUp, false);
	//test->transform->rotateAround(30, vec3(0, 0, 1), false);
	//log(test->transform->getRotation(false));

	//quat pitch = angleAxis(radians(0.f), vec3(1, 0, 0));
	//quat yaw = angleAxis(radians(90.f), vec3(0, 1, 0));
	//quat roll = angleAxis(radians(30.f), vec3(0, 0, 1));
	//test->transform->setRotation(pitch * yaw * roll, false);
	//log(pitch * yaw * roll);

	//log(test->transform->getEulerAngles(false));
	//log(degrees(eulerAngles(test->transform->getRotation(false))));
	//log(test->transform->getEulerAngles(true));
	//log(degrees(eulerAngles(test->transform->getRotation(true))));

	//log(mainCamera->transform->getRotation(true));


	//test->transform->setPosition(vec3(0, 2, 10), false);
	//test->transform->setRotation(angleAxis(radians(45.f), vec3(0, 1, 0)), false);
	//test->transform->setScale(vec3(2, 3, 4), false);
	//player->setParent(test);

	//player->transform->setPosition(vec3(2, 2, 2), true);
	//player->transform->setRotation(angleAxis(radians(30.f), vec3(0, 1, 0)), false);
	//player->transform->setScale(vec3(5, 6, 7), true);

	//vec3 angles = vec3(90, 90, 90);
	//angles = radians(angles);
	//mat4 mat(1);
	//mat *= rotate(angles.x, vec3(1, 0, 0));
	//mat *= rotate(angles.y, vec3(0, 1, 0));
	//mat *= rotate(angles.z, vec3(0, 0, 1));
	//quat q = quat_cast(mat);
	//log(degrees(eulerAngles(q)));
	//log(player->transform->getPosition(true));
	//log(player->transform->getPosition(false));	
	//quat pitch = angleAxis(radians(90.f), vec3(1, 0, 0));
	//quat yaw = angleAxis(radians(90.f), vec3(0, 1, 0));
	//player->transform->setRotation(yaw * pitch, true);
	//log(degrees(eulerAngles(player->transform->getRotation(true))));
	//log(degrees(eulerAngles(player->transform->getRotation(false))));
	//player->transform->setRotation(pitch * yaw, true);
	//log(degrees(eulerAngles(player->transform->getRotation(true))));
	//log(degrees(eulerAngles(player->transform->getRotation(false))));
	//log(player->transform->getScale(true));
	//log(player->transform->getScale(false));

	//mat4 mat(1);
	//quat r = angleAxis(radians(45.f), vec3(0, 1, 0));
	//mat *= mat4_cast(r);
	//quat q = quat_cast(mat);
	//log(degrees(eulerAngles(q)));

	//player->faceTo(vec3(0));
	//player->transform->position = vec3(0, 0, -3);
	//player->faceTo(test->transform->position, worldUp);
	//playerAngles = player->transform->getEulerAngles(false);
	//test->setParent(ground);
	//ground->transform->position = vec3(1, 1, 1);
	//ground->transform->scale = vec3(1, 1, 1);
	//ground->transform->setEulerAngles(vec3(0, 30, 0));
	//test->transform->setEulerAngles(vec3(0, 0, -30));

	//EntityManager::getInstance()->update();
	//cout << "world: ";
	//log(test->transform->getEulerAngles(true));
	//cout << "local: ";
	//log(test->transform->getEulerAngles(false));

	//test = EntityManager::getInstance()->createEntity();
	//OpenGLMatrix matrix;
	//matrix.LoadIdentity();
	//matrix.Rotate(45, 1, 0, 0);
	//matrix.Rotate(45, 0, 1, 0);
	//matrix.Rotate(45, 0, 0, 1);
	//float m[16];
	//matrix.GetMatrix(m);
	//mat4 mat = make_mat4(m);
	//float x, y, z;
	//extractEulerAngleXYZ(mat, x, y, z);
	//log(degrees(vec3(x, y, z)));

	//quat q = quat_cast(mat);
	//log(q);
	//log(degrees(eulerAngles(q)));

	//matrix.LoadIdentity();
	//matrix.Rotate(degrees(x), 1, 0, 0);
	//matrix.Rotate(degrees(y), 0, 1, 0);
	//matrix.Rotate(degrees(z), 0, 0, 1);
	//matrix.GetMatrix(m);
	//mat = make_mat4(m);
	//extractEulerAngleXYZ(mat, x, y, z);
	//log(degrees(vec3(x, y, z)));

	//q = quat_cast(mat);
	//log(q);
	//log(degrees(eulerAngles(q)));
	//extractEulerAngleXYZ(mat, x, y, z);
	//q = quat(vec3(x, y, z));
	//matrix.LoadIdentity();
	//matrix.Rotate(degrees(x), 1, 0, 0);
	//matrix.Rotate(degrees(y), 0, 1, 0);
	//matrix.Rotate(degrees(z), 0, 0, 1);
	//matrix.GetMatrix(m);
	//mat = make_mat4(m);
	//extractEulerAngleXYZ(mat, x, y, z);
	//log(degrees(vec3(x, y, z)));

	//q = toQuat(transpose(orientate3(vec3(x, y, z))));
	//log((((q))));

	//mat = toMat4(q);
	//extractEulerAngleXYZ(mat, x, y, z);
	//log(degrees(vec3(x, y, z)));


	//quat qx = angleAxis(x, vec3(1, 0, 0));
	//quat qy = angleAxis(y, vec3(0, 1, 0));
	//quat qz = angleAxis(z, vec3(0, 0, 1));
	//q = qz * qx * qy;
	////q = quat(1, 0, 0, 0);
	////q = rotate(q, x, vec3(1, 0, 0));
	////q = rotate(q, y, vec3(0, 1, 0));
	////q = rotate(q, z, vec3(0, 0, 1));
	////q = toQuat(orientate3(vec3(x, y, z)));
	//log(q);
	//log(degrees(eulerAngles(q)));

	//mat4 mat(1);
	//mat *= rotate(radians(90.0f), vec3(0, 0, 1));
	//mat *= rotate(radians(45.0f), vec3(1, 0, 0));
	//mat *= rotate(radians(90.0f), vec3(0, 1, 0));
	//quat q = toQuat((mat));
	//float w = sqrt(1.0 + mat[0][0] + mat[1][1] + mat[2][2]) / 2.0;
	//double w4 = (4.0 * w);
	//float x = (mat[2][1] - mat[1][2]) / w4;
	//float y = (mat[0][2] - mat[2][0]) / w4;
	//float z = (mat[1][0] - mat[0][1]) / w4;
	//log(degrees(eulerAngles(quat(w, x, y, z))));
	//log(degrees(eulerAngles((q))));
	//glm::mat4 transformation = mat; // your transformation matrix.
	//glm::vec3 scale;
	//glm::quat rotation;
	//glm::vec3 translation;
	//glm::vec3 skew;
	//glm::vec4 perspective;
	//glm::decompose(transformation, scale, rotation, translation, skew, perspective);
	//log(degrees(eulerAngles(conjugate(rotation))));

	createSplineObjects();

	RollerCoaster* coaster = rollerCoasters[currentCoasterIndex]->getComponent<RollerCoaster>();
	//player->faceTo(vec3(10, 0, 0));
	//coaster->start();
	//player->setParent(coaster->seat);
	//player->transform->position = vec3(0, 0, 0);
	//Entity* test2 = EntityManager::getInstance()->createEntity();
	//test2->transform->position = player->transform->position;
	//test2->transform->setEulerAngles(vec3(0, 0, 1));
	//player->transform->position = vec3(0);
	//player->getComponent<Physics>()->setActive(false);
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


