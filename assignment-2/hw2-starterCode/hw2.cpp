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





BasicPipelineProgram* pipelineProgram;

int mousePos[2]; // x,y coordinate of the mouse position

int leftMouseButton = 0; // 1 if pressed, 0 if not 
int middleMouseButton = 0; // 1 if pressed, 0 if not
int rightMouseButton = 0; // 1 if pressed, 0 if not

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROL_STATE;
CONTROL_STATE controlState = ROTATE;

// timer
int previousTime = 0; // in ms
float deltaTime = 0; // in s

// entities
EntityManager entityManager;
Camera* camera;
Entity* trackEntity;

// lighting 
vec4 lightPosition = vec4(0, 0, 0, 1);
vec4 lightAmbient = vec4(1, 1, 1, 1);
vec4 lightDiffuse = vec4(.8, .8, .8, 1);
vec4 lightSpecular = vec4(1, 1, 1, 1);
vec4 ambientCoef = vec4(0.5, 0.5, 0.5, 1);
vec4 diffuseCoef = vec4(1, 1, 1, 1);
vec4 specularCoef = vec4(.9, .9, .9, 1);
float materialShininess = 1;

// display window
int windowWidth = 1280;
int windowHeight = 720;
char windowTitle[512] = "CSCI 420 homework I";

// images
vector<string> imagePaths;
string imageDirectory;
int currentImageIndex = 0;
int screenshotIndex = 0;

// controls
float mouseSensitivity = 5.0f;
vec2 xAngleLimit(-90, 80);
float translateSpeed = 0.1f;
float rotateSpeed = 0.5f;
float scaleSpeed = 0.01f;

// physics
float rcSpeed = 1.0f;

// other params
const int restartIndex = -1;
bool isTakingScreenshot = false;
int delay = 8; // hard coded for recording on 120 fps monitor
int currentFrame = 0;


// ---assignment-2---

// the spline array 
Spline* splines;
// total number of splines 
int numSplines;

// catmull-rom spline
mat4 basis;

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
	GLuint loc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "lightPosition");
	glUniform4f(loc, lightPosition[0], lightPosition[1], lightPosition[2], lightPosition[3]);

	loc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "ambientCoef");
	glUniform4f(loc, ambientCoef[0], ambientCoef[1], ambientCoef[2], ambientCoef[3]);
	loc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "diffuseCoef");
	glUniform4f(loc, diffuseCoef[0], diffuseCoef[1], diffuseCoef[2], diffuseCoef[3]);
	loc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "specularCoef");
	glUniform4f(loc, specularCoef[0], specularCoef[1], specularCoef[2], specularCoef[3]);
	loc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "materialShininess");
	glUniform1f(loc, materialShininess);

	loc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "lightAmbient");
	glUniform4f(loc, lightAmbient[0], lightAmbient[1], lightAmbient[2], lightAmbient[3]);
	loc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "lightDiffuse");
	glUniform4f(loc, lightDiffuse[0], lightDiffuse[1], lightDiffuse[2], lightDiffuse[3]);
	loc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "lightSpecular");
	glUniform4f(loc, lightSpecular[0], lightSpecular[1], lightSpecular[2], lightSpecular[3]);
}



int numOfStepsPerSegment = 1000;
float s = 0.5f;
vector<SplineObject*> splineObjects;
int currentSplineObjectIndex = -1;

