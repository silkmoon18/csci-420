#ifndef _UTILITY_H_
#define _UTILITY_H_

#define GLM_FORCE_RADIANS

#include "glm/gtx/transform.hpp"
#include "glm/gtx/euler_angles.hpp"
#include "glm/gtx/matrix_decompose.hpp"
#include "basicPipelineProgram.h"
#include "openGLMatrix.h"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/epsilon.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <vector>
#include <map>
#include <algorithm>
#include <iostream>

using namespace std;
using namespace glm;

template<class T> class Singleton;
class Timer;
class EntityManager;
class Entity;

class Component;
class Transform;
class Renderer;
class Camera;
class VertexArrayObject;


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

// Get type of a class
template<class T> string getType();
// Get type of an object
template<class T> string getType(T obj);

// Debug log vec3
void log(vec3 v, bool endOfLine = true);
// Debug log quat
void log(quat q, bool endOfLine = true);


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
class EntityManager : public Singleton<EntityManager> {
public:
	vector<Entity*> entities; // all entities

	// Update all entities. Called once per frame.
	void update();
	// Create a new entity
	Entity* createEntity(string name = "");
};

// Basic entity
class Entity {
public:
	friend EntityManager;

	Transform* transform = nullptr; // entity transform
	string name; // entity name

	// Check if this is activated
	bool isActive();
	// Activate or deactivate this
	void setActive(bool isActive);
	// Get float array from the model matrix
	void getModelMatrix(float m[16]);
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
	bool isActivated = true; // is this active
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
	friend EntityManager;
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

// Entity renderer
class Renderer : public Component {
public:
	friend Entity;

	// Preset shapes
	enum Shape { Cube, Sphere, Cylinder };
	GLenum drawMode = GL_POINTS; // draw mode

	Renderer(GLenum drawMode);
	Renderer(VertexArrayObject* vao, GLenum drawMode);
	Renderer(BasicPipelineProgram* pipelineProgram, Shape shape, vec4 color = vec4(255));

	void setVAO(VertexArrayObject* vao);

protected:
	VertexArrayObject* vao = nullptr; // used for rendering

	void onUpdate() override;
};

// Camera
class Camera : public Component {
public:
	friend Entity;

	float fieldOfView = 60.0f; // in degrees
	float aspect = 1920 / 1080.0f; // width / height
	float zNear = 0.01f;
	float zFar = 1000.0f;

	static Camera* currentCamera; // current camera being used

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

protected:
	mat4 projectionMatrix = mat4(1);
	mat4 viewMatrix = mat4(1);

	void onUpdate() override;
};


// Handles VAO related operations
// Used in renderer
class VertexArrayObject {
public:
	BasicPipelineProgram* pipelineProgram;

	VertexArrayObject(BasicPipelineProgram* pipelineProgram);
	VertexArrayObject(BasicPipelineProgram* pipelineProgram, vector<vec3> positions, vector<vec4> colors, vector<int> indices);

	// Set vertex positions, colors and indices
	void setVertices(vector<vec3> positions, vector<vec4> colors, vector<int> indices);
	// Use model view matrix, projection matrix and draw mode to draw
	void draw(float* mv, float* p, GLenum drawMode);
	// Send data to shaders
	template<class T> void sendData(vector<T> data, int size, string name);

private:
	GLuint positionBuffer, colorBuffer, indexBuffer;
	GLuint vertexArray;
	int numVertices, numColors, numIndices;
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