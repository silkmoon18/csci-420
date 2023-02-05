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




OpenGLMatrix matrix;
BasicPipelineProgram* pipelineProgram;

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

// set up the parameters for lighting 
vec4 light_pos = vec4(0, 0, 0, 1);
vec4 light_ambient = vec4( 1, 1, 1, 1);
vec4 light_diffuse = vec4(.8, .8, .8, 1);
vec4 light_specular = vec4(1, 1, 1, 1);
vec4 mat_ambient = vec4(1, 1, 1, 1);
vec4 mat_diffuse = vec4(1, 1, 1, 1);
vec4 mat_specular = vec4(.9, .9, .9, 1);
float mat_shine = 50;

int windowWidth = 1280;
int windowHeight = 720;
char windowTitle[512] = "CSCI 420 homework I";

ImageIO* heightmapImage;
string imageDirectory = "./heightmap";
vector<string> imagePaths;
int currentImageIndex = 0;

vec4 defaultColor = vec4(0.18, 0.75, 0.98, 1);
//vec4 defaultColor = vec4(1);

float heightScalar = 0.3f;
vec3 fieldCenter;
vec3 eyePosition;

// control speeds
float translateSpeed = 0.1f;
float rotateSpeed = 0.5f;
float scaleSpeed = 0.01f;

const int restartIndex = -1;


// object that handles VAO related operations
class VertexArrayObject {
public:
	GLuint positionBuffer, colorBuffer, indexBuffer;
	GLuint vertexArray;
	GLenum drawMode = GL_POINTS;
	int numVertices, numColors, numIndices;

	VertexArrayObject(vector<vec3> positions, vector<vec4> colors, vector<int> indices, GLenum drawMode) {
		numVertices = positions.size();
		numColors = colors.size();
		numIndices = indices.size();
		if (numVertices == 0) {
			printf("\nerror: the number of vertices cannot be 0. \n");
			return;
		}if (numColors == 0) {
			printf("\nerror: the number of colors cannot be 0. \n");
			return;
		}if (numIndices == 0) {
			printf("\nerror: the number of indices cannot be 0. \n");
			return;
		}if (numVertices != numColors) {
			printf("\nerror: the number of vertices %i does not match the number of colors %i. \n", numVertices, numColors);
			return;
		}

		this->drawMode = drawMode;

		// vertex array
		glGenVertexArrays(1, &vertexArray);
		glBindVertexArray(vertexArray);

		// position data
		glGenBuffers(1, &positionBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * numVertices, &positions[0], GL_STATIC_DRAW);

		// color data
		glGenBuffers(1, &colorBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vec4) * numColors, &colors[0], GL_STATIC_DRAW);

		// index data
		glGenBuffers(1, &indexBuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int) * numIndices, &indices[0], GL_STATIC_DRAW);

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


		printf("Created VAO: numVertices %i, numColors %i, numIndices %i\n", numVertices, numColors, numIndices);
	}

	void Draw() {
		// lighting
		GLuint loc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "lightPosition");
		glUniform4f(loc, light_pos[0], light_pos[1], light_pos[2], light_pos[3]);

		loc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "ambientCoef");
		glUniform4f(loc, mat_ambient[0], mat_ambient[1], mat_ambient[2], mat_ambient[3]);
		loc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "diffuseCoef");
		glUniform4f(loc, mat_diffuse[0], mat_diffuse[1], mat_diffuse[2], mat_diffuse[3]);
		loc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "specularCoef");
		glUniform4f(loc, mat_specular[0], mat_specular[1], mat_specular[2], mat_specular[3]);
		loc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "materialShininess");
		glUniform1f(loc, mat_shine);

		loc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "lightAmbient");
		glUniform4f(loc, light_ambient[0], light_ambient[1], light_ambient[2], light_ambient[3]);
		loc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "lightDiffuse");
		glUniform4f(loc, light_diffuse[0], light_diffuse[1], light_diffuse[2], light_diffuse[3]);
		loc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "lightSpecular");
		glUniform4f(loc, light_specular[0], light_specular[1], light_specular[2], light_specular[3]);

		// send height scalar
		GLint heightScaleLoc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "heightScale");
		glUniform1f(heightScaleLoc, heightScalar);

		glBindVertexArray(vertexArray);
		glDrawElements(drawMode, numIndices, GL_UNSIGNED_INT, 0);
	}
};

