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


#pragma region SceneManager
void SceneManager::setLightings() {
	positions.clear();
	ambients.clear();
	diffuses.clear();
	speculars.clear();
	for (int i = 0; i < lights.size(); i++) {
		Light* light = lights[i];

		positions.push_back(light->getEntity()->transform->getPosition(true));
		ambients.push_back(light->ambient);
		diffuses.push_back(light->diffuse);
		speculars.push_back(light->specular);
	}
	// set lightings
	for (int i = 0; i < pipelinePrograms.size(); i++) {
		BasicPipelineProgram* pipeline = pipelinePrograms[i];

		GLuint loc = glGetUniformLocation(pipeline->GetProgramHandle(), "isLightingEnabled");
		glUniform1i(loc, isLightingEnabled);
		
		if (!isLightingEnabled) continue;

		loc = glGetUniformLocation(pipeline->GetProgramHandle(), "numOfLights");
		glUniform1i(loc, lights.size());
		loc = glGetUniformLocation(pipeline->GetProgramHandle(), "lightPositions");
		glUniform3fv(loc, positions.size(), reinterpret_cast<GLfloat*>(&positions[0]));
		loc = glGetUniformLocation(pipeline->GetProgramHandle(), "lightAmbients");
		glUniform4fv(loc, ambients.size(), reinterpret_cast<GLfloat*>(&ambients[0]));
		loc = glGetUniformLocation(pipeline->GetProgramHandle(), "lightDiffuses");
		glUniform4fv(loc, diffuses.size(), reinterpret_cast<GLfloat*>(&diffuses[0]));
		loc = glGetUniformLocation(pipeline->GetProgramHandle(), "lightSpeculars");
		glUniform4fv(loc, speculars.size(), reinterpret_cast<GLfloat*>(&speculars[0]));
	}
}
void SceneManager::update() {
	setLightings();
	

	for (int i = 0; i < entities.size(); i++) {
		Entity* entity = entities[i];

		// update root entities only
		if (entity->getParent()) continue;
		entity->update();
	}
}
Entity* SceneManager::createEntity(string name) {
	if (name.empty()) {
		// set default name
		name = "Entity_" + to_string(entities.size());
	}
	Entity* entity = new Entity(name);
	entities.push_back(entity);
	printf("Entity \"%s\" created. Number of entities: %i\n", name.c_str(), entities.size());
	return entity;
}
BasicPipelineProgram* SceneManager::createPipelineProgram(string shaderPath) {
	BasicPipelineProgram* pipeline = new BasicPipelineProgram;
	int ret = pipeline->Init(shaderPath.c_str());
	if (ret != 0) abort();
	pipelinePrograms.push_back(pipeline);
	printf("Pipeline program created. Shader path: %s. Number of pipelines: %i. \n", shaderPath.c_str(), pipelinePrograms.size());
	return pipeline;
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
bool Entity::isActive() {
	return isActivated;
}
void Entity::setActive(bool isActive) {
	isActivated = isActive;
}
mat4 Entity::getModelMatrix() {
	return transform->modelMatrix;
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
	if (!isActivated) return;

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
bool Component::isActive() {
	return isActivated;
}
void Component::setActive(bool isActive) {
	isActivated = isActive;
}
void Component::update() {
	if (!isActivated) return;
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
	vector<vec3> normals;
	vector<vec2> texCoords;
	switch (shape) {
		case Shape::Cube:
			positions = { 
				vec3(-0.5, -0.5, 0.5), vec3(0.5, -0.5, 0.5), vec3(-0.5, 0.5, 0.5),
				vec3(0.5, 0.5, 0.5), vec3(-0.5, 0.5, 0.5), vec3(0.5, 0.5, 0.5),
				vec3(-0.5, 0.5, -0.5), vec3(0.5, 0.5, -0.5), vec3(-0.5, 0.5, -0.5),
				vec3(0.5, 0.5, -0.5), vec3(-0.5, -0.5, -0.5), vec3(0.5, -0.5, -0.5),
				vec3(-0.5, -0.5, -0.5), vec3(0.5, -0.5, -0.5), vec3(-0.5, -0.5, 0.5),
				vec3(0.5, -0.5, 0.5), vec3(0.5, -0.5, 0.5), vec3(0.5, -0.5, -0.5),
				vec3(0.5, 0.5, 0.5), vec3(0.5, 0.5, -0.5), vec3(-0.5, -0.5, -0.5),
				vec3(-0.5, -0.5, 0.5), vec3(-0.5, 0.5, -0.5), vec3(-0.5, 0.5, 0.5) };

			normals = { 
				vec3(0, 0, 1), vec3(0, 0, 1), vec3(0, 0, 1),
				vec3(0, 0, 1), vec3(0, 1, 0), vec3(0, 1, 0),
				vec3(0, 1, 0), vec3(0, 1, 0), vec3(0, 0, -1),
				vec3(0, 0, -1), vec3(0, 0, -1), vec3(0, 0, -1),
				vec3(0, -1, 0), vec3(0, -1, 0), vec3(0, -1, 0),
				vec3(0, -1, 0), vec3(1, 0, 0), vec3(1, 0, 0),
				vec3(1, 0, 0), vec3(1, 0, 0), vec3(-1, 0, 0),
				vec3(-1, 0, 0), vec3(-1, 0, 0), vec3(-1, 0, 0) };

			indices = { 0, 1, 2, 2, 1, 3, 4, 5, 6, 6, 5, 7, 8, 9, 10, 10, 9, 11, 12, 13, 14, 14, 13, 15, 16, 17, 18, 18, 17, 19, 20, 21, 22, 22, 21, 23 };
			drawMode = GL_TRIANGLES;
			break;
		case Shape::Sphere:
			// to-do
			break;
		case Shape::Cylinder:
			// to-do
			break;
		case Shape::Plane:
			positions = {
				vec3(0.5, 0.5, 0),
				vec3(-0.5, 0.5, 0),
				vec3(-0.5, -0.5, 0),
				vec3(0.5, -0.5, 0) }
			;
			normals = {
				vec3(0, 0, 1),
				vec3(0, 0, 1),
				vec3(0, 0, 1),
				vec3(0, 0, 1) }
			;
			indices = {0, 1, 2, 0, 2, 3 };
			texCoords = {
				vec2(1, 0),
				vec2(0, 0),
				vec2(0, 1),
				vec2(1, 1) };
			drawMode = GL_TRIANGLES;
			break;
		default:
			break;
	}
	colors = vector<vec4>(positions.size(), color);
	vao = new VertexArrayObject(pipelineProgram, positions, colors, indices);
	vao->setNormals(normals);
	vao->setTexCoords(texCoords);
}
void Renderer::onUpdate() {
	if (!vao) return;

	// set material data
	BasicPipelineProgram* pipelineProgram = vao->pipelineProgram;
	GLuint loc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "ambientCoef");
	glUniform4f(loc, ambient[0], ambient[1], ambient[2], ambient[3]);
	loc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "diffuseCoef");
	glUniform4f(loc, diffuse[0], diffuse[1], diffuse[2], diffuse[3]);
	loc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "specularCoef");
	glUniform4f(loc, specular[0], specular[1], specular[2], specular[3]);
	loc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "materialShininess");
	glUniform1f(loc, shininess);

	loc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "eyePosition");
	vec3 pos = Camera::currentCamera->getEntity()->transform->getPosition(true);
	glUniform3f(loc, pos[0], pos[1], pos[2]);
	// get matrices
	float m[16], v[16], p[16], n[16];
	mat4 modelMatrix = entity->getModelMatrix();
	extractMatrix(modelMatrix, m);
	extractMatrix(transpose(inverse(modelMatrix)), n);
	Camera::currentCamera->getViewMatrix(v);
	Camera::currentCamera->getProjectionMatrix(p);

	// draw
	vao->draw(m, v, p, n, drawMode);
}
#pragma endregion