void createSplineObjects() {
	vector<vec3> positions;
	vector<vec4> colors;
	vector<int> indices;
	for (int i = 0; i < numSplines; i++) {
		Spline spline = splines[i];

		vector<vec3> tangents;
		for (int j = 1; j < spline.numControlPoints - 2; j++) {
			mat3x4 control = mat3x4(
				spline.points[j - 1].x, spline.points[j].x, spline.points[j + 1].x, spline.points[j + 2].x,
				spline.points[j - 1].y, spline.points[j].y, spline.points[j + 1].y, spline.points[j + 2].y,
				spline.points[j - 1].z, spline.points[j].z, spline.points[j + 1].z, spline.points[j + 2].z
			);

			float delta = 1.0f / numOfStepsPerSegment;
			for (int k = 0; k < numOfStepsPerSegment + 1; k++) {
				float u = k * 1.0f / numOfStepsPerSegment;
				vec4 uVector(u * u * u, u * u, u, 1);
				vec3 point = uVector * basis * control;
				positions.push_back(point);
				indices.push_back(indices.size());

				vec4 uPrime(3 * u * u, 2 * u, 1, 0);
				tangents.push_back(normalize(uPrime * basis * control));

				u += delta;
			}
		}
		colors = vector<vec4>(positions.size(), vec4(255));

		trackEntity = entityManager.createEntity(new VertexArrayObject(pipelineProgram, positions, colors, indices, GL_LINE_STRIP));
		splineObjects.push_back(new SplineObject(spline, positions, tangents));
	}

	if (splineObjects.size() > 0) {
		currentSplineObjectIndex = 0;
	}
}

void HandleCameraMotion() {
	SplineObject* splineObject = splineObjects[currentSplineObjectIndex];

	float step = rcSpeed * deltaTime;
	vec3 position = splineObject->moveForward(step);

	vec3 direction = splineObject->getDirection();

	camera->transform.position = position;
	camera->faceTo(position + direction);
}

void idleFunc() {
	// calculate delta time
	int currentTime = glutGet(GLUT_ELAPSED_TIME);
	deltaTime = (currentTime - previousTime) / 1000.0f;
	previousTime = currentTime;

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

	HandleCameraMotion();

	// set uniforms
	setUniforms();

	// debug
	//cameraEntity->transform.rotation.z++;

	// update entities
	entityManager.update();

	glutSwapBuffers();
}

void keyboardFunc(unsigned char key, int x, int y) {
	switch (key) {
		case 27: // ESC key
			exit(0); // exit the program
			break;

		case ' ':
			cout << "You pressed the spacebar." << endl;
			break;

			// change light position
			//case 'w':
			//	light_pos[2] += lightTranslateSpeed;
			//	break;
			//case 's':
			//	light_pos[2] -= lightTranslateSpeed;
			//	break;
			//case 'a':
			//	light_pos[0] -= lightTranslateSpeed;
			//	break;
			//case 'd':
			//	light_pos[0] += lightTranslateSpeed;
			//	break;
			//case 'q':
			//	light_pos[1] -= lightTranslateSpeed;
			//	break;
			//case 'e':
			//	light_pos[1] += lightTranslateSpeed;
			//	break;

			// toggle screenshots recording
		case 'x':
			isTakingScreenshot = !isTakingScreenshot;
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

	// the change in mouse position since the last invocation of this function
	int mousePosDelta[2] = { x - mousePos[0], y - mousePos[1] };

	float lookStep = mouseSensitivity * deltaTime;
	camera->transform.rotation.x += mousePosDelta[1] * lookStep;
	camera->transform.rotation.y += mousePosDelta[0] * lookStep;
	camera->transform.rotation.x = std::clamp(camera->transform.rotation.x, xAngleLimit.x, xAngleLimit.y);

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

	camera->setPerspective(54.0f, (float)w / (float)h, 0.01f, 1000.0f);
}


void initScene() {
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glEnable(GL_DEPTH_TEST);

	// polygon offset
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(1, 1);

	// restart index
	glEnable(GL_PRIMITIVE_RESTART);
	glPrimitiveRestartIndex(restartIndex);

	// pipeline
	pipelineProgram = new BasicPipelineProgram;
	int ret = pipelineProgram->Init(shaderBasePath);
	if (ret != 0) abort();

	cout << "\nGL error: " << glGetError() << endl;
}

void initObjects() {
	camera = entityManager.createCamera();
	camera->enable();
	camera->transform.position = vec3(0, 0, 5);

	//debug 
	//camera->transform.rotation = vec3(0, 45, 45);
	//log(camera->getForwardVector());
	//log(camera->getRightVector());
	//log(camera->getUpVector());
	//camera->faceTo(vec3(10, 10, -10));

	basis = mat4(
		-s, 2 * s, -s, 0,
		2 - s, s - 3, 0, 1,
		s - 2, 3 - 2 * s, s, 0,
		s, -s, 0, 0
	);

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


