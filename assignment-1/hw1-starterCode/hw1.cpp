/*
  CSCI 420 Computer Graphics, USC
  Assignment 1: Height Fields with Shaders.

  Student username: Baihua Yang
*/

#include "basicPipelineProgram.h"
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

BasicPipelineProgram *pipelineProgram;

int mousePos[2]; // x,y coordinate of the mouse position

int leftMouseButton = 0;   // 1 if pressed, 0 if not
int middleMouseButton = 0; // 1 if pressed, 0 if not
int rightMouseButton = 0;  // 1 if pressed, 0 if not

typedef enum
{
	ROTATE,
	TRANSLATE,
	SCALE
} CONTROL_STATE;
CONTROL_STATE controlState = ROTATE;

// state of the world
vec3 landRotate = vec3(0);
vec3 landTranslate = vec3(0);
vec3 landScale = vec3(1);

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

// control speeds
float translateSpeed = 0.1f;
float rotateSpeed = 0.5f;
float scaleSpeed = 0.01f;

// other params
const float heightScalar = 0.3f;
vec3 fieldCenter;
vec3 eyePosition;
const int restartIndex = -1;
bool wireframeEnabled = false;
bool isTakingScreenshot = false;

// 0: points, 1: lines, 2: triangles, 3: smoothened, 4: wireframe for 2
Entity *holder;
vector<Entity *> fields;
Entity *camera;
int currentFieldIndex = 0;

// write a screenshot to the specified filename
void saveScreenshot()
{
	if (screenshotIndex > 999)
	{
		cout << "Num of screenshots has reached limit 999. " << endl;
		return;
	}

	int digit = to_string(screenshotIndex).length();
	string filename = "./screenshots/";
	for (int i = digit; i < 3; i++)
	{
		filename += "0";
	}
	filename += to_string(screenshotIndex + 1) + ".jpg";

	unsigned char *screenshotData = new unsigned char[windowWidth * windowHeight * 3];
	glReadPixels(0, 0, windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, screenshotData);

	ImageIO screenshotImg(windowWidth, windowHeight, 3, screenshotData);

	if (screenshotImg.save(filename.c_str(), ImageIO::FORMAT_JPEG) == ImageIO::OK)
	{
		cout << "File " << filename << " saved successfully." << endl;
		screenshotIndex++;
	}
	else
		cout << "Failed to save file " << filename << '.' << endl;

	delete[] screenshotData;
}

ImageIO *tryReadImage(string imagePath)
{
	ImageIO *image = new ImageIO();
	if (image->loadJPEG(imagePath.c_str()) != ImageIO::OK)
	{
		return NULL;
	}
	return image;
}

// find images in directory
bool FindImages()
{
	printf("\n");
	if (imageDirectory.empty())
	{
		printf("Error: image directory is not specified. \n");
		return false;
	}
	if (!filesystem::exists(imageDirectory))
	{
		printf("Error: directory %s does not exist. \n", imageDirectory.c_str());
		return false;
	}

	printf("Finding images...\n");
	for (const auto &entry : filesystem::directory_iterator(imageDirectory))
	{
		filesystem::path path = entry.path();

		string extension = path.extension().string();
		if (extension == ".jpg" || extension == ".jpeg")
		{
			printf("Image found: %s\n", path.string().c_str());
			imagePaths.push_back(path.string());
		}
		else
		{
			printf("Error: file %s is not jpg. \n", path.string().c_str());
		}
	}

	int size = imagePaths.size();
	if (size == 0)
	{
		printf("Error: no images found. \n");
		return false;
	}
	else
	{
		printf("Found %i images in %s\n", size, imageDirectory.c_str());
	}
	printf("\n");
	return true;
}

void setUniforms()
{
	// set height scalar
	GLint heightScaleLoc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "heightScale");
	glUniform1f(heightScaleLoc, heightScalar);

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