#pragma region Physics
Physics::Physics(float minDistance, bool checkGround) {
	this->minDistance = minDistance;
	this->checkGround = checkGround;
}
void Physics::onUpdate() {
	vec3 position = entity->transform->getPosition(true);
	float deltaTime = Timer::getInstance()->getDeltaTime();
	position += velocity * deltaTime;

	float minY = GROUND_Y + minDistance;
	if (position.y > minY) {
		velocity += GRAVITY * deltaTime;
		position += 0.5f * GRAVITY * deltaTime * deltaTime;
		isOnGround = false;
	}
	if (position.y <= minY) {
		velocity.y = 0;
		position.y = minY;
		isOnGround = true;
	}

	entity->transform->setPosition(position, true);
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

#pragma region Light
Light::Light() {
	ambient = vec4(0.4, 0.4, 0.4, 1);
	diffuse = vec4(.8, .8, .8, 1);
	specular = vec4(1, 1, 1, 1);
	SceneManager::getInstance()->lights.push_back(this);

	// debug 
	ambient = vec4(1);
}
void Light::onUpdate() {

}
#pragma endregion

#pragma region PlayerController
PlayerController::PlayerController() {

}
void PlayerController::move(vec4 input, float verticalMove) {
	float z = input.x - input.y;
	float x = input.w - input.z;
	if (z == 0 && x == 0 && verticalMove == 0) return;

	if (verticalMove > 0.0f) verticalMove = 1.0f;
	else if (verticalMove < 0.0f) verticalMove = -1.0f;

	vec3 move(0);
	float step = Timer::getInstance()->getDeltaTime() * speed;
	vec3 horizontalDirection = entity->transform->getForwardVector(true) * z + entity->transform->getRightVector(true) * x;
	if (length(horizontalDirection) > 0.0f) move += normalize(horizontalDirection) * step;
	move += worldUp * step * verticalMove;
	vec3 position = entity->transform->getPosition(true) + move;
	entity->transform->setPosition(position, true);
}
void PlayerController::moveOnGround(vec4 input) {
	float z = input.x - input.y;
	float x = input.w - input.z;
	if (z == 0 && x == 0) return;

	float step = Timer::getInstance()->getDeltaTime() * speed;
	vec3 forward = normalize(getProjectionOnPlane(entity->transform->getForwardVector(true)));
	vec3 right = normalize(getProjectionOnPlane(entity->transform->getRightVector(true)));
	vec3 move = normalize(x * right + z * forward) * step;

	vec3 position = entity->transform->getPosition(true) + move;
	entity->transform->setPosition(position, true);
}
void PlayerController::onUpdate() {

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
void VertexArrayObject::setNormals(vector<vec3> normals) {
	numNormals = normals.size();
	if (numNormals == 0) {
		printf("\nerror: the number of normals cannot be 0. \n");
		return;
	}if (numVertices != numNormals) {
		printf("\nerror: the number of vertices %i does not match the number of normals %i. \n", numVertices, numNormals);
		return;
	}
	glGenBuffers(1, &normalBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * numNormals, &normals[0], GL_STATIC_DRAW);

	GLuint loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "normal");
	glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

}
void VertexArrayObject::setTexCoords(vector<vec2> texCoords) {
	numTexCoords = texCoords.size();
	if (numTexCoords == 0) {
		printf("\nerror: the number of texture coordinates cannot be 0. \n");
		return;
	}
	glGenBuffers(1, &texCoordBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, texCoordBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec2) * numTexCoords, &texCoords[0], GL_STATIC_DRAW);

	GLuint loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "texCoord");
	glBindBuffer(GL_ARRAY_BUFFER, texCoordBuffer);
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, 0, (const void*)0);
}
void VertexArrayObject::draw(float* m, float* v, float* p, float* n, GLenum drawMode) {
	pipelineProgram->Bind();
	pipelineProgram->SetModelMatrix(m);
	pipelineProgram->SetViewMatrix(v);
	pipelineProgram->SetProjectionMatrix(p);
	pipelineProgram->SetNormalMatrix(n);
	glBindVertexArray(vertexArray);
	glDrawElements(drawMode, numIndices, GL_UNSIGNED_INT, 0);
}
#pragma endregion