// 0: points, 1: lines, 2: triangles, 3: smoothened
VertexArrayObject* vaos[5];
int currentVaoIndex = 0;


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

// calculate color value based on given height value
vec4 calculateColor(float value) {
	vec4 normalized = defaultColor * value;
	return vec4(normalized.x, normalized.y, normalized.z, defaultColor.w);
}

void FindImages() {
	for (const auto& entry : filesystem::directory_iterator(imageDirectory)) {
		imagePaths.push_back(entry.path().string());
		printf("Image found: %s\n", entry.path().string().c_str());
	}
	if (imagePaths.size() == 0) {
		printf("error: no images found. \n");
	}
}

void generateField() {
	// load the image from a jpeg disk file to main memory
	string imagePath = imagePaths[currentImageIndex];

	// debug
	//imagePath = "./heightmap/GrandTeton-768.jpg";

	heightmapImage = new ImageIO();
	if (heightmapImage->loadJPEG(imagePath.c_str()) != ImageIO::OK) {
		cout << "Error reading image " << imagePath << "." << endl;
		exit(EXIT_FAILURE);
	}

	// read image
	int width = heightmapImage->getWidth();
	int height = heightmapImage->getHeight();

	// vertex attributes
	vector<vec3> vertexPositions;
	vector<vec4> vertexColors;

	// for mode 0
	// vertex indices
	vector<int> pointIndices;
	vector<int> lineIndices;
	vector<int> triangleIndices;

	// position offsets
	float xOffset = -width / 2.0f;
	float zOffset = height / 2.0f;

	vector<vector<float>> heights;
	float maxHeight = 0;

	bool isRGB = !(heightmapImage->getBytesPerPixel() == 1);

	printf("\nImage info: \n\tfilename: %s, width: %i, height: %i, number of pixels: %i, rgb: %s. \n\n", 
		   imagePath.c_str(), width, height, width * height, isRGB ? "true" : "false");

	// init
	for (int i = 0; i < width; i++) {
		heights.push_back(vector<float>());

		for (int j = 0; j < height; j++) {
			int index = i * height + j;
			float x, y, z, lum;

			// vertex color
			vec4 color;
			if (isRGB) {
				float r = heightmapImage->getPixel(i, j, 0);
				float g = heightmapImage->getPixel(i, j, 1);
				float b = heightmapImage->getPixel(i, j, 2);
				lum = 0.2126 * r + 0.7152 * g + 0.0722 * b;
				color = vec4(r, g, b, 255);
				color.w = 1;
			}
			else {
				lum = heightmapImage->getPixel(i, j, 0);
				color = vec4(255, 255, 255, 255);
				color.w = 1;
			}
			vertexColors.push_back(color);

			// vertex position
			x = i + xOffset;
			y = lum * heightScalar;
			z = -j + zOffset;
			vertexPositions.push_back(vec3(x, y, z));

			// add height
			heights[i].push_back(y);
			if (y > maxHeight) {
				maxHeight = y;
			}

			// add point index
			pointIndices.push_back(index);

			// add line indices
			if (i > 0) {
				// add left line indices
				lineIndices.push_back(index - height);
				lineIndices.push_back(index);
			}
			if (j > 0) {
				// add bottom line indices
				lineIndices.push_back(index - 1);
				lineIndices.push_back(index);
			}

			// add triangle indices
			if (i > 0 && j > 0) {
				triangleIndices.push_back(index - height - 1);
				triangleIndices.push_back(index - 1);
				triangleIndices.push_back(index - height);
				triangleIndices.push_back(index);

				// restart after each column
				if (j == height - 1) {
					triangleIndices.push_back(restartIndex);
				}
			}
		}
	}

	// set other positions
	fieldCenter = vec3(width / 2.0f + xOffset, 0, -height / 2.0f + zOffset);
	eyePosition = vec3(fieldCenter.x, fieldCenter.y + maxHeight, height * 1.25f);
	light_pos = vec4(fieldCenter.x, eyePosition.y, fieldCenter.z, 1);

	// create VAOs
	vaos[0] = new VertexArrayObject(vertexPositions, vertexColors, pointIndices, GL_POINTS);
	vaos[1] = new VertexArrayObject(vertexPositions, vertexColors, lineIndices, GL_LINES);
	vaos[2] = new VertexArrayObject(vertexPositions, vertexColors, triangleIndices, GL_TRIANGLE_STRIP);

	glPointSize(10);

	// init mode 1
	// heights of points of top, bottom, left, right
	vector<vec4> neighborHeights;

	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			vec4 neighborHeight;

			// get neighbors' heights.
			// top 
			neighborHeight.x = j == height - 1 ? heights[i][j - 1] : heights[i][j + 1];

			// bottom 
			neighborHeight.y = j == 0 ? heights[i][j + 1] : heights[i][j - 1];

			// left 
			neighborHeight.z = i == 0 ? heights[i + 1][j] : heights[i - 1][j];

			// right 
			neighborHeight.w = i == width - 1 ? heights[i - 1][j] : heights[i + 1][j];

			neighborHeights.push_back(neighborHeight);
		}
	}
	vaos[3] = new VertexArrayObject(vertexPositions, vertexColors, triangleIndices, GL_TRIANGLE_STRIP);

	GLuint heightsBuffer;
	glGenBuffers(1, &heightsBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, heightsBuffer);  // bind the VBO buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec4) * neighborHeights.size(), &neighborHeights[0], GL_STATIC_DRAW);

	GLuint loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "neighborHeights");
	glBindBuffer(GL_ARRAY_BUFFER, heightsBuffer);
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, (const void*)0);

	// debug 

	vaos[4] = new VertexArrayObject(vertexPositions, vertexColors, pointIndices, GL_POINTS);
	 heightsBuffer;
	glGenBuffers(1, &heightsBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, heightsBuffer);  // bind the VBO buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec4)* neighborHeights.size(), &neighborHeights[0], GL_STATIC_DRAW);

	 loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "neighborHeights");
	glBindBuffer(GL_ARRAY_BUFFER, heightsBuffer);
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, (const void*)0);
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
	matrix.LookAt(eyePosition.x, eyePosition.y, eyePosition.z,
				  fieldCenter.x, fieldCenter.y, fieldCenter.z,
				  0, 1, 0);

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
	GLint modeLoc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "mode");

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

		// point mode
		case '1':
			currentVaoIndex = 0;
			glUniform1i(modeLoc, 0);
			break;

		// line mode
		case '2':
			currentVaoIndex = 1;
			glUniform1i(modeLoc, 0);
			break;

		// triangle mode
		case '3':
			currentVaoIndex = 2;
			glUniform1i(modeLoc, 0);
			break;

		// smoothened mode
		case '4':
			currentVaoIndex = 3;
			glUniform1i(modeLoc, 1);
			break;
		
		// debug

		case '5':
			currentVaoIndex = 4;
			glUniform1i(modeLoc, 1);
			break;

		// previous image
		case 'o':
			currentImageIndex--;
			if (currentImageIndex < 0) {
				currentImageIndex = imagePaths.size() - 1;
			}
			generateField();
			break;

		// next image
		case 'p':
			currentImageIndex++;
			if (currentImageIndex == imagePaths.size()) {
				currentImageIndex = 0;
			}
			generateField();
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
	matrix.Perspective(54.0f, (float)w / (float)h, 0.01f, 3000.0f);

	matrix.SetMatrixMode(OpenGLMatrix::ModelView);

}

void initScene() {
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	// set restart index
	glEnable(GL_PRIMITIVE_RESTART);
	glPrimitiveRestartIndex(restartIndex);

	// create pipeline
	pipelineProgram = new BasicPipelineProgram;
	int ret = pipelineProgram->Init(shaderBasePath);
	if (ret != 0) abort();


	generateField();


	glEnable(GL_DEPTH_TEST);

	cout << "\nGL error: " << glGetError() << endl;
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
	FindImages();
	initScene();

	// sink forever into the glut loop
	glutMainLoop();
}


