#include "Utility.h"

SimpleVertexArrayObject::SimpleVertexArrayObject(BasicPipelineProgram* pipelineProgram, vector<vec3> positions, vector<vec4> colors, vector<int> indices, GLenum drawMode) {
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

	this->pipelineProgram = pipelineProgram;
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

void SimpleVertexArrayObject::draw() {
	pipelineProgram->Bind();
	glBindVertexArray(vertexArray);
	glDrawElements(drawMode, numIndices, GL_UNSIGNED_INT, 0);
}



SplineObject::SplineObject(Spline spline, vector<vec3> vertexPositions, vector<vec3> vertexTangents) {
	this->spline = spline;
	this->vertexPositions = vertexPositions;
	this->vertexTangents = vertexTangents;

	currentVertexIndex = 0;
	numOfVertices = vertexPositions.size();

	for (int i = 0; i < vertexPositions.size() - 1; i++) {
		float distanceToNext = distance(vertexPositions[i], vertexPositions[i + 1]);
		vertexDistances.push_back(distanceToNext);
	}
	vertexDistances.push_back(-1);
}

vec3 SplineObject::getDirection() {
	return vertexTangents[currentVertexIndex];
}

vec3 SplineObject::moveForward(float step) {
	if (currentVertexIndex == numOfVertices - 1) {
		return vertexPositions[currentVertexIndex];
	}

	// consume step
	while (step > 0) {
		float distanceToNext = vertexDistances[currentVertexIndex] * (1 - currentSegmentProgress);
		if (step < distanceToNext) break;

		step -= distanceToNext;
		// move to the next vertex and reset distance from current vertex
		currentSegmentProgress = 0;
		currentVertexIndex++;

		// if reach the end, return the end point position
		if (currentVertexIndex == numOfVertices - 1) {
			return vertexPositions[currentVertexIndex];
		}
	}

	currentSegmentProgress += step / vertexDistances[currentVertexIndex];
	return mix(vertexPositions[currentVertexIndex], vertexPositions[currentVertexIndex + 1], currentSegmentProgress);
}



Object::Object(SimpleVertexArrayObject* vao) {
	this->vao = vao;
}

void Object::translate(float x, float y, float z) {
	matrix.SetMatrixMode(OpenGLMatrix::ModelView);
	transform.position += vec3(x, y, z);
}

void Object::rotate(float xDegree, float yDegree, float zDegree) {
	matrix.SetMatrixMode(OpenGLMatrix::ModelView);
	transform.rotation += vec3(xDegree, yDegree, zDegree);
	fmod(transform.rotation.x, 360);
	fmod(transform.rotation.y, 360);
	fmod(transform.rotation.z, 360);
}

void Object::scale(float x, float y, float z) {
	matrix.SetMatrixMode(OpenGLMatrix::ModelView);
	transform.scale = vec3(x, y, z);
}


void Object::update() {
	matrix.SetMatrixMode(OpenGLMatrix::ModelView);
	matrix.LoadIdentity();
	matrix.Translate(transform.position.x, transform.position.y, transform.position.z);
	matrix.Rotate(transform.rotation.x, 1, 0, 0);
	matrix.Rotate(transform.rotation.y, 0, 1, 0);
	matrix.Rotate(transform.rotation.z, 0, 0, 1);
	matrix.Scale(transform.scale.x, transform.scale.y, transform.scale.z);
}