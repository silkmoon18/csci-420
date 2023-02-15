#include "Utility.h"






float degreeToRadian(float degree) {
	return degree * PI / 180.0f;
}
float radianToDegree(float radian) {
	return radian * (180.0f / PI);
}
void log(vec3 vector, bool endOfLine) {
	string newLine = endOfLine ? "\n" : "";
	printf("vec3(%f, %f, %f)%s", vector.x, vector.y, vector.z, newLine.c_str());
}


#pragma region EntityManager
EntityManager::EntityManager() {

}
void EntityManager::update() {
	Entity* camera = Camera::currentCamera->getEntity();
	vec3 position = camera->transform.position;
	vec3 center = position + camera->getForwardVector();
	vec3 up = camera->getUpVector();

	for (int i = 0; i < entities.size(); i++) {
		Entity* entity = entities[i];

		entity->initMatrix(position, center, up);
		entity->update();
	}
}
Entity* EntityManager::createEntity() {
	Entity* object = new Entity();
	entities.push_back(object);
	return object;
}
#pragma endregion



#pragma region Entity 
Entity::Entity() {
	modelViewMatrix.SetMatrixMode(OpenGLMatrix::ModelView);
}
void Entity::getModelViewMatrix(float m[16]) {
	modelViewMatrix.GetMatrix(m);
}
void Entity::faceTo(vec3 target, vec3 up) {
	vec3 forward = normalize(target - transform.position);
	up = normalize(up);

	float x = asin(forward.y);
	float y = atan2(-forward.x, -forward.z);

	float z = asin(up.z * sin(y) - up.x * cos(y));
	if (up.y < 0)
		z = ((0.0f <= z) - (z < 0.0f)) * PI - z;

	x = radianToDegree(x);
	y = radianToDegree(y);
	z = radianToDegree(z);

	transform.rotation = vec3(x, y, z);
}
vec3 Entity::getForwardVector() {
	vec3 rotation = vec3(degreeToRadian(transform.rotation.x), degreeToRadian(transform.rotation.y), degreeToRadian(transform.rotation.z));
	float x = -sin(rotation.y) * cos(rotation.x);
	float y = sin(rotation.x);
	float z = -cos(rotation.x) * cos(rotation.y);
	return vec3(x, y, z);
}
vec3 Entity::getRightVector() {
	vec3 rotation = vec3(degreeToRadian(transform.rotation.x), degreeToRadian(transform.rotation.y), degreeToRadian(transform.rotation.z));
	float x = sin(rotation.z) * sin(rotation.x) * sin(rotation.y) + cos(rotation.z) * cos(rotation.y);
	float y = cos(rotation.x) * sin(rotation.z);
	float z = sin(rotation.z) * sin(rotation.x) * cos(rotation.y) - cos(rotation.z) * sin(rotation.y);
	return vec3(x, y, z);
}
vec3 Entity::getUpVector() {
	vec3 rotation = vec3(degreeToRadian(transform.rotation.x), degreeToRadian(transform.rotation.y), degreeToRadian(transform.rotation.z));
	float x = cos(rotation.z) * sin(rotation.x) * sin(rotation.y) - sin(rotation.z) * cos(rotation.y);
	float y = cos(rotation.x) * cos(rotation.z);
	float z = cos(rotation.z) * sin(rotation.x) * cos(rotation.y) + sin(rotation.z) * sin(rotation.y);
	return vec3(x, y, z);
}

void Entity::initMatrix(vec3 eye, vec3 center, vec3 up) {
	modelViewMatrix.LoadIdentity();
	modelViewMatrix.LookAt(eye.x, eye.y, eye.z,
						   center.x, center.y, center.z,
						   up.x, up.y, up.z);
}

void Entity::update() {
	// clamp rotation degrees
	fmod(transform.rotation.x, 360);
	fmod(transform.rotation.y, 360);
	fmod(transform.rotation.z, 360);

	// apply transformations
	modelViewMatrix.Translate(transform.position.x, transform.position.y, transform.position.z);
	modelViewMatrix.Rotate(transform.rotation.x, 1, 0, 0);
	modelViewMatrix.Rotate(transform.rotation.y, 0, 1, 0);
	modelViewMatrix.Rotate(transform.rotation.z, 0, 0, 1);
	modelViewMatrix.Scale(transform.scale.x, transform.scale.y, transform.scale.z);

	for (auto const& kvp : typeToComponent) {
		kvp.second->update();
	}
}
string Entity::toClassKey(string type) {
	int index = 0;
	for (int i = 0; type[i]; i++)
		if (type[i] != ' ' || type[i] != '*')
			type[index++] = type[i];
	type[index] = '\0'; 
	type.erase(std::remove_if(type.begin(), type.end(),
							  [](auto const& c) -> bool { return !isalnum(c); }), type.end());

	return type;
}
#pragma endregion



Component::Component() {
	// hided constructor
}
Entity* Component::getEntity() {
	return entity;
}


#pragma region Camera
Camera* Camera::currentCamera = nullptr;

Camera::Camera(bool setCurrent) {
	projectionMatrix.SetMatrixMode(OpenGLMatrix::Projection);
	if (setCurrent) {
		enable();
	}
}
void Camera::getProjectionMatrix(float* pMatrix) {
	projectionMatrix.GetMatrix(pMatrix);
}
void Camera::setPerspective(float fieldOfView, float aspect, float zNear, float zFar) {
	this->fieldOfView = fieldOfView;
	this->aspect = aspect;
	this->zNear = zNear;
	this->zFar = zFar;
	projectionMatrix.LoadIdentity();
	projectionMatrix.Perspective(fieldOfView, aspect, zNear, zFar);
}
void Camera::enable() {
	Camera::currentCamera = this;
}
bool Camera::isCurrentCamera() {
	return Camera::currentCamera == this;
}
void Camera::update() {
	if (!isCurrentCamera()) return;
}
#pragma endregion



#pragma region VertexArrayObject
VertexArrayObject::VertexArrayObject(BasicPipelineProgram* pipelineProgram, vector<vec3> positions, vector<vec4> colors, vector<int> indices, GLenum drawMode) {
	if (!pipelineProgram) {
		printf("error: pipeline program cannot be null. \n");
		return;
	}
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

void VertexArrayObject::update() {
	// get matrices
	float m[16];
	entity->getModelViewMatrix(m);
	float p[16];
	Camera::currentCamera->getProjectionMatrix(p);

	pipelineProgram->Bind();
	// set variable
	pipelineProgram->SetModelViewMatrix(m);
	pipelineProgram->SetProjectionMatrix(p);

	glBindVertexArray(vertexArray);
	glDrawElements(drawMode, numIndices, GL_UNSIGNED_INT, 0);
}
#pragma endregion



#pragma region SplineObject
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
#pragma endregion



