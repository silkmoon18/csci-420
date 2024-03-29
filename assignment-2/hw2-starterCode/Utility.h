#ifndef _UTILITY_H_
#define _UTILITY_H_

#define GLM_FORCE_RADIANS

#include "glm/gtx/transform.hpp"
#include "glm/gtx/euler_angles.hpp"
#include "glm/gtx/matrix_decompose.hpp"
#include "basicPipelineProgram.h"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/epsilon.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "imageIO.h"

#include <vector>
#include <map>
#include <algorithm>
#include <iostream>
#include <string>
#include <filesystem>

using namespace std;
using namespace glm;

template<class T> class Singleton;
class Timer;
class SceneManager;
class Entity;

class Component;
class Transform;
class Renderer;
class Physics;
class Camera;
class Light;
class DirectionalLight;
class PointLight;
class PlayerController;
class RollerCoaster;

class VertexArrayObject;

class Texture;
class Texture2D;
class Cubemap;

struct Shape;


const float EPSILON = 0.000001f; // epsilon used for comparing vec3
const int RESTARTINDEX = -1; // index for glPrimitiveRestartIndex
const vec3 worldForward = vec3(0, 0, -1); // world forward vector
const vec3 worldRight = vec3(1, 0, 0); // world right vector
const vec3 worldUp = vec3(0, 1, 0); // world up vector

// Compare two vec3
bool approximately(vec3 a, vec3 b); 

// Get unit projection vector from u on v
vec3 getProjectionOnVector(vec3 u, vec3 v); 
// Get unit projection vector from u on a plane
vec3 getProjectionOnPlane(vec3 u, vec3 planeNormal = vec3(0, 1, 0)); 
// Extract float array from mat4
void extractMatrix(mat4 matrix, float* m); 
// Extract position from mat4
vec3 extractPosition(mat4 m);
// Extract rotation from mat4
quat extractRotation(mat4 m); 
// Extract scale from mat4
vec3 extractScale(mat4 m);
// Get current directory
string getCurrentDirectory();

// Generate data of shapes
Shape makePlane(float width = 1.0f, float length = 1.0f);
Shape makeCube(float width = 1.0f, float length = 1.0f, float height = 1.0f);
Shape makeSphere(float radius = 0.5f, int resolution = 50);
Shape makeCylinder(float radius = 0.5f, float height = 1.0f, int resolution = 50);
Shape makeTetrahedron(float width = 1.0f, float height = 1.0f);

// Get type of a class
template<class T> string getType(); 
// Get type of an object
template<class T> string getType(T obj); 
// Check if obj is derived from Base class
//template <class Base, class Target> bool isType(const Target* obj);

// Debug log vec3
void log(vec3 v, bool endOfLine = true); 
// Debug log quat
void log(quat q, bool endOfLine = true); 

// Shape data
struct Shape {
	vector<vec3> positions;
	vector<int> indices;
	vector<vec3> normals;
	vector<vec2> texCoords;
	GLenum drawMode;

	Shape(
		vector<vec3> positions,
		vector<int> indices,
		vector<vec3> normals,
		vector<vec2> texCoords,
		GLenum drawMode = GL_POINTS);
}; 


// Base class for singletons
template<class T>
class Singleton {
protected:
	static inline T* instance = nullptr;

	Singleton() noexcept = default;
	Singleton(const Singleton&) = delete;
	Singleton& operator=(const Singleton&) = delete;
	virtual ~Singleton() = default;

public:
	static T* getInstance() {
		if (!instance) {
			instance = new T();
		}
		return instance;
	}
};

// Manage time
class Timer : public Singleton<Timer> {
public:
	// Get time between this and previous frame
	float getDeltaTime();
	// Set time of current frame
	void setCurrentTime(int curr);

private:
	int previousTime = 0; // in ms
	float deltaTime = 0; // in s
};


// Manage entities
class SceneManager : public Singleton<SceneManager> {
public:
	friend Light;

	bool isLightingEnabled = false;

	// Create a new entity
	Entity* createEntity(string name = "");
	// Create the skybox
	Entity* createSkybox(BasicPipelineProgram* pipeline, Texture* texture);
	// Create a pipeline
	BasicPipelineProgram* createPipelineProgram(string shaderPath);
	// Update all entities. Called once per frame.
	void update();

private:
	vector<BasicPipelineProgram*> pipelinePrograms; // pipeline programs
	vector<Entity*> entities; // entities
	Entity* skybox = nullptr; // skybox
};

