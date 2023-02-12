#ifndef _UTILITY_H_
#define _UTILITY_H_

#include "basicPipelineProgram.h"
#include "openGLMatrix.h"

#include <vector>

using namespace std;
using namespace glm;

class EntityManager;
class Entity;
class SimpleVertexArrayObject;
class SplineObject;

/// <summary>
/// Entity
/// </summary>

class EntityManager {
public:
	vector<Entity*> objects;

	EntityManager();

	void update();
	Entity* createEntity();
	Entity* createEntity(SimpleVertexArrayObject* vao);
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

class Entity {
public:
	Transform transform;

	void translate(float x, float y, float z);
	void rotate(float xDegree, float yDegree, float zDegree); // in degrees
	void scale(float x, float y, float z);
	void lookAt(vec3 target, vec3 up);

	friend EntityManager;

protected:
	OpenGLMatrix matrix;
	SimpleVertexArrayObject* vao;

	Entity();
	Entity(SimpleVertexArrayObject* vao);

	virtual void update();
};

class Camera : public Entity {
public:
	float fieldOfView; // in degrees
	float aspect; // width / height
	float zNear;
	float zFar;

	void setPerspective(float fieldOfView, float aspect, float zNear, float zFar);

protected:
	void update() override;
};


// a simple object that handles VAO related operations
class SimpleVertexArrayObject {
public:
	BasicPipelineProgram* pipelineProgram;
	GLuint positionBuffer, colorBuffer, indexBuffer;
	GLuint vertexArray;
	GLenum drawMode = GL_POINTS;
	int numVertices, numColors, numIndices;

	SimpleVertexArrayObject(BasicPipelineProgram* pipelineProgram, vector<vec3> positions, vector<vec4> colors, vector<int> indices, GLenum drawMode);

	void draw();
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
	int numOfVertices;

	SplineObject(Spline spline, vector<vec3> vertexPositions, vector<vec3> vertexTangents);

	vec3 getDirection();
	vec3 moveForward(float step);
};


#endif