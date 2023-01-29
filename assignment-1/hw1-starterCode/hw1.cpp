/*
  CSCI 420 Computer Graphics, USC
  Assignment 1: Height Fields with Shaders.
  C++ starter code

  Student username: Baihua Yang
*/

#include "basicPipelineProgram.h"
#include "openGLMatrix.h"
#include "imageIO.h"
#include "openGLHeader.h"
#include "glutHeader.h"

#include <iostream>
#include <cstring>
#include <vector>

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

int mousePos[2]; // x,y coordinate of the mouse position

int leftMouseButton = 0; // 1 if pressed, 0 if not 
int middleMouseButton = 0; // 1 if pressed, 0 if not
int rightMouseButton = 0; // 1 if pressed, 0 if not

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROL_STATE;
CONTROL_STATE controlState = ROTATE;


// state of the world
float landRotate[3] = { 0.0f, 0.0f, 0.0f };
float landTranslate[3] = { 0.0f, 0.0f, 0.0f };
float landScale[3] = { 1.0f, 1.0f, 1.0f };

int windowWidth = 1280;
int windowHeight = 720;
char windowTitle[512] = "CSCI 420 homework I";

ImageIO* heightmapImage;

vec4 defaultColor = vec4(0.18, 0.75, 0.98, 1);
float heightScalar = 0.3f;
vec3 fieldCenter;
vec3 fieldOffset;

OpenGLMatrix matrix;
BasicPipelineProgram* pipelineProgram;

// control speeds
float translateSpeed = 0.1f;
float rotateSpeed = 0.5f;
float scaleSpeed = 0.01f;


class VertexArrayObject {
public:
	GLuint positionBuffer, colorBuffer;
	GLuint indexBuffer;
	GLuint vertexArray;
	int numVertices;
	GLenum drawMode = GL_POINTS;

	vector<int> indices;

	VertexArrayObject(vector<vec3> positions, vector<vec4> colors, vector<int> indices, GLenum drawMode) {
		numVertices = positions.size();
		if (numVertices == 0) {
			printf("error: number of vertices cannot be 0. ");
			return;
		}

		this->indices = indices;
		this->drawMode = drawMode;

		// position data
		glGenBuffers(1, &positionBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * numVertices, &positions[0], GL_STATIC_DRAW);

		// color data
		glGenBuffers(1, &colorBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vec4) * numVertices, &colors[0], GL_STATIC_DRAW);

		// vertex array
		glGenVertexArrays(1, &vertexArray);
		glBindVertexArray(vertexArray);

		// position attribute
		GLuint loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
		glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
		glEnableVertexAttribArray(loc);
		glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

		// color attribute
		loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "color");
		glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
		glEnableVertexAttribArray(loc);
		glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, (const void*)0);
	}

	void Draw() {
		glBindVertexArray(vertexArray);
		glDrawElements(drawMode, indices.size(), GL_UNSIGNED_INT, &indices[0]);
	}
};

// 0: points, 1: lines, 2: triangles 
VertexArrayObject* vaos[3];
int currentVaoIndex = 0;

// calculate color value based on given height value
vec4 calculateColor(float value) {
	//return defaultColor * value / 255.0f;
	return defaultColor;
}

// write a screenshot to the specified filename
void saveScreenshot(const char* filename) {
	unsigned char* screenshotData = new unsigned char[windowWidth * windowHeight * 3];
	glReadPixels(0, 0, windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, screenshotData);

	ImageIO screenshotImg(windowWidth, windowHeight, 3, screenshotData);

	if (screenshotImg.save(filename, ImageIO::FORMAT_JPEG) == ImageIO::OK)
		cout << "File " << filename << " saved successfully." << endl;
	else cout << "Failed to save file " << filename << '.' << endl;

	delete[] screenshotData;
}


void idleFunc() {
	// do some stuff... 

	// for example, here, you can save the screenshots to disk (to make the animation)

	// make the screen update 
	glutPostRedisplay();
}

void displayFunc() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	matrix.SetMatrixMode(OpenGLMatrix::ModelView);
	matrix.LoadIdentity();
	matrix.LookAt(0, 0, 5, 0, 0, 0, 0, 1, 0);

	matrix.Translate(landTranslate[0], landTranslate[1], landTranslate[2]);
	matrix.Rotate(landRotate[0], 1, 0, 0);
	matrix.Rotate(landRotate[1], 0, 1, 0);
	matrix.Rotate(landRotate[2], 0, 0, 1);
	matrix.Scale(landScale[0], landScale[1], landScale[2]);

	float m[16];
	matrix.SetMatrixMode(OpenGLMatrix::ModelView);
	matrix.GetMatrix(m);

	float p[16];
	matrix.SetMatrixMode(OpenGLMatrix::Projection);
	matrix.GetMatrix(p);

	// bind shader
	pipelineProgram->Bind();

	// set variable
	pipelineProgram->SetModelViewMatrix(m);
	pipelineProgram->SetProjectionMatrix(p);

	vaos[currentVaoIndex]->Draw();

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

		case 'x':
			// take a screenshot
			saveScreenshot("screenshot.jpg");
			break;

		case '1':
			currentVaoIndex = 0;
			break;

		case '2':
			currentVaoIndex = 1;
			break;

		case '3':
			currentVaoIndex = 2;
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
	switch (glutGetModifiers()) {
		case GLUT_ACTIVE_CTRL:
			controlState = TRANSLATE;
			break;

		case GLUT_ACTIVE_SHIFT:
			controlState = SCALE;
			break;

			// if CTRL and SHIFT are not pressed, we are in rotate mode
		default:
			controlState = ROTATE;
			break;
	}

	// store the new mouse position
	mousePos[0] = x;
	mousePos[1] = y;
}