// Basic entity
class Entity {
public:
	friend SceneManager;

	Transform* transform = nullptr; // entity transform
	string name; // entity name

	// Check if this is activated
	bool isActive();
	// Activate or deactivate this
	void setActive(bool isActive);
	// Get model matrix
	mat4 getModelMatrix();
	// Get parent entity
	Entity* getParent();
	// Set parent entity
	void setParent(Entity* parent);
	// Get child entities
	vector<Entity*> getChildren();

	// Add a component
	template<class T> bool addComponent(T* component);
	// Get a component
	template<class T> T* getComponent();
	// Check if a component of class T already exists
	template<class T> bool containsComponent();

protected:
	bool isActivated = true; // is this entity active
	map<string, Component*> typeToComponent; // keys of component class type to added components
	Entity* parent = nullptr; // parent entity
	vector<Entity*> children; // child entities

	Entity(string name);

	// Called once per frame
	virtual void update();
	// Generate a key of component class type
	string toClassKey(string type);
};

// Added to entities for functionalities
class Component {
public:
	friend Entity;

	// Get owner entity
	Entity* getEntity();
	// Check if this is activated
	bool isActive();
	// Activate or deactivate this
	virtual void setActive(bool isActive);

protected:
	Entity* entity = nullptr; // owner entity
	bool isActivated = true; // is this active

	Component();

	// Perform update
	virtual void onUpdate() = 0;

private:
	// Update this if this is active
	void update();
};

// Every entity has one transform
class Transform : Component {
public:
	friend SceneManager;
	friend Entity;

	// Get position
	vec3 getPosition(bool isWorld);
	// Get rotation
	quat getRotation(bool isWorld);
	// Get scale
	vec3 getScale(bool isWorld);
	// Set position
	void setPosition(vec3 pos, bool isWorld);
	// Set rotation
	void setRotation(quat rotation, bool isWorld);
	// Set scale
	void setScale(vec3 scale, bool isWorld);
	// Get rotation in euler angles (degrees)
	vec3 getEulerAngles(bool isWorld);
	// Set rotation from euler angles (degrees)
	void setEulerAngles(vec3 angles, bool isWorld);
	// Rotate around an axis (degrees)
	void rotateAround(float degree, vec3 axis, bool isWorld);
	// Rotate around an axis of a pivot (degrees)
	void rotateAround(vec3 pivot, float degree, vec3 axis);
	// Rotate to a target position
	void faceTo(vec3 target, vec3 up = vec3(0, 1, 0));
	// Get local forward vector
	vec3 getForwardVector(bool isWorld);
	// Get local right vector
	vec3 getRightVector(bool isWorld);
	// Get local up vector
	vec3 getUpVector(bool isWorld);

private:
	vec3 position = vec3(0); // local position
	quat rotation = quat(1, 0, 0, 0); // local rotation
	vec3 scale = vec3(1); // local scale
	mat4 modelMatrix = mat4(1); // model matrix in world

	Transform();

	// Get parent's model matrix
	mat4 getParentMatrix();
	// Update this and child entities' model matrices
	void updateModelMatrix();
	void onUpdate() override;
};

class Texture {
public:
	// Get texture handle
	GLuint getHandle();
	// Load this texture for use
	virtual void load(BasicPipelineProgram* pipeline);
protected:
	GLuint handle; // texture handle
	GLenum type; // texture type
	int textureTypeId = 0; // 0: 2d, 1: cube

	Texture(GLuint t, int id) : type(t), textureTypeId(id) {}
};
class Texture2D : public Texture {
public:
	Texture2D(string imageName);
};
class Cubemap : public Texture {
public:
	Cubemap(string imageDirectory);
};

// Entity renderer
class Renderer : public Component {
public:
	friend Entity;

	// Get all renderers
	static vector<Renderer*> getRenderers();

	bool isSkyBox = false; // is this rendering a skybox
	VertexArrayObject* vao = nullptr; // used for rendering
	GLenum drawMode = GL_POINTS; // draw mode