// generate heightfield
void generateField()
{
	// load the image from a jpeg disk file to main memory
	string path = imagePaths[currentImageIndex];
	ImageIO *image = tryReadImage(path);
	if (!image)
	{
		printf("Error: cannot read image %s. \n", path.c_str());
		return;
	}

	// read image
	int width = image->getWidth();
	int height = image->getHeight();

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

	bool isRGB = !(image->getBytesPerPixel() == 1);

	printf("\nImage info: \n\twidth: %i, height: %i, number of pixels: %i, rgb: %s. \n\n",
		   width, height, width * height, isRGB ? "true" : "false");

	// init
	for (int i = 0; i < width; i++)
	{
		heights.push_back(vector<float>());

		for (int j = 0; j < height; j++)
		{
			int index = i * height + j;
			float x, y, z, lum;

			// vertex color
			vec4 color;
			if (isRGB)
			{
				float r = image->getPixel(i, j, 0);
				float g = image->getPixel(i, j, 1);
				float b = image->getPixel(i, j, 2);
				lum = 0.2126f * r + 0.7152f * g + 0.0722f * b;
				color = vec4(r, g, b, 255);
				color.w = 1;
			}
			else
			{
				lum = image->getPixel(i, j, 0);
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
			if (y > maxHeight)
			{
				maxHeight = y;
			}

			// add point index
			pointIndices.push_back(index);

			// add line indices
			if (i > 0)
			{
				// add left line indices
				lineIndices.push_back(index - height);
				lineIndices.push_back(index);
			}
			if (j > 0)
			{
				// add bottom line indices
				lineIndices.push_back(index - 1);
				lineIndices.push_back(index);
			}

			// add triangle indices
			if (i > 0 && j > 0)
			{
				triangleIndices.push_back(index - height - 1);
				triangleIndices.push_back(index - 1);
				triangleIndices.push_back(index - height);
				triangleIndices.push_back(index);

				// restart after each column
				if (j == height - 1)
				{
					triangleIndices.push_back(restartIndex);
				}
			}
		}
	}

	// set other positions
	fieldCenter = vec3(width / 2.0f + xOffset, 0, -height / 2.0f + zOffset);
	eyePosition = vec3(fieldCenter.x, fieldCenter.y + maxHeight, height * 1.25f);

	camera->transform->setPosition(eyePosition, true);
	camera->transform->faceTo(fieldCenter);
	lightPosition = vec4(fieldCenter.x, eyePosition.y, fieldCenter.z, 1);

	// create field entities
	fields[0]->getComponent<Renderer>()->setVAO(
		new VertexArrayObject(pipelineProgram, vertexPositions, vertexColors, pointIndices));
	fields[1]->getComponent<Renderer>()->setVAO(
		new VertexArrayObject(pipelineProgram, vertexPositions, vertexColors, lineIndices));
	fields[2]->getComponent<Renderer>()->setVAO(
		new VertexArrayObject(pipelineProgram, vertexPositions, vertexColors, triangleIndices));

	// init mode 1
	// heights of points of top, bottom, left, right
	vector<vec4> neighborHeights;

	for (int i = 0; i < width; i++)
	{
		for (int j = 0; j < height; j++)
		{
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

	VertexArrayObject *vao = new VertexArrayObject(pipelineProgram, vertexPositions, vertexColors, triangleIndices);
	vao->sendData(neighborHeights, 4, "neighborHeights");
	fields[3]->getComponent<Renderer>()->setVAO(vao);

	vector<vec4> wireframeColors(vertexColors.size());
	for (vec4 color : wireframeColors)
	{
		color = vec4(1);
	}
	vao = new VertexArrayObject(pipelineProgram, vertexPositions, wireframeColors, lineIndices);
	vao->sendData(neighborHeights, 4, "neighborHeights");
	fields[4]->getComponent<Renderer>()->setVAO(vao);
}

int delay = 8; // hard coded for recording on 120 fps monitor
int currentFrame = 0;
void idleFunc()
{
	// calculate delta time
	Timer::getInstance()->setCurrentTime(glutGet(GLUT_ELAPSED_TIME));

	// save 15 screenshots per second
	if (isTakingScreenshot && currentFrame % 8 == 0)
	{
		saveScreenshot();
	}

	// make the screen update
	glutPostRedisplay();
	currentFrame++;
}

void displayFunc()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// set uniforms
	setUniforms();

	// set current active, others inactive
	for (int i = 0; i < fields.size(); i++)
	{
		fields[i]->setActive(i == currentFieldIndex);
	}
	if ((currentFieldIndex == 2 || currentFieldIndex == 3) && wireframeEnabled)
	{
		fields[4]->setActive(true);
	}

	// update holder's transform
	holder->transform->setPosition(landTranslate, true);
	holder->transform->setEulerAngles(landRotate, true);
	holder->transform->setScale(landScale, true);

	// update entities
	EntityManager::getInstance()->update();

	glutSwapBuffers();
}

void keyboardFunc(unsigned char key, int x, int y)
{
	GLint polygonModeLoc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "polygonMode");
	GLint lightModeLoc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "lightMode");

	switch (key)
	{
	case 27:	 // ESC key
		exit(0); // exit the program
		break;

	case ' ':
		cout << "You pressed the spacebar." << endl;
		break;

	// toggle screenshots recording
	case 'x':
		isTakingScreenshot = !isTakingScreenshot;
		break;

	// point mode
	case '1':
		currentFieldIndex = 0;
		glUniform1i(polygonModeLoc, 0);
		break;

	// line mode
	case '2':
		currentFieldIndex = 1;
		glUniform1i(polygonModeLoc, 0);
		break;

	// triangle mode
	case '3':
		currentFieldIndex = 2;
		glUniform1i(polygonModeLoc, 0);
		break;

	// smoothened mode
	case '4':
		currentFieldIndex = 3;
		glUniform1i(polygonModeLoc, 1);
		break;

	// previous image
	case 'o':
		currentImageIndex--;
		if (currentImageIndex < 0)
		{
			currentImageIndex = imagePaths.size() - 1;
		}
		generateField();
		break;

	// next image
	case 'p':
		currentImageIndex++;
		if (currentImageIndex == imagePaths.size())
		{
			currentImageIndex = 0;
		}
		generateField();
		break;

	// toggle wireframe display
	case 'l':
		wireframeEnabled = !wireframeEnabled;
		break;

	// default lighting
	case 'v':
		glUniform1i(lightModeLoc, 0);
		break;
	// ambient lighting
	case 'b':
		glUniform1i(lightModeLoc, 1);
		break;
	// diffuse lighting
	case 'n':
		glUniform1i(lightModeLoc, 2);
		break;
	// specular lighting
	case 'm':
		glUniform1i(lightModeLoc, 3);
		break;
	}
}