#pragma region RollerCoaster
RollerCoaster::RollerCoaster(Spline spline, int numOfStepsPerSegment) {
	// calculate data from spline
	for (int j = 1; j < spline.numControlPoints - 2; j++) {
		mat3x4 control = mat3x4(
			spline.points[j - 1].x, spline.points[j].x, spline.points[j + 1].x, spline.points[j + 2].x,
			spline.points[j - 1].y, spline.points[j].y, spline.points[j + 1].y, spline.points[j + 2].y,
			spline.points[j - 1].z, spline.points[j].z, spline.points[j + 1].z, spline.points[j + 2].z
		);

		// calculate positions and tangents
		float delta = 1.0f / numOfStepsPerSegment;
		for (int k = 0; k < numOfStepsPerSegment + 1; k++) {
			float u = k * 1.0f / numOfStepsPerSegment;
			vec4 uVector(u * u * u, u * u, u, 1);
			vec3 point = uVector * BASIS * control;
			vertexPositions.push_back(point);

			vec4 uPrime(3 * u * u, 2 * u, 1, 0);
			vertexTangents.push_back(normalize(uPrime * BASIS * control));

			u += delta;
		}
	}
	currentVertexIndex = 0;
	numOfVertices = vertexPositions.size();

	// calculate distances
	for (int i = 0; i < vertexPositions.size() - 1; i++) {
		float distanceToNext = distance(vertexPositions[i], vertexPositions[i + 1]);
		vertexDistances.push_back(distanceToNext);
	}
	vertexDistances.push_back(0);

	// deactivated at defualt
	setActive(false);
}
vec3 RollerCoaster::getCurrentPosition() {
	vec3 position;
	if (currentVertexIndex == numOfVertices - 1) {
		position = vertexPositions[currentVertexIndex];
	}
	else {
		position = mix(vertexPositions[currentVertexIndex], vertexPositions[currentVertexIndex + 1], currentSegmentProgress);
	}
	position += vertexNormals[currentVertexIndex] * size * 2.0f;
	return position;
}
vec3 RollerCoaster::getCurrentDirection() {
	return vertexTangents[currentVertexIndex];
}
vec3 RollerCoaster::getCurrentNormal() {
	return vertexNormals[currentVertexIndex];
}
void RollerCoaster::start(bool isRepeating, bool isTwoWay) {
	this->isRepeating = isRepeating;
	this->isTwoWay = isTwoWay;
	setActive(true);
}
void RollerCoaster::pause() {
	setActive(false);
}
void RollerCoaster::reset() {
	setActive(false);
	currentVertexIndex = 0;
	moveSeat();
}
void RollerCoaster::render() {
	Renderer* renderer = entity->getComponent<Renderer>();
	if (!renderer) {
		printf("Error: cannot find Renderer to render the RollerCoaster. \n");
		return;
	}

	vector<vec3> normals;
	vector<vec3> binormals;

	vector<vec3> positions;
	vector<vec4> colors;
	vector<int> indices;

	// calculate normals and binormals
	for (int i = 0; i < numOfVertices; i++) {
		vec3 p = vertexPositions[i];
		vec3 t = vertexTangents[i];
		vec3 n;
		if (i == 0) {
			vec3 v(1, 0, 0);
			n = normalize(cross(t, v));
		}
		else {
			n = normalize(cross(binormals[i - 1], t));
		}
		vec3 b = normalize(cross(t, n));

		normals.push_back(n);
		binormals.push_back(b);

		// calculate positions, colors and indices
		vec3 v0 = p + size * (-n + b);
		vec3 v1 = p + size * (n + b);
		vec3 v2 = p + size * (n - b);
		vec3 v3 = p + size * (-n - b);
		positions.insert(positions.end(), { v0, v1, v2, v3 });

		vec4 color = vec4(n, 1) * 255.0f;
		colors.insert(colors.end(), { color, color, color, color });

		if (i == 0 || i == numOfVertices - 1) {
			int index = 4 * i;
			indices.insert(indices.end(), { index, index + 1, index + 3, index + 2, RESTARTINDEX });
		}
		if (i > 0) {
			int index = 4 * (i - 1);
			indices.insert(indices.end(),
						   { index, index + 4, index + 1, index + 5,
						   index + 2, index + 6, index + 3, index + 7,
						   index, index + 4, RESTARTINDEX });
		}
	}
	vertexNormals = normals;

	// set up renderer
	renderer->vao->setVertices(positions, colors, indices);
	renderer->drawMode = GL_TRIANGLE_STRIP;

	// set up seat
	seat = SceneManager::getInstance()->createEntity("Seat");
	seat->addComponent(new Renderer(renderer->vao->pipelineProgram, Renderer::Shape::Cube));
	seat->setParent(entity);
	moveSeat();
}
void RollerCoaster::moveSeat() {
	seat->transform->setPosition(getCurrentPosition(), false);
	seat->transform->faceTo(seat->transform->getPosition(true) + getCurrentDirection(), getCurrentNormal());
}
void RollerCoaster::onUpdate() {
	float step = speed * Timer::getInstance()->getDeltaTime();
	// consume step
	while (step > 0) {
		float distanceToNext = vertexDistances[currentVertexIndex] * (1 - currentSegmentProgress);
		if (step < distanceToNext) break;

		step -= distanceToNext;

		// move to the next vertex and reset distance from current vertex
		currentSegmentProgress = 0;
		currentVertexIndex++;
		if (currentVertexIndex == numOfVertices - 1) {
			if (!isRepeating) {
				step = 0;
				pause();
			}
			else {
				reset();
				start(isTwoWay);
			}
		}
	}
	currentSegmentProgress += step / vertexDistances[currentVertexIndex];
	moveSeat();
}
#pragma endregion



