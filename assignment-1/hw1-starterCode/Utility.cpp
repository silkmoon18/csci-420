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
vec3 getProjectionOnVector(vec3 u, vec3 v) {
	float vLength = length(v);
	return v * dot(u, v) / (vLength * vLength);
}
vec3 getProjectionOnPlane(vec3 u, vec3 planeNormal) {
	planeNormal = normalize(planeNormal);
	return u - getProjectionOnVector(u, planeNormal);
}
void extractMatrix(mat4 matrix, float* m) {
	// copied from openGLMatrix.cpp
	memcpy(m, value_ptr(matrix), sizeof(float) * 16);
}
vec3 extractPosition(mat4 m) {
	return vec3(m[3][0], m[3][1], m[3][2]);
}
quat extractRotation(mat4 m) {
	return quat_cast(m);
}
vec3 extractScale(mat4 m) {
	float s1 = sqrt(m[0][0] * m[0][0] + m[0][1] * m[0][1] + m[0][2] * m[0][2]);
	float s2 = sqrt(m[1][0] * m[1][0] + m[1][1] * m[1][1] + m[1][2] * m[1][2]);
	float s3 = sqrt(m[2][0] * m[2][0] + m[2][1] * m[2][1] + m[2][2] * m[2][2]);
	return vec3(s1, s2, s3);
}
void log(vec3 v, bool endOfLine) {
	string newLine = endOfLine ? "\n" : "";
	printf("vec3(%f, %f, %f)%s", v.x, v.y, v.z, newLine.c_str());
}
void log(quat q, bool endOfLine) {
	string newLine = endOfLine ? "\n" : "";
	printf("quat(%f, %f, %f, %f)%s", q.x, q.y, q.z, q.w, newLine.c_str());
}



#pragma region Timer
float Timer::getDeltaTime() {
	return deltaTime;
}
void Timer::setCurrentTime(int currentTime) {
	deltaTime = (currentTime - previousTime) / 1000.0f;
	previousTime = currentTime;
}
#pragma endregion


#pragma region EntityManager
void EntityManager::update() {

	for (int i = 0; i < entities.size(); i++) {
		Entity* entity = entities[i];

		// update root entities only
		if (entity->getParent()) continue;
		entity->update();
	}
}
Entity* EntityManager::createEntity(string name) {
	if (name.empty()) {
		// set default name
		name = "Entity_" + entities.size();
	}
	Entity* entity = new Entity(name);
	entities.push_back(entity);
	return entity;
}
#pragma endregion