	bool useLight = true; // is this rendered with lights
	vec4 ambient = vec4(1, 1, 1, 1); // ambient coefficient
	vec4 diffuse = vec4(1, 1, 1, 1); // diffuse coefficient
	vec4 specular = vec4(.9, .9, .9, 1); // specular coefficient
	float shininess = 0; // shininess coefficient

	Renderer(VertexArrayObject* vao, GLenum drawMode);
	Renderer(BasicPipelineProgram* pipelineProgram, Shape shape, vec4 color = vec4(255));

	// Set 2d texture
	void setTexture(Texture* texture);
	// Render
	void render();

protected:
	static inline vector<Renderer*> renderers; // renderers
	Texture* texture = nullptr;

	void onUpdate() override;
};

// Simple physics
class Physics : public Component {
public:
	friend Entity;

	static inline const vec3 GRAVITY = vec3(0, -9.8f, 0); // gravity vector
	static inline const float GROUND_Y = 0.0f; // world ground y-axis
	vec3 velocity = vec3(0); // current velocity
	bool checkGround = true; // check if ground y-axis is reached when performing physics
	bool isOnGround = false; // is ground reached
	float minDistance = 0; // min distance limit to ground y-axis

	Physics(bool checkGround = true, float minDistance = 0.0f);

protected:
	void onUpdate() override;
};

// Camera
class Camera : public Component {
public:
	friend Entity;

	float fieldOfView = 60.0f; // in degrees
	float aspect = 1920 / 1080.0f; // width / height
	float zNear = 0.1f;
	float zFar = 3000.0f;

	static inline Camera* currentCamera = nullptr; // current camera being used

	Camera(bool setCurrent = true);

	// Get float array from the projection matrix
	void getProjectionMatrix(float* m);
	// Get float array from the view matrix
	void getViewMatrix(float* m);
	// Set up perspective
	void setPerspective(float fieldOfView, float aspect, float zNear, float zFar);
	// Set this as current camera
	void setCurrent();
	// Check if this is the current camera
	bool isCurrentCamera();
	// Update camera view
	void view();

protected:
	mat4 projectionMatrix = mat4(1);
	mat4 viewMatrix = mat4(1);

	void onUpdate() override;
};

// Light
class Light : public Component {
public:
	vec4 ambient = vec4(0, 0, 0, 1); // ambient color
	vec4 diffuse = vec4(0.9, 0.9, 0.9, 1); // diffuse color
	vec4 specular = vec4(1, 1, 1, 1); // specular color

	// Get directional light
	Light* getDirectionalLight();
	// Get point lights
	vector<PointLight*> getPointLights();
	// Send data to shader
	static void sendData(GLuint pipelineHandle);

protected:
	Light() {};
	static inline DirectionalLight* directionalLight = nullptr; // only one directional light
	static inline vector<PointLight*> pointLights; // multiple point lights

	void onUpdate() override;
};
class DirectionalLight : public Light {
public:
	vec3 direction; // direction for directional light

	DirectionalLight(vec3 direction = vec3(1, -1, -1));
};
class PointLight : public Light {
public:
	vec3 attenuation; // attenuation coefficients a, b, c

	PointLight(vec3 attenuation = vec3(1, 0.1f, 0.005f));
};

// First-person player controller
class PlayerController : public Component {
public:
	friend Entity;

	float speed = 20.0f;

	void move(vec4 input, float verticalMove);
	// Move horizontally
	void moveOnGround(vec4 input);

	PlayerController();

protected:
	void onUpdate() override;
};

// Handles VAO related operations
// Used in renderer
class VertexArrayObject {
public:
	BasicPipelineProgram* pipelineProgram; 

	VertexArrayObject(BasicPipelineProgram* pipelineProgram);
	VertexArrayObject(BasicPipelineProgram* pipelineProgram, vector<vec3> positions, vector<vec4> colors, vector<int> indices);

	// Bind pipeline
	void bindPipeline();
	// Set data
	void setPositions(vector<vec3> positions);
	void setColors(vector<vec4> colors);
	void setIndices(vector<int> indices);
	void setNormals(vector<vec3> normals);
	void setTexCoords(vector<vec2> texCoords);
	// Use model view matrix, projection matrix and draw mode to draw
	void draw(float* m, float* v, float* p, float* n, GLenum drawMode);
	// Send data to shaders
	template<class T> void sendData(vector<T> data, int size, string name);

private:
	GLuint positionBuffer, colorBuffer, indexBuffer, normalBuffer, texCoordBuffer;
	GLuint vertexArray;
	int numVertices, numColors, numIndices, numNormals, numTexCoords;
};