void mouseMotionDragFunc(int x, int y) {
	// mouse has moved and one of the mouse buttons is pressed (dragging)

	// the change in mouse position since the last invocation of this function
	int mousePosDelta[2] = { x - mousePos[0], y - mousePos[1] };

	switch (controlState) {
		// translate the landscape
		case TRANSLATE:
			if (leftMouseButton) {
				// control x,y translation via the left mouse button
				landTranslate[0] += mousePosDelta[0] * translateSpeed;
				landTranslate[1] -= mousePosDelta[1] * translateSpeed;
			}
			if (middleMouseButton) {
				// control z translation via the middle mouse button
				landTranslate[2] += mousePosDelta[1] * translateSpeed;
			}
			break;

			// rotate the landscape
		case ROTATE:
			if (leftMouseButton) {
				// control x,y rotation via the left mouse button
				landRotate[0] += mousePosDelta[1] * rotateSpeed;
				landRotate[1] += mousePosDelta[0] * rotateSpeed;
			}
			if (middleMouseButton) {
				// control z rotation via the middle mouse button
				landRotate[2] += mousePosDelta[1] * rotateSpeed;
			}
			break;

			// scale the landscape
		case SCALE:
			if (leftMouseButton) {
				// control x,y scaling via the left mouse button
				landScale[0] *= 1.0f + mousePosDelta[0] * scaleSpeed;
				landScale[1] *= 1.0f - mousePosDelta[1] * scaleSpeed;
			}
			if (middleMouseButton) {
				// control z scaling via the middle mouse button
				landScale[2] *= 1.0f - mousePosDelta[1] * scaleSpeed;
			}
			break;
	}

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

	matrix.SetMatrixMode(OpenGLMatrix::Projection);
	matrix.LoadIdentity();
	matrix.Perspective(54.0f, (float)w / (float)h, 0.01f, 1000.0f);

	matrix.SetMatrixMode(OpenGLMatrix::ModelView);

}

void initScene(int argc, char* argv[]) {
	// load the image from a jpeg disk file to main memory
	heightmapImage = new ImageIO();
	if (heightmapImage->loadJPEG(argv[1]) != ImageIO::OK) {
		cout << "Error reading image " << argv[1] << "." << endl;
		exit(EXIT_FAILURE);
	}

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	// create pipeline
	pipelineProgram = new BasicPipelineProgram;
	int ret = pipelineProgram->Init(shaderBasePath);
	if (ret != 0) abort();

	// read image
	int width = heightmapImage->getWidth();
	int height = heightmapImage->getHeight();
	printf("\nImage info: \n\twidth: %i, height: %i, number of pixels: %i. \n\n", width, height, width * height);

	vector<vec3> pointPositions;
	vector<vec4> pointColors;
	vector<int> pointIndices;

	vector<vec3> linePositions;
	vector<vec4> lineColors;
	vector<int> lineIndices;

	vector<vec3> trianglePositions;
	vector<vec4> triangleColors;

	float xOffset = -width / 2.0;
	float yOffset = 0;
	float zOffset = height / 2.0;

	// init
	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			int index = i * width + j;

			float pixelValue = heightmapImage->getPixel(i, j, 0);

			float x = i + xOffset;
			float y = heightScalar * pixelValue + yOffset;
			float z = -j + zOffset;

			// points
			pointPositions.push_back(vec3(x, y, z));
			pointColors.push_back(calculateColor(pixelValue));
			pointIndices.push_back(index);

			// lines
			if (i > 0) {
				// add left lines
				lineIndices.push_back(index - height);
				lineIndices.push_back(index);
			}

			if (j > 0) {
				// add bottom lines
				lineIndices.push_back(index - 1);
				lineIndices.push_back(index);
			}

			//// triangles
			//if (i > 0 && j > 0) {
			//	if (i % 2 == 0) {
			//		index = height - j - 1;
			//	}
			//	trianglePositions.push_back(pointPositions[index - height]);
			//	trianglePositions.push_back(pointPositions[index - height - 1]);
			//	trianglePositions.push_back(pointPositions[index]);
			//	trianglePositions.push_back(pointPositions[index - 1]);

			//	triangleColors.push_back(pointColors[index - height]);
			//	triangleColors.push_back(pointColors[index - height - 1]);
			//	triangleColors.push_back(pointColors[index]);
			//	triangleColors.push_back(pointColors[index - 1]);
			//}
		}
	}
	fieldCenter = vec3(width / 2.0 + xOffset, yOffset, -height / 2.0 + zOffset);

	vaos[0] = new VertexArrayObject(pointPositions, pointColors, pointIndices, GL_POINTS);
	vaos[1] = new VertexArrayObject(pointPositions, pointColors, lineIndices, GL_LINES);
	//vaos[2] = new VertexArrayObject(trianglePositions, triangleColors, GL_TRIANGLE_STRIP);


	glEnable(GL_DEPTH_TEST);

	std::cout << "GL error: " << glGetError() << std::endl;
}

int main(int argc, char* argv[]) {
	if (argc != 2) {
		cout << "The arguments are incorrect." << endl;
		cout << "usage: ./hw1 <heightmap file>" << endl;
		exit(EXIT_FAILURE);
	}

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
	initScene(argc, argv);

	// sink forever into the glut loop
	glutMainLoop();
}