#pragma region Transform
Transform::Transform() {
	position = vec3(0);
	rotation = quat(1, 0, 0, 0);
	scale = vec3(1);
	modelMatrix = mat4(1);
}
vec3 Transform::getPosition(bool isWorld) {
	return isWorld ? extractPosition(modelMatrix) : position;
}
quat Transform::getRotation(bool isWorld) {
	return isWorld ? extractRotation(modelMatrix) : rotation;
}
vec3 Transform::getScale(bool isWorld) {
	return isWorld ? extractScale(modelMatrix) : scale;
}
mat4 Transform::getParentMatrix() {
	Entity* parent = entity->getParent();
	if (parent) return parent->transform->modelMatrix;

	// return identity matrix if this is a root entity
	return mat4(1);
}
void Transform::setPosition(vec3 position, bool isWorld) {
	if (!isWorld) {
		this->position = position;
	}
	else {
		// if is world space, inverse previous transforms
		mat4 m = translate(position);
		m = inverse(getParentMatrix()) * m;
		this->position = extractPosition(m);
	}
	updateModelMatrix();
}
void Transform::setRotation(quat rotation, bool isWorld) {
	rotation = normalize(rotation);
	if (!isWorld) {
		this->rotation = rotation;
	}
	else {
		// if is world space, inverse previous transforms
		mat4 m = mat4_cast(rotation);
		m = inverse(getParentMatrix()) * m;
		this->rotation = extractRotation(m);
	}
	updateModelMatrix();
}
void Transform::setScale(vec3 scale, bool isWorld) {
	if (!isWorld) {
		this->scale = scale;
	}
	else {
		// if is world space, inverse previous transforms
		mat4 m = glm::scale(scale);
		m = inverse(getParentMatrix()) * m;
		this->scale = extractScale(m);
	}
	updateModelMatrix();
}
vec3 Transform::getEulerAngles(bool isWorld) {
	quat q = getRotation(isWorld);
	vec3 angles = degrees(eulerAngles(q));

	// clamp x between -180 and 180 degrees
	if (fabs(angles.x) >= 180) {
		float offset = sign(angles.x) * 180;
		angles.x -= offset;
		angles.y = offset - angles.y;
		angles.z += offset;
	}
	angles.x = fmod(angles.x, 360);
	angles.y = fmod(angles.y, 360);
	angles.z = fmod(angles.z, 360);
	return angles;
}
void Transform::setEulerAngles(vec3 angles, bool isWorld) {
	angles = radians(angles);
	quat x = angleAxis(angles.x, worldRight);
	quat y = angleAxis(angles.y, worldUp);
	quat z = angleAxis(angles.z, -worldForward);
	quat q = x * y * z;

	Entity* parent = entity->getParent();
	quat r(1, 0, 0, 0);
	if (parent) {
		r = parent->transform->getRotation(true);
	}

	q = isWorld ? q * r : r * q;
	setRotation(q, true);
}
void Transform::rotateAround(float degree, vec3 axis, bool isWorld) {
	quat q = getRotation(true);
	if (isWorld)
		q = angleAxis(radians(degree), axis) * q;
	else
		q = q * angleAxis(radians(degree), axis);

	setRotation(q, true);
}
void Transform::faceTo(vec3 target, vec3 up) {
	if (length(up) == 0) {
		up = worldUp;
	}
	vec3 position = getPosition(true);
	vec3 direction = normalize(target - position);
	up = normalize(up);
	if (approximately(direction, up)) {
		up = worldUp;
	}
	if (approximately(direction, worldUp)) {
		up = direction.z > 0 ? worldForward : -worldForward;
	}
	vec3 right = cross(direction, up);
	up = cross(right, direction);
	setRotation(conjugate(toQuat(lookAt(position, position + direction, up))), true);
}
vec3 Transform::getForwardVector(bool isWorld) {
	quat q = getRotation(isWorld);
	return normalize(q * worldForward);
}
vec3 Transform::getRightVector(bool isWorld) {
	quat q = getRotation(isWorld);
	return normalize(q * worldRight);
}
vec3 Transform::getUpVector(bool isWorld) {
	quat q = getRotation(isWorld);
	return normalize(q * worldUp);
}
void Transform::updateModelMatrix() {
	modelMatrix = getParentMatrix();
	modelMatrix *= translate(position);
	modelMatrix *= mat4_cast(rotation);
	modelMatrix *= glm::scale(scale);
	for (auto child : entity->getChildren()) {
		child->transform->updateModelMatrix();
	}
}
void Transform::onUpdate() {

}
#pragma endregion


#pragma region Entity 
Entity::Entity(string name) {
	this->name = name;
	transform = new Transform();
	addComponent(transform);
}
void Entity::getModelMatrix(float m[16]) {
	extractMatrix(transform->modelMatrix, m);
}
Entity* Entity::getParent() {
	return parent;
}
void Entity::setParent(Entity* parent) {
	this->parent = parent;
	parent->children.push_back(this);
	transform->modelMatrix = parent->transform->modelMatrix * transform->modelMatrix;
}
vector<Entity*> Entity::getChildren() {
	return children;
}

