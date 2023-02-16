#include "Utility.h"







bool approximately(vec3 a, vec3 b) {
	for (int i = 0; i < a.length(); i++) {
		float diff = abs(a[i] - b[i]);
		if (diff > EPSILON) {
			return false;
		}
	}
	return true;
}
float degreeToRadian(float degree) {
	return degree * PI / 180.0f;
}
vec3 degreeToRadian(vec3 degrees) {
	return vec3(degreeToRadian(degrees.x), degreeToRadian(degrees.y), degreeToRadian(degrees.z));
}
float radianToDegree(float radian) {
	return radian * (180.0f / PI);
}
vec3 radianToDegree(vec3 radians) {
	return vec3(radianToDegree(radians.x), radianToDegree(radians.y), radianToDegree(radians.z));
}
vec3 getProjectionOnVector(vec3 u, vec3 v) {
	float vLength = length(v);
	return v * dot(u, v) / (vLength * vLength);
}
vec3 getProjectionOnPlane(vec3 u, vec3 planeNormal) {
	planeNormal = normalize(planeNormal);
	return u - getProjectionOnVector(u, planeNormal);
}
void log(vec3 v, bool endOfLine) {
	string newLine = endOfLine ? "\n" : "";
	printf("vec3(%f, %f, %f)%s", v.x, v.y, v.z, newLine.c_str());
}
void log(quat q, bool endOfLine) {
	string newLine = endOfLine ? "\n" : "";
	printf("quat(%f, %f, %f, %f)%s", q.x, q.y, q.z, q.w, newLine.c_str());
}


#pragma region EntityManager
EntityManager::EntityManager() {

}
void EntityManager::update() {
	Entity* camera = Camera::currentCamera->getEntity();
	vec3 position = camera->transform->position;
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


#pragma region Transform
Transform::Transform() {
	position = vec3(0);
	rotation = quat(1, 0, 0, 0);
	scale = vec3(1);
}
vec3 Transform::getEulerAngles() {
	return radianToDegree(eulerAngles(rotation));
}
void Transform::setEulerAngles(vec3 anglesInDegree) {
	anglesInDegree.x = fmod(anglesInDegree.x, 360);
	anglesInDegree.y = fmod(anglesInDegree.y, 360);
	anglesInDegree.z = fmod(anglesInDegree.z, 360);
	rotation = quat(degreeToRadian(anglesInDegree));
}
#pragma endregion


#pragma region Entity 
Entity::Entity() {
	transform = new Transform();
	modelViewMatrix.SetMatrixMode(OpenGLMatrix::ModelView);
}
void Entity::getModelViewMatrix(float m[16]) {
	modelViewMatrix.GetMatrix(m);
}
void Entity::faceTo(vec3 target, vec3 up) {
	if (length(up) == 0) {
		up = worldUp;
	}
	vec3 position = transform->position;
	vec3 direction = normalize(target - transform->position);
	up = normalize(up);
	if (approximately(direction, up)) {
		up = worldUp;
	}
	if (approximately(direction, worldUp)) {
		up = direction.z > 0 ? worldForward : -worldForward;
	}
	vec3 right = cross(direction, up);
	up = cross(right, direction);
	transform->rotation = conjugate(toQuat(lookAt(position, position + direction, up)));

	//vec3 r = radianToDegree(eulerAngles(transform->rotation));
	//log(transform->rotation);
	//log(r);
}
void Entity::rotateAround(float degree, vec3 axis) {
	transform->rotation = rotate(transform->rotation, degreeToRadian(degree), axis);
}
vec3 Entity::getForwardVector() {
	return transform->rotation * worldForward;
}
vec3 Entity::getRightVector() {
	return transform->rotation * worldRight;
}
vec3 Entity::getUpVector() {
	return transform->rotation * worldUp;
}

void Entity::initMatrix(vec3 eye, vec3 center, vec3 up) {
	modelViewMatrix.LoadIdentity();
	modelViewMatrix.LookAt(eye.x, eye.y, eye.z,
						   center.x, center.y, center.z,
						   up.x, up.y, up.z);
}

void Entity::update() {
	// apply transformations
	modelViewMatrix.Translate(transform->position.x, transform->position.y, transform->position.z);
	modelViewMatrix.Rotate(transform->rotation.x, 1, 0, 0);
	modelViewMatrix.Rotate(transform->rotation.y, 0, 1, 0);
	modelViewMatrix.Rotate(transform->rotation.z, 0, 0, 1);
	modelViewMatrix.Scale(transform->scale.x, transform->scale.y, transform->scale.z);

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



PlayerController::PlayerController() {

}
void PlayerController::moveOnGround(vec4 input, float step) {
	float z = input.x - input.y;
	float x = input.w - input.z;
	if (z == 0 && x == 0) return;

	vec3 forward = normalize(getProjectionOnPlane(entity->getForwardVector()));
	vec3 right = normalize(getProjectionOnPlane(entity->getRightVector()));
	vec3 move = normalize(x * right + z * forward) * step;
	entity->transform->position += move;
}
void PlayerController::update() {
	// to-do: physics
}

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