void mouseButtonFunc(int button, int state, int x, int y)
{
	// a mouse button has has been pressed or depressed

	// keep track of the mouse button state, in leftMouseButton, middleMouseButton, rightMouseButton variables
	switch (button)
	{
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
	switch (glutGetModifiers())
	{
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

void mouseMotionDragFunc(int x, int y)
{
	// mouse has moved and one of the mouse buttons is pressed (dragging)

	// the change in mouse position since the last invocation of this function
	int mousePosDelta[2] = {x - mousePos[0], y - mousePos[1]};

	switch (controlState)
	{
	// translate the landscape
	case TRANSLATE:
		if (leftMouseButton)
		{
			// control x,y translation via the left mouse button
			landTranslate[0] += mousePosDelta[0] * translateSpeed;
			landTranslate[1] -= mousePosDelta[1] * translateSpeed;
		}
		if (middleMouseButton)
		{
			// control z translation via the middle mouse button
			landTranslate[2] += mousePosDelta[1] * translateSpeed;
		}
		break;

		// rotate the landscape
	case ROTATE:
		if (leftMouseButton)
		{
			// control x,y rotation via the left mouse button
			landRotate[0] += mousePosDelta[1] * rotateSpeed;
			landRotate[1] += mousePosDelta[0] * rotateSpeed;
		}
		if (middleMouseButton)
		{
			// control z rotation via the middle mouse button
			landRotate[2] += mousePosDelta[1] * rotateSpeed;
		}
		break;

		// scale the landscape
	case SCALE:
		if (leftMouseButton)
		{
			// control x,y scaling via the left mouse button
			landScale[0] *= 1.0f + mousePosDelta[0] * scaleSpeed;
			landScale[1] *= 1.0f - mousePosDelta[1] * scaleSpeed;
		}
		if (middleMouseButton)
		{
			// control z scaling via the middle mouse button
			landScale[2] *= 1.0f - mousePosDelta[1] * scaleSpeed;
		}
		break;
	}

	// store the new mouse position
	mousePos[0] = x;
	mousePos[1] = y;
}

void mouseMotionFunc(int x, int y)
{
	// mouse has moved
	// store the new mouse position
	mousePos[0] = x;
	mousePos[1] = y;
}

void reshapeFunc(int w, int h)
{
	glViewport(0, 0, w, h);

	Camera::currentCamera->setPerspective(Camera::currentCamera->fieldOfView,
										  (float)w / (float)h,
										  Camera::currentCamera->zNear,
										  Camera::currentCamera->zFar);
}

void initScene()
{
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
	if (ret != 0)
		abort();

	// init entities
	holder = EntityManager::getInstance()->createEntity("Holder");

	Entity *points = EntityManager::getInstance()->createEntity("Points");
	points->addComponent(new Renderer(GL_POINTS));
	points->setParent(holder);
	fields.push_back(points);

	Entity *lines = EntityManager::getInstance()->createEntity("Lines");
	lines->addComponent(new Renderer(GL_LINE_STRIP));
	lines->setParent(holder);
	fields.push_back(lines);

	Entity *triangles = EntityManager::getInstance()->createEntity("Triangles");
	triangles->addComponent(new Renderer(GL_TRIANGLE_STRIP));
	triangles->setParent(holder);
	fields.push_back(triangles);

	Entity *smoothened = EntityManager::getInstance()->createEntity("Smoothened");
	smoothened->addComponent(new Renderer(GL_TRIANGLE_STRIP));
	smoothened->setParent(holder);
	fields.push_back(smoothened);

	Entity *wireframe = EntityManager::getInstance()->createEntity("Wireframe");
	wireframe->addComponent(new Renderer(GL_LINE_STRIP));
	wireframe->setParent(holder);
	fields.push_back(wireframe);

	// init camera
	camera = EntityManager::getInstance()->createEntity("Camera");
	camera->addComponent(new Camera());
	camera->getComponent<Camera>()->zFar = 3000.0f;

	generateField();

	cout << "\nGL error: " << glGetError() << endl;
}

int main(int argc, char *argv[])
{
	if (argc > 1)
	{
		imageDirectory = argv[1];
	}

	bool isValid = FindImages();
	while (!isValid)
	{
		cout << "Please specify a directory that contains at least one jpg image: \n"
			 << endl;
		cin >> imageDirectory;
		isValid = FindImages();
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
	if (result != GLEW_OK)
	{
		cout << "error: " << glewGetErrorString(result) << endl;
		exit(EXIT_FAILURE);
	}
#endif

	// do initialization
	initScene();

	// sink forever into the glut loop
	glutMainLoop();
}
