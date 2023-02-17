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

template<class T> class Singleton;
class Timer;
class EntityManager;
class Entity;

class Component;
class Physics;
class Camera;
class VertexArrayObject;
class SplineData;


const float PI = 3.14159265359f;
const float EPSILON = 0.000001f;
const int RESTARTINDEX = -1;
const vec3 worldForward = vec3(0, 0, -1);
const vec3 worldRight = vec3(1, 0, 0);
const vec3 worldUp = vec3(0, 1, 0);

bool approximately(vec3 a, vec3 b);

vec3 getProjectionOnVector(vec3 u, vec3 v);
vec3 getProjectionOnPlane(vec3 u, vec3 planeNormal = vec3(0, 1, 0));

template<class T> string getType();
template<class T> string getType(T obj);

void log(vec3 v, bool endOfLine = true);
void log(quat q, bool endOfLine = true);



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

class Timer : public Singleton<Timer> {
public:
	float getDeltaTime();
	void setCurrentTime(int curr);

private:
	int previousTime = 0; // in ms
	float deltaTime = 0; // in s
};


// manager for all entities
class EntityManager : public Singleton<EntityManager> {
public:
	vector<Entity*> entities;

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
	friend EntityManager;

	Transform* transform = nullptr;

	void getModelViewMatrix(float m[16]);
	void faceTo(vec3 target, vec3 up = vec3(0, 1, 0));
	void rotateAround(float degree, vec3 axis);
	vec3 getForwardVector();
	vec3 getRightVector();
	vec3 getUpVector();

	template<class T> bool addComponent(T* component);
	template<class T> T* getComponent();
	template<class T> bool containsComponent();

protected:
	OpenGLMatrix modelViewMatrix;
	map<string, Component*> typeToComponent;

	Entity();

	virtual void initMatrix(vec3 eye, vec3 center, vec3 up);
	virtual void update();
	string toClassKey(string type);
};


class Component {
public:
	friend Entity;

	Entity* getEntity();
	virtual void setActive(bool isActive);

protected:
	Entity* entity = nullptr;
	bool isActive = true;

	Component();

	virtual void onUpdate() = 0;

private:
	void update();
};

class Renderer : public Component {
public:
	friend Entity;

	enum Shape { Cube, Sphere, Cylinder };
	VertexArrayObject* vao = nullptr;
	GLenum drawMode = GL_POINTS;

	Renderer(VertexArrayObject* vao);
	Renderer(BasicPipelineProgram* pipelineProgram, Shape shape, vec4 color = vec4(255));

protected:
	void onUpdate() override;
};

// simple physics
class Physics : public Component {
public:
	friend Entity;

	static inline const vec3 GRAVITY = vec3(0, -9.8f, 0);
	static inline const float GROUND_Y = 0.0f;
	vec3 velocity = vec3(0);
	bool checkGround = true;
	bool isOnGround = false;
	float minDistance = 0;

	Physics(float minDistance, bool checkGround = true);

protected:
	void onUpdate() override;
};


// camera entity
class Camera : public Component {
public:
	friend Entity;

	float fieldOfView = 60.0f; // in degrees
	float aspect = 1920 / 1080.0f; // width / height
	float zNear = 0.01f;
	float zFar = 1000.0f;

	static Camera* currentCamera;

	Camera(bool setCurrent = true);

	void getProjectionMatrix(float* pMatrix);
	void setPerspective(float fieldOfView, float aspect, float zNear, float zFar);
	void setCurrent();
	bool isCurrentCamera();

protected:
	OpenGLMatrix projectionMatrix;

	void onUpdate() override;
};

class PlayerController : public Component {
public:
	friend Entity;

	void moveOnGround(vec4 input, float step);

	PlayerController();

protected:
	void onUpdate() override;
};


// object that handles VAO related operations
class VertexArrayObject {
public:
	BasicPipelineProgram* pipelineProgram;
	GLuint positionBuffer, colorBuffer, indexBuffer;
	GLuint vertexArray;
	int numVertices, numColors, numIndices;

	VertexArrayObject(BasicPipelineProgram* pipelineProgram);
	VertexArrayObject(BasicPipelineProgram* pipelineProgram, vector<vec3> positions, vector<vec4> colors, vector<int> indices);

	void setVertices(vector<vec3> positions, vector<vec4> colors, vector<int> indices);
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

// based on catmull-rom spline
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
	vector<vec3> vertexPositions;
	vector<vec3> vertexNormals;
	vector<vec3> vertexTangents;
	vector<float> vertexDistances;
	int currentVertexIndex = -1;
	float currentSegmentProgress = 0;
	int numOfVertices = 0;
	float size = 0.5f;
	float speed = 5.0f;

	RollerCoaster(Spline spline, int numOfStepsPerSegment = 1000);

	vec3 getDirection();
	vec3 getNormal();
	void perform(Entity* target);
	void render();

protected:
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