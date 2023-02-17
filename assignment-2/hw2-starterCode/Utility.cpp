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
	vec3 angles = degrees(eulerAngles(rotation));
	if (fabs(angles.x) >= 90) {
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
void Transform::setEulerAngles(vec3 angles) {
	angles.x = fmod(angles.x, 360);
	angles.y = fmod(angles.y, 360);
	angles.z = fmod(angles.z, 360);
	angles = radians(angles);
	rotation = quat(angles);
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
	transform->rotation = rotate(transform->rotation, radians(degree), axis);
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
	vec3 angles = transform->getEulerAngles();
	modelViewMatrix.Translate(transform->position.x, transform->position.y, transform->position.z);
	modelViewMatrix.Rotate(angles.x, 1, 0, 0);
	modelViewMatrix.Rotate(angles.y, 0, 1, 0);
	modelViewMatrix.Rotate(angles.z, 0, 0, 1);
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
	type = "component" + type;
	return type;
}
#pragma endregion



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
	colors = vector<vec4>(positions.size(), vec4(255));
	vao = new VertexArrayObject(pipelineProgram, positions, colors, indices);
}
void Renderer::onUpdate() {
	if (!vao) return;

	// get matrices
	float m[16];
	entity->getModelViewMatrix(m);
	float p[16];
	Camera::currentCamera->getProjectionMatrix(p);

	vao->pipelineProgram->Bind();
	// set variable
	vao->pipelineProgram->SetModelViewMatrix(m);
	vao->pipelineProgram->SetProjectionMatrix(p);

	glBindVertexArray(vao->vertexArray);
	glDrawElements(drawMode, vao->numIndices, GL_UNSIGNED_INT, 0);
}

#pragma region Physics
Physics::Physics(float minDistance, bool checkGround) {
	this->minDistance = minDistance;
	this->checkGround = checkGround;
}
void Physics::onUpdate() {
	vec3 position = entity->transform->position;
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

	entity->transform->position = position;
}
#pragma endregion


#pragma region Camera
Camera* Camera::currentCamera = nullptr;

Camera::Camera(bool setToCurrent) {
	projectionMatrix.SetMatrixMode(OpenGLMatrix::Projection);
	projectionMatrix.LoadIdentity();
	projectionMatrix.Perspective(fieldOfView, aspect, zNear, zFar);
	if (setToCurrent) {
		setCurrent();
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
void Camera::setCurrent() {
	Camera::currentCamera = this;
}
bool Camera::isCurrentCamera() {
	return Camera::currentCamera == this;
}
void Camera::onUpdate() {
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
	vertexDistances.push_back(-1);
}

vec3 RollerCoaster::getDirection() {
	return vertexTangents[currentVertexIndex];
}
vec3 RollerCoaster::getNormal() {
	return vertexNormals[currentVertexIndex];
}
void RollerCoaster::perform(Entity* target) {
	if (currentVertexIndex == numOfVertices - 1) {
		return;
	}

	float step = speed * Timer::getInstance()->getDeltaTime();
	// consume step
	while (step > 0) {
		float distanceToNext = vertexDistances[currentVertexIndex] * (1 - currentSegmentProgress);
		if (step < distanceToNext) break;

		step -= distanceToNext;
		// move to the next vertex and reset distance from current vertex
		currentSegmentProgress = 0;
		currentVertexIndex++;

		// if reach the end, return the end point position
		if (currentVertexIndex == numOfVertices - 1) break;
	}

	vec3 position;
	if (currentVertexIndex == numOfVertices - 1) {
		position = vertexPositions[currentVertexIndex];
	}
	else {
		currentSegmentProgress += step / vertexDistances[currentVertexIndex];
		position = mix(vertexPositions[currentVertexIndex], vertexPositions[currentVertexIndex + 1], currentSegmentProgress);
	}
	position += vertexNormals[currentVertexIndex] * size * 2.0f;

	target->transform->position = position;
	target->faceTo(position + getDirection(), getNormal());
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
			vec3 v(-1, 0, 0);
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
}
void RollerCoaster::onUpdate() {

}
#pragma endregion