void Entity::update() {
	for (auto const& kvp : typeToComponent) {
		kvp.second->update();
	}
	for (int i = 0; i < children.size(); i++) {
		children[i]->update();
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
	type = "component" + type;
	return type;
}
#pragma endregion


#pragma region Component
Component::Component() {
	// hided constructor
}
Entity* Component::getEntity() {
	return entity;
}
void Component::setActive(bool isActive) {
	this->isActive = isActive;
}
void Component::update() {
	if (!isActive) return;
	onUpdate();
}
#pragma endregion

#pragma region Renderer
Renderer::Renderer(VertexArrayObject* vao) {
	this->vao = vao;
}
Renderer::Renderer(BasicPipelineProgram* pipelineProgram, Shape shape, vec4 color) {
	vector<vec3> positions;
	vector<vec4> colors;
	vector<int> indices;
	switch (shape) {
		case Shape::Cube:
			positions = { vec3(0.5, -0.5, 0.5), vec3(0.5, 0.5, 0.5), vec3(-0.5, 0.5, 0.5), vec3(-0.5, -0.5, 0.5),
							 vec3(0.5, -0.5, -0.5), vec3(0.5, 0.5, -0.5), vec3(-0.5, 0.5, -0.5), vec3(-0.5, -0.5, -0.5) };
			indices = { 3, 0, 7, 4, 5, 0, 1, 3, 2, 7, 6, 5, 2, 1 };
			drawMode = GL_TRIANGLE_STRIP;
			break;
		case Shape::Sphere:
			// to-do
			break;
		case Shape::Cylinder:
			// to-do
			break;
		default:
			break;
	}
	colors = vector<vec4>(positions.size(), color);
	vao = new VertexArrayObject(pipelineProgram, positions, colors, indices);
}
void Renderer::onUpdate() {
	if (!vao) return;

	// get matrices
	float v[16], m[16], mv[16], p[16];
	Camera::currentCamera->getViewMatrix(v);
	entity->getModelMatrix(m);
	mat4 mvMatrix = make_mat4(v) * make_mat4(m);
	extractMatrix(mvMatrix, mv);
	Camera::currentCamera->getProjectionMatrix(p);

	// draw
	vao->draw(mv, p, drawMode);
}
#pragma endregion

#pragma region Camera
Camera* Camera::currentCamera = nullptr;
Camera::Camera(bool setToCurrent) {
	projectionMatrix = perspective(radians(fieldOfView), aspect, zNear, zFar);
	if (setToCurrent) {
		setCurrent();
	}
}
void Camera::getProjectionMatrix(float* m) {
	extractMatrix(projectionMatrix, m);
}
void Camera::getViewMatrix(float* m) {
	extractMatrix(viewMatrix, m);
}
void Camera::setPerspective(float fieldOfView, float aspect, float zNear, float zFar) {
	this->fieldOfView = fieldOfView;
	this->aspect = aspect;
	this->zNear = zNear;
	this->zFar = zFar;
	projectionMatrix = perspective(radians(fieldOfView), aspect, zNear, zFar);
}
void Camera::setCurrent() {
	Camera::currentCamera = this;
}
bool Camera::isCurrentCamera() {
	return Camera::currentCamera == this;
}
void Camera::onUpdate() {
	if (!isCurrentCamera()) return;

	// update view matrix
	vec3 position = entity->transform->getPosition(true);
	vec3 center = position + entity->transform->getForwardVector(true);
	vec3 up = entity->transform->getUpVector(true);
	viewMatrix = lookAt(position, center, up);
}
#pragma endregion


#pragma region VertexArrayObject
VertexArrayObject::VertexArrayObject(BasicPipelineProgram* pipelineProgram) {
	this->pipelineProgram = pipelineProgram;
}
VertexArrayObject::VertexArrayObject(BasicPipelineProgram* pipelineProgram, vector<vec3> positions, vector<vec4> colors, vector<int> indices) {
	if (!pipelineProgram) {
		printf("error: pipeline program cannot be null. \n");
		return;
	}
	this->pipelineProgram = pipelineProgram;
	setVertices(positions, colors, indices);
	printf("Created VAO: numVertices %i, numColors %i, numIndices %i\n", numVertices, numColors, numIndices);
}
void VertexArrayObject::setVertices(vector<vec3> positions, vector<vec4> colors, vector<int> indices) {
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

	// generate vertex array
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
}
void VertexArrayObject::draw(float* mv, float* p, GLenum drawMode) {
	pipelineProgram->Bind();
	pipelineProgram->SetModelViewMatrix(mv);
	pipelineProgram->SetProjectionMatrix(p);
	glBindVertexArray(vertexArray);
	glDrawElements(drawMode, numIndices, GL_UNSIGNED_INT, 0);
}
#pragma endregion




