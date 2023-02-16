#ifndef _UTILITY_H_
#define _UTILITY_H_

#define GLM_FORCE_RADIANS

#include "basicPipelineProgram.h"
#include "openGLMatrix.h"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/epsilon.hpp"

#include <vector>
#include <map>
#include <algorithm>


using namespace std;
using namespace glm;


class EntityManager;
class Entity;

class Component;
class Camera;
class VertexArrayObject;
class SplineObject;


const float PI = 3.14159274101257324219f;
const float EPSILON = 0.000001f;
const vec3 worldForward = vec3(0, 0, -1);
const vec3 worldRight = vec3(1, 0, 0);
const vec3 worldUp = vec3(0, 1, 0);

bool approximately(vec3 a, vec3 b);
float degreeToRadian(float degree);
vec3 degreeToRadian(vec3 degrees);
float radianToDegree(float radian);
vec3 radianToDegree(vec3 radians);

vec3 getProjectionOnVector(vec3 u, vec3 v);
vec3 getProjectionOnPlane(vec3 u, vec3 planeNormal = vec3(0, 1, 0));

template<class T> string getType();
template<class T> string getType(T obj);

void log(vec3 v, bool endOfLine = true);
void log(quat q, bool endOfLine = true);

// manager for all entities
class EntityManager {
public:
	vector<Entity*> entities;

	EntityManager();

	void update();
	Entity* createEntity();
};


class Transform {
public:
	vec3 position;
	quat rotation;
	vec3 scale;

	Transform();

	vec3 getEulerAngles();
	void setEulerAngles(vec3 angles);
};


// basic entity
class Entity {
public:
	Transform* transform;

	void getModelViewMatrix(float m[16]);
	void faceTo(vec3 target, vec3 up = vec3(0, 1, 0)); 
	void rotateAround(float degree, vec3 axis);
	vec3 getForwardVector();
	vec3 getRightVector();
	vec3 getUpVector(); 

	template<class T> bool addComponent(T* component);
	template<class T> T* getComponent();
	template<class T> bool containsComponent();

	friend EntityManager;

protected:
	OpenGLMatrix modelViewMatrix;
	VertexArrayObject* vao = nullptr;
	map<string, Component*> typeToComponent;

	Entity();

	virtual void initMatrix(vec3 eye, vec3 center, vec3 up);
	virtual void update();
	string toClassKey(string type);
};


class Component {
public:
	Entity* getEntity();

	friend Entity;

protected:
	Entity* entity;

	Component();

	virtual void update() = 0;
};

// camera entity
class Camera : public Component {
public:
	float fieldOfView = 80.0f; // in degrees
	float aspect = 1920 / 1080.0f; // width / height
	float zNear = 0.01f;
	float zFar = 1000.0f;

	static Camera* currentCamera;

	Camera(bool setCurrent = true);

	void getProjectionMatrix(float* pMatrix);
	void setPerspective(float fieldOfView, float aspect, float zNear, float zFar);
	void enable();
	bool isCurrentCamera();

	friend Entity;

protected:
	string type = "Camera";
	OpenGLMatrix projectionMatrix;

	void update() override;
};

class PlayerController : public Component {
public:
	void moveOnGround(vec4 input, float step);

	PlayerController();

	friend Entity;

protected:
	string type = "PlayerController";

	void update() override;
};


// object that handles VAO related operations
class VertexArrayObject : public Component {
public:
	BasicPipelineProgram* pipelineProgram;
	GLuint positionBuffer, colorBuffer, indexBuffer;
	GLuint vertexArray;
	GLenum drawMode = GL_POINTS;
	int numVertices, numColors, numIndices;

	VertexArrayObject(BasicPipelineProgram* pipelineProgram, vector<vec3> positions, vector<vec4> colors, vector<int> indices, GLenum drawMode);

	void update() override;
};


// represents one control point along the spline 
struct Point {
	double x;
	double y;
	double z;
};

// spline struct 
// contains how many control points the spline has, and an array of control points 
struct Spline {
	int numControlPoints;
	Point* points;
};
class SplineObject {
public:
	Spline spline;
	vector<vec3> vertexPositions;
	vector<vec3> vertexTangents;
	vector<float> vertexDistances;
	int currentVertexIndex = -1;
	float currentSegmentProgress = 0;
	int numOfVertices = 0;

	SplineObject(Spline spline, vector<vec3> vertexPositions, vector<vec3> vertexTangents);

	vec3 getDirection();
	vec3 moveForward(float step);
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
#pragma endregion

#endif