// spline struct 
// contains how many control points the spline has, and an array of control points 
struct Spline {
	int numControlPoints = 0;
	vector<vec3> points;
};

// Based on catmull-rom spline
class RollerCoaster : Component {
public:
	friend Entity;

	static inline const float S = 0.5f;
	static inline const mat4 BASIS =
		mat4(
			-S, 2 * S, -S, 0,
			2 - S, S - 3, 0, 1,
			S - 2, 3 - 2 * S, S, 0,
			S, -S, 0, 0
		);
	Spline spline;
	float startSpeed = 10.0f;
	float minSpeed = 5.0f;
	float speed = startSpeed;
	Entity* seat = nullptr; // to be moved by coaster

	RollerCoaster(vector<Spline> splines, bool closedPath, float scale = 10.0f, float maxLineLength = 0.1f);

	// Get start position
	vec3 getStartPosition();
	// Get current point position
	vec3 getCurrentPosition();
	// Get current point forward direction
	vec3 getCurrentDirection();
	// Get current point normal
	vec3 getCurrentNormal();
	// Start the coaster
	void start(bool isRepeating = true);
	// Pause the coaster
	void pause();
	// Reset the coaster
	void reset(bool resetSpeed);
	// Generate the coaster from given spline data
	void render(vec3 normal, vec4 crossbarColor, vec4 trackColor, vec4 saddleColor, vec4 backColor);

protected:
	float maxLineLength = 0.01f; // line length for subdivision
	int startClosingIndex = -1; // point index where path starts to get closing when closedPath == true
	bool closedPath = true; // is the rail path closed
	bool isRepeating = false; // is repeating after finished
	// vertex data
	vector<vec3> vertexPositions;
	vector<vec3> vertexNormals;
	vector<vec3> vertexTangents;
	vector<float> vertexDistances;
	int currentVertexIndex = -1;
	float currentSegmentProgress = 0; // progress between current and next vertex
	int numOfVertices = 0;

	// Move seat to current vertex position
	void moveSeat(); 
	// Calculate spline data by subdivision
	void subdivide(float u0, float u1, float maxLength, mat3x4 control);
	// Generate a track
	void makeTrack(float width, float height, float offset, vec4 color);

	void onUpdate() override;
};



#pragma region Templates
template<class T>
string getType() {
	return typeid(T).name();
}
template<class T>
string getType(T obj) {
	return typeid(obj).name();
}
template <class Base, class Target>
bool isType(const Target* obj) {
	return is_base_of<Base, Target>::value;
}

template<class T>
bool Entity::addComponent(T* component) {
	string type = toClassKey(getType(component));
	if (!is_base_of<Component, T>::value || is_same<Component, T>::value) {
		printf("Error: type %s is not derived from Component. \n", type.c_str());
		return false;
	}
	if (containsComponent<decltype(component)>()) {
		printf("Error: already contains a component of type %s. \n", type.c_str());
		return false;
	}

	component->entity = this;
	typeToComponent.insert({ type, component });
	return true;
}
template<class T>
T* Entity::getComponent() {
	if (!containsComponent<T>()) {
		return nullptr;
	}
	else {
		string type = toClassKey(getType<T>());
		return (T*)typeToComponent[type];
	}
}
template<class T>
bool Entity::containsComponent() {
	//for (auto const& c : typeToComponent) {
	//	cout << isType<T>(c.second) << endl;
	//	if (isType<T>(c.second)) {
	//		return true;
	//	}
	//}
	//return false;
	string type = toClassKey(getType<T>());
	if (typeToComponent.find(type) == typeToComponent.end()) {
		return false;
	}
	return true;
}

template<class T> 
void VertexArrayObject::sendData(vector<T> data, int size, string name) {
	GLuint buffer;
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);  // bind the VBO buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(T) * data.size(), &data[0], GL_STATIC_DRAW);

	GLuint loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), name.c_str());
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc, size, GL_FLOAT, GL_FALSE, 0, (const void*)0);
}
#pragma endregion

#endif