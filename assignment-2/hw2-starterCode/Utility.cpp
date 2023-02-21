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
// copied from openGLMatrix.cpp
void extractMatrix(mat4 matrix, float* m) {
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
		if (entity->getParent()) continue;
		entity->update();
	}
}
Entity* EntityManager::createEntity(string name) {
	if (name.empty()) {
		name = "Entity_" + entities.size();
	}
	Entity* entity = new Entity(name);
	entities.push_back(entity);
	return entity;
}
#pragma endregion


#pragma region Transform
Transform::Transform(Entity* entity) {
	this->entity = entity;
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
	
	return mat4(1);
}
void Transform::setPosition(vec3 position, bool isWorld) {
	if (!isWorld) {
		this->position = position;
	}
	else {
		mat4 m = translate(position);
		if (isWorld) m = inverse(getParentMatrix()) * m;
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
		mat4 m = mat4_cast(rotation);
		if (isWorld) m = inverse(getParentMatrix()) * m;
		this->rotation = extractRotation(m);
	}
	updateModelMatrix();
}
void Transform::setScale(vec3 scale, bool isWorld) {
	if (!isWorld) {
		this->scale = scale;
	}
	else {
		mat4 m = glm::scale(scale);
		m = inverse(getParentMatrix()) * m;
		this->scale = extractScale(m);
	}
	updateModelMatrix();
}
vec3 Transform::getEulerAngles(bool isWorld) {
	quat q = getRotation(isWorld);
	vec3 angles = degrees(eulerAngles(q));
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
void Transform::updateModelMatrix() {
	modelMatrix = getParentMatrix();
	modelMatrix *= translate(position);
	modelMatrix *= mat4_cast(rotation);
	modelMatrix *= glm::scale(scale);
	for (auto child : entity->getChildren()) {
		child->transform->updateModelMatrix();
	}
}	

#pragma endregion


#pragma region Entity 
Entity::Entity(string name) {
	this->name = name;
	transform = new Transform(this);
}
void Entity::getWorldModelMatrix(float m[16]) {
	extractMatrix(transform->modelMatrix, m);
}

void Entity::faceTo(vec3 target, vec3 up) {
	if (length(up) == 0) {
		up = worldUp;
	}
	vec3 position = transform->getPosition(true);
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
	transform->setRotation(conjugate(toQuat(lookAt(position, position + direction, up))), true);
}
vec3 Entity::getForwardVector(bool isWorld) {
	quat q = transform->getRotation(isWorld);
	return normalize(q * worldForward);
}
vec3 Entity::getRightVector(bool isWorld) {
	quat q = transform->getRotation(isWorld);
	return normalize(q * worldRight);
}
vec3 Entity::getUpVector(bool isWorld) {
	quat q = transform->getRotation(isWorld);
	return normalize(q * worldUp);
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
			break;
		case Shape::Cylinder:
			break;
		default:
			break;
	}
	colors = vector<vec4>(positions.size(), color);
	vao = new VertexArrayObject(pipelineProgram, positions, colors, indices);
}
void Renderer::onUpdate() {
	if (!vao) return;

	float mv[16];

	// get matrices
	float v[16];
	Camera::currentCamera->getViewMatrix(v);

	float m[16];
	entity->getWorldModelMatrix(m);

	OpenGLMatrix mvMatrix;
	mvMatrix.LoadMatrix(v);
	mvMatrix.MultMatrix(m);
	mvMatrix.GetMatrix(mv);

	float p[16];
	Camera::currentCamera->getProjectionMatrix(p);

	vao->pipelineProgram->Bind();
	// set variable
	vao->pipelineProgram->SetModelViewMatrix(mv);
	vao->pipelineProgram->SetProjectionMatrix(p);

	glBindVertexArray(vao->vertexArray);
	glDrawElements(drawMode, vao->numIndices, GL_UNSIGNED_INT, 0);
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
	projectionMatrix = mat4(1);
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

	vec3 position = entity->transform->getPosition(true);
	vec3 center = position + entity->getForwardVector(true);
	vec3 up = entity->getUpVector(true);
	viewMatrix = lookAt(position, center, up);
}
#pragma endregion



PlayerController::PlayerController() {

}
void PlayerController::moveOnGround(vec4 input, float step) {
	float z = input.x - input.y;
	float x = input.w - input.z;
	if (z == 0 && x == 0) return;

	vec3 forward = normalize(getProjectionOnPlane(entity->getForwardVector(true)));
	vec3 right = normalize(getProjectionOnPlane(entity->getRightVector(true)));
	vec3 move = normalize(x * right + z * forward) * step;

	vec3 position = entity->transform->getPosition(true) + move;
	entity->transform->setPosition(position, true);
	//log(entity->transform->getPosition(false));
}
void PlayerController::onUpdate() {
	// to-do: physics
}


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

	// vertex array
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
#pragma endregion



#pragma region RollerCoaster
RollerCoaster::RollerCoaster(Spline spline, int numOfStepsPerSegment) {
	for (int j = 1; j < spline.numControlPoints - 2; j++) {
		mat3x4 control = mat3x4(
			spline.points[j - 1].x, spline.points[j].x, spline.points[j + 1].x, spline.points[j + 2].x,
			spline.points[j - 1].y, spline.points[j].y, spline.points[j + 1].y, spline.points[j + 2].y,
			spline.points[j - 1].z, spline.points[j].z, spline.points[j + 1].z, spline.points[j + 2].z
		);

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

	for (int i = 0; i < vertexPositions.size() - 1; i++) {
		float distanceToNext = distance(vertexPositions[i], vertexPositions[i + 1]);
		vertexDistances.push_back(distanceToNext);
	}
	vertexDistances.push_back(0);
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
	isRunning = true;
}
void RollerCoaster::pause() {
	isRunning = false;
}
void RollerCoaster::reset() {
	isRunning = false;
	currentVertexIndex = 0;
	moveSeat();
}
void RollerCoaster::perform() {
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
	// get normals and binormals
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
	renderer->vao->setVertices(positions, colors, indices);
	renderer->drawMode = GL_TRIANGLE_STRIP;

	seat = EntityManager::getInstance()->createEntity();
	seat->addComponent(new Renderer(renderer->vao->pipelineProgram, Renderer::Shape::Cube));
	seat->setParent(entity);
	moveSeat();
}
void RollerCoaster::moveSeat() {
	seat->transform->setPosition(getCurrentPosition(), false);
	seat->faceTo(seat->transform->getPosition(true) + getCurrentDirection(), getCurrentNormal());
}
void RollerCoaster::onUpdate() {
	if (isRunning) {
		perform();
	}
}
#pragma endregion



