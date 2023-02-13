#ifndef _UTILITY_H_
#define _UTILITY_H_

#include "basicPipelineProgram.h"
#include "openGLMatrix.h"

#include <vector>

using namespace std;
using namespace glm;

class EntityManager;
class Entity;
class Camera;
class VertexArrayObject;
class SplineObject;



static const float PI = 3.14159265359f;
static float degreeToRadian(float degree);
static float radianToDegree(float radian);

// manager for all entities
class EntityManager {
public:
	vector<Entity*> entities;

	EntityManager();

	void update();
	Entity* createEntity();
	Entity* createEntity(VertexArrayObject* vao);
	Camera* createCamera();
};


struct Transform {
	vec3 position;
	vec3 rotation;
	vec3 scale;
	Transform() {
		position = vec3(0);
		rotation = vec3(0);
		scale = vec3(1);
	}
};


// basic entity
class Entity {
public:
	Transform transform;

	void translate(float x, float y, float z);
	void rotate(float xDegree, float yDegree, float zDegree); // in degrees
	void scale(float x, float y, float z);
	void lookAt(vec3 target, vec3 up);
	vec3 getForwardVector();
	vec3 getRightVector();
	vec3 getUpVector();

	friend EntityManager;

protected:
	OpenGLMatrix modelViewMatrix;
	VertexArrayObject* vao = nullptr;

	Entity();
	Entity(VertexArrayObject* vao);

	void initMatrix(vec3 eye, vec3 center, vec3 up);
	virtual void update();
};


// camera entity
class Camera : public Entity {
public:
	float fieldOfView = 80.0f; // in degrees
	float aspect = 1920 / 1080.0f; // width / height
	float zNear = 0.01f;
	float zFar = 1000.0f;

	static Camera* currentCamera;

	void getProjectionMatrix(float* pMatrix);
	void setPerspective(float fieldOfView, float aspect, float zNear, float zFar);
	void enable();
	bool isCurrentCamera();

	friend EntityManager;

protected:
	OpenGLMatrix projectionMatrix;

	Camera();

	void update();
};


// object that handles VAO related operations
class VertexArrayObject {
public:
	BasicPipelineProgram* pipelineProgram;
	GLuint positionBuffer, colorBuffer, indexBuffer;
	GLuint vertexArray;
	GLenum drawMode = GL_POINTS;
	int numVertices, numColors, numIndices;

	VertexArrayObject(BasicPipelineProgram* pipelineProgram, vector<vec3> positions, vector<vec4> colors, vector<int> indices, GLenum drawMode);

	void draw(OpenGLMatrix modelViewMatrix);
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


#endif