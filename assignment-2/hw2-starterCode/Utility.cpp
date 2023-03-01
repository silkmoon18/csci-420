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
string getCurrentDirectory() {
	return filesystem::current_path().string();
}

#pragma region Shape
Shape makePlane(float width, float length) {
	vector<vec3> positions;
	vector<int> indices;
	vector<vec3> normals;
	vector<vec2> texCoords;
	GLenum drawMode = GL_TRIANGLES;

	float x = width / 2.0f;
	float z = length / 2.0f;
	positions = {
		vec3(x, 0, z),
		vec3(-x, 0, z),
		vec3(-x, 0, -z),
		vec3(x, 0, -z) }
	;
	normals = {
		vec3(0, 1, 0),
		vec3(0, 1, 0),
		vec3(0, 1, 0),
		vec3(0, 1, 0) }
	;
	indices = { 0, 1, 2, 0, 2, 3 };
	texCoords = {
		vec2(1, 0),
		vec2(0, 0),
		vec2(0, 1),
		vec2(1, 1) };

	return Shape(positions, indices, normals, texCoords, drawMode);
}
Shape makeCube(float width, float length, float height) {
	vector<vec3> positions;
	vector<int> indices;
	vector<vec3> normals;
	vector<vec2> texCoords;
	GLenum drawMode = GL_TRIANGLES;

	float x = width / 2.0f;
	float y = height / 2.0f;
	float z = length / 2.0f;
	positions = {
		vec3(-x, -y, z), vec3(x, -y, z), vec3(-x, y, z),
		vec3(x, y, z), vec3(-x, y, z), vec3(x, y, z),
		vec3(-x, y, -z), vec3(x, y, -z), vec3(-x, y, -z),
		vec3(x, y, -z), vec3(-x, -y, -z), vec3(x, -y, -z),
		vec3(-x, -y, -z), vec3(x, -y, -z), vec3(-x, -y, z),
		vec3(x, -y, z), vec3(x, -y, z), vec3(x, -y, -z),
		vec3(x, y, z), vec3(x, y, -z), vec3(-x, -y, -z),
		vec3(-x, -y, z), vec3(-x, y, -z), vec3(-x, y, z) };

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

	return Shape(positions, indices, normals, texCoords, drawMode);
}
Shape makeSphere(float radius, int resolution) {
	vector<vec3> positions;
	vector<int> indices;
	vector<vec3> normals;
	vector<vec2> texCoords;
	GLenum drawMode = GL_TRIANGLES;

	float rad = radians(360.0f / resolution);
	float stackAngle = radians(90.0f) - rad;
	float sectorAngle = radians(0.0f);

	positions.push_back(vec3(0, radius, 0));
	normals.push_back(vec3(0, radius, 0));

	int index = 0;
	for (int i = 1; i < resolution / 2; i++) {
		for (int j = 0; j < resolution; j++) {
			float x = radius * cos(stackAngle) * cos(sectorAngle);
			float y = radius * sin(stackAngle);
			float z = radius * cos(stackAngle) * sin(sectorAngle);

			positions.push_back(vec3(x, y, z));
			normals.push_back(vec3(x, y, z));

			float s = fmod(sectorAngle, radians(360.0f)) / radians(360.0f);
			float t = (fmod(stackAngle, radians(360.0f)) + radians(90.0f)) / radians(180.0f);
			texCoords.push_back(vec2(s, t));
			
			sectorAngle += rad;
			index = (i - 1) * resolution + j;

			if (i == 1) {
				indices.push_back(0);
				indices.push_back(index + 1);
				if (j == resolution - 1) indices.push_back(1);
				else indices.push_back(index + 2);
			}
			else {
				index++;

				indices.push_back(index - resolution);
				if (j == resolution - 1) indices.push_back(index - 2 * resolution + 1);
				else indices.push_back(index + 1 - resolution);
				indices.push_back(index);

				if (j == resolution - 1) indices.push_back(index + 1 - resolution);
				else indices.push_back(index + 1);
				indices.push_back(index);
				if (j == resolution - 1) indices.push_back(index - 2 * resolution + 1);
				else indices.push_back(index + 1 - resolution);
			}
		}
		stackAngle -= rad;
	}

	positions.push_back(vec3(0, -radius, 0));
	normals.push_back(vec3(0, -radius, 0));

	for (int i = index - resolution + 1; i < index + 1; i++) {
		indices.push_back(i);
		if (i == index) indices.push_back(index - resolution + 1);
		else indices.push_back(i + 1);
		indices.push_back(index + 1);
	}

	return Shape(positions, indices, normals, texCoords, drawMode);
}
Shape makeCylinder(float radius, float height, int resolution) {
	vector<vec3> positions;
	vector<int> indices;
	vector<vec3> normals;
	vector<vec2> texCoords;
	GLenum drawMode = GL_TRIANGLES;

	float rad = radians(360.0f / resolution);
	float prevX = 0;
	float prevZ = radius;
	for (int i = 0; i < resolution; i++) {
		float x = prevX * cos(rad) - prevZ * sin(rad);
		float y = height / 2;
		float z = prevZ * cos(rad) + prevX * sin(rad);

		positions.push_back(vec3(x, y, z));
		positions.push_back(vec3(x, -y, z));

		normals.push_back(vec3(x, 0, z));
		normals.push_back(vec3(x, 0, z));

		prevX = x;
		prevZ = z;

		int index = 2 * i;
		if (i == resolution - 1) {
			indices.push_back(index);
			indices.push_back(0);
			indices.push_back(index + 1);

			indices.push_back(1);
			indices.push_back(index + 1);
			indices.push_back(0);
		}
		else {
			indices.push_back(index);
			indices.push_back(index + 2);
			indices.push_back(index + 1);

			indices.push_back(index + 3);
			indices.push_back(index + 1);
			indices.push_back(index + 2);
		}
	}

	return Shape(positions, indices, normals, texCoords, drawMode);
}
Shape makeTetrahedron(float width, float height) {
	vector<vec3> positions;
	vector<int> indices;
	vector<vec3> normals;
	vector<vec2> texCoords;
	GLenum drawMode = GL_TRIANGLES;

	float x = width / 2.0f;
	float y = height;
	float z = (float)sqrt(3) * x / 2;
	float topZ = z - x / (float)sqrt(3);
	positions = {
		vec3(x, 0, z), vec3(-x, 0, z), vec3(0, 0, -z),
		vec3(x, 0, z), vec3(-x, 0, z), vec3(0, y, topZ),
		vec3(-x, 0, z), vec3(0, 0, -z), vec3(0, y, topZ),
		vec3(x, 0, z), vec3(0, 0, -z), vec3(0, y, topZ) };

	vec3 bottomNormal = vec3(0, -1, 0);
	vec3 frontNormal = normalize(cross(positions[0] - positions[5], positions[0] - positions[1]));
	vec3 leftBackNormal = normalize(cross(positions[1] - positions[5], positions[1] - positions[2]));
	vec3 rightBackNormal = normalize(cross(positions[0] - positions[2], positions[0] - positions[5]));
	normals = {
		bottomNormal, bottomNormal, bottomNormal,
		frontNormal, frontNormal, frontNormal,
		leftBackNormal, leftBackNormal, leftBackNormal,
		rightBackNormal, rightBackNormal, rightBackNormal };

	indices = {
		0, 1, 2,
		3, 4, 5,
		6, 7, 8,
		9, 10, 11};

	return Shape(positions, indices, normals, texCoords, drawMode);
}
Shape::Shape(
	vector<vec3> positions,
	vector<int> indices,
	vector<vec3> normals,
	vector<vec2> texCoords,
	GLenum drawMode) {
	this->positions = positions;
	this->indices = indices;
	this->normals = normals;
	this->texCoords = texCoords;
	this->drawMode = drawMode;
}
#pragma endregion

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
Entity* SceneManager::createEntity(string name) {
	if (name.empty()) {
		// set default name
		name = "Entity_" + to_string(entities.size());
	}
	Entity* entity = new Entity(name);
	entities.push_back(entity);
	return entity;
}
Entity* SceneManager::createSkybox(BasicPipelineProgram* pipeline, Texture* texture) {
	skybox = new Entity("Skybox");
	skybox->transform->setPosition(vec3(0, 0, 0), true);
	Shape* shape = new Shape(makeCube());
	VertexArrayObject* vao = new VertexArrayObject(pipeline);
	vao->setPositions(shape->positions);
	vao->setIndices(shape->indices);
	vao->setColors(vector<vec4>(shape->positions.size(), vec4(255)));
	Renderer* skyRenderer = new Renderer(vao, shape->drawMode);
	skyRenderer->useLight = false;
	skyRenderer->setTexture(texture);
	skyRenderer->isSkyBox = true;
	skybox->addComponent(skyRenderer);
	return skybox;
}
BasicPipelineProgram* SceneManager::createPipelineProgram(string shaderPath) {
	BasicPipelineProgram* pipeline = new BasicPipelineProgram;
	int ret = pipeline->Init((getCurrentDirectory() + shaderPath).c_str());
	if (ret != 0) abort();
	pipelinePrograms.push_back(pipeline);
	printf("Pipeline program created. Shader path: %s. Number of pipelines: %i. \n", shaderPath.c_str(), pipelinePrograms.size());
	return pipeline;
}
void SceneManager::update() {
	// set lightings
	for (unsigned int i = 0; i < pipelinePrograms.size(); i++) {
		if (!isLightingEnabled) break;

		BasicPipelineProgram* pipeline = pipelinePrograms[i];
		pipeline->Bind();

		GLuint handle = pipeline->GetProgramHandle();
		Light::sendData(handle);

		GLuint loc = glGetUniformLocation(handle, "eyePosition");
		vec3 pos = Camera::currentCamera->getEntity()->transform->getPosition(true);
		glUniform3f(loc, pos[0], pos[1], pos[2]);
	}

	// draw skybox first
	if (skybox) {
		glDepthMask(GL_FALSE);
		skybox->update();
		skybox->getComponent<Renderer>()->render();
		glDepthMask(GL_TRUE);
	}

	// update entities
	for (unsigned int i = 0; i < entities.size(); i++) {
		Entity* entity = entities[i];

		// update root entities only
		if (entity->getParent()) continue;
		entity->update();
	}

	// finally update camera and draw everything
	Camera::currentCamera->view();
	for (auto* renderer : Renderer::getRenderers()) {
		if (renderer->isSkyBox) continue;
		renderer->render();
	}
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
	angles.x = (float)fmod(angles.x, 360);
	angles.y = (float)fmod(angles.y, 360);
	angles.z = (float)fmod(angles.z, 360);
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
void Transform::rotateAround(vec3 pivot, float degree, vec3 axis) {
	quat q = angleAxis(radians(degree), axis);
	setPosition(q * (getPosition(true) - pivot) + pivot, true);
	setRotation(q * getRotation(true), true);
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
	if (!parent) return;

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
	for (unsigned int i = 0; i < children.size(); i++) {
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

#pragma region Texture
GLuint Texture::getHandle() {
	return handle;
}
void Texture::load(BasicPipelineProgram* pipeline) {
	glActiveTexture(GL_TEXTURE0 + textureTypeId);
	GLuint loc = glGetUniformLocation(pipeline->GetProgramHandle(), "textureTypeId");
	glUniform1i(loc, textureTypeId);
	glBindTexture(type, handle);
}
Texture2D::Texture2D(string imageName) : Texture(GL_TEXTURE_2D, 0) {
	imageName = getCurrentDirectory() + imageName;
	const char* imageFilename = imageName.c_str();
	// read the texture image
	ImageIO img;
	ImageIO::fileFormatType imgFormat;
	ImageIO::errorType err = img.load(imageFilename, &imgFormat);
	if (err != ImageIO::OK) {
		printf("!!!Error: Loading texture from %s failed.\n", imageFilename);
		return;
	}
	// check that the number of bytes is a multiple of 4
	if (img.getWidth() * img.getBytesPerPixel() % 4) {
		printf("!!!Error (%s): The width*numChannels in the loaded image must be a multiple of 4.\n", imageFilename);
		return;
	}

	// generate and bind the texture
	glGenTextures(1, &handle);
	glBindTexture(GL_TEXTURE_2D, handle);

	// allocate space for an array of pixels
	int width = img.getWidth();
	int height = img.getHeight();
	unsigned char* pixelsRGBA = new unsigned char[4 * width * height]; // we will use 4 bytes per pixel, i.e., RGBA

	// fill the pixelsRGBA array with the image pixels
	memset(pixelsRGBA, 0, 4 * width * height); // set all bytes to 0
	for (int h = 0; h < height; h++)
		for (int w = 0; w < width; w++) {
			// assign some default byte values (for the case where img.getBytesPerPixel() < 4)
			pixelsRGBA[4 * (h * width + w) + 0] = 0; // red
			pixelsRGBA[4 * (h * width + w) + 1] = 0; // green
			pixelsRGBA[4 * (h * width + w) + 2] = 0; // blue
			pixelsRGBA[4 * (h * width + w) + 3] = 255; // alpha channel; fully opaque

			// set the RGBA channels, based on the loaded image
			int numChannels = img.getBytesPerPixel();
			for (int c = 0; c < numChannels; c++) // only set as many channels as are available in the loaded image; the rest get the default value
				pixelsRGBA[4 * (h * width + w) + c] = img.getPixel(w, h, c);
		}

	// initialize the texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelsRGBA);

	// generate the mipmaps for this texture
	glGenerateMipmap(GL_TEXTURE_2D);

	// set the texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// query support for anisotropic texture filtering
	GLfloat fLargest;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
	//printf("Max available anisotropic samples: %f\n", fLargest);
	// set anisotropic texture filtering
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 0.5f * fLargest);

	// query for any errors
	GLenum errCode = glGetError();
	if (errCode != 0) {
		printf("!!!Error: texture initialization error. %d.\n", errCode);
		return;
	}
}
Cubemap::Cubemap(string imageDirectory) : Texture(GL_TEXTURE_CUBE_MAP, 1) {
	// find images from directory
	string imageNames[6];
	imageDirectory = getCurrentDirectory() + imageDirectory;
	for (const auto& entry : filesystem::directory_iterator(imageDirectory)) {
		filesystem::path path = entry.path();

		string extension = path.extension().string();
		if (extension == ".jpg" || extension == ".jpeg") {
			string filename = path.stem().string();
			string filePath = imageDirectory + "/" + filename + ".jpg";
			if (filename.compare("right") == 0) imageNames[0] = filePath;
			else if (filename.compare("left") == 0) imageNames[1] = filePath;
			else if (filename.compare("bottom") == 0) imageNames[2] = filePath;
			else if (filename.compare("top") == 0) imageNames[3] = filePath;
			else if (filename.compare("front") == 0) imageNames[4] = filePath;
			else if (filename.compare("back") == 0) imageNames[5] = filePath;
		}
		else {
			printf("!!!Error: texture file %s is not jpg. \n", path.string().c_str());
			return;
		}
	}
	// check if there are 6 images
	int numOfImages = 0;
	for (int i = 0; i < 6; i++) {
		if (!imageNames[i].empty()) numOfImages++;
	}
	if (numOfImages < 6) {
		printf("!!!Error: 6 images are needed for cubemap. Currently %i. \n", numOfImages);
		return;
	}

	// generate and bind the texture
	glGenTextures(1, &handle);
	glBindTexture(GL_TEXTURE_CUBE_MAP, handle);

	// read the texture image
	ImageIO images[6];
	ImageIO::fileFormatType imgFormat;
	for (int i = 0; i < 6; i++) {
		ImageIO img = images[i];
		string imageName = imageNames[i];
		const char* imageFilename = (imageName).c_str();

		ImageIO::errorType err = img.load(imageFilename, &imgFormat);

		if (err != ImageIO::OK) {
			printf("!!!Error: loading texture from %s failed.\n", imageFilename);
			return;
		}

		// check that the number of bytes is a multiple of 4
		if (img.getWidth() * img.getBytesPerPixel() % 4) {
			printf("!!!Error (%s): The width*numChannels in the loaded image must be a multiple of 4.\n", imageFilename);
			return;
		}

		// allocate space for an array of pixels
		int width = img.getWidth();
		int height = img.getHeight();
		unsigned char* pixelsRGBA = new unsigned char[4 * width * height]; // we will use 4 bytes per pixel, i.e., RGBA

		// fill the pixelsRGBA array with the image pixels
		memset(pixelsRGBA, 0, 4 * width * height); // set all bytes to 0

		for (int h = 0; h < height; h++) {
			for (int w = 0; w < width; w++) {
				// assign some default byte values (for the case where img.getBytesPerPixel() < 4)
				pixelsRGBA[4 * (h * width + w) + 0] = 0; // red
				pixelsRGBA[4 * (h * width + w) + 1] = 0; // green
				pixelsRGBA[4 * (h * width + w) + 2] = 0; // blue
				pixelsRGBA[4 * (h * width + w) + 3] = 255; // alpha channel; fully opaque

				// set the RGBA channels, based on the loaded image
				int numChannels = img.getBytesPerPixel();
				for (int c = 0; c < numChannels; c++) // only set as many channels as are available in the loaded image; the rest get the default value
					pixelsRGBA[4 * (h * width + w) + c] = img.getPixel(w, h, c);
			}
		}

		// initialize the texture
		glTexImage2D(
			GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
			0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelsRGBA
		);
	}

	// generate the mipmaps for this texture
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	// set the texture parameters
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	// query support for anisotropic texture filtering
	GLfloat fLargest;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
	//printf("Max available anisotropic samples: %f\n", fLargest);
	// set anisotropic texture filtering
	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_ANISOTROPY_EXT, 0.5f * fLargest);

	// query for any errors
	GLenum errCode = glGetError();
	if (errCode != 0) {
		printf("!!!Error: texture initialization error. Error code: %d.\n", errCode);
		return;
	}
}
#pragma endregion

#pragma region Renderer
vector<Renderer*> Renderer::getRenderers() {
	return renderers;
}
Renderer::Renderer(VertexArrayObject* vao, GLenum drawMode) {
	this->vao = vao;
	this->drawMode = drawMode;
	renderers.push_back(this);
}
Renderer::Renderer(BasicPipelineProgram* pipelineProgram, Shape shape, vec4 color) {
	vao = new VertexArrayObject(pipelineProgram, shape.positions, vector<vec4>(shape.positions.size(), color), shape.indices);
	vao->setNormals(shape.normals);
	vao->setTexCoords(shape.texCoords);
	drawMode = shape.drawMode;
	renderers.push_back(this);
}
void Renderer::setTexture(Texture* texture) {
	this->texture = texture;
}
void Renderer::render() {
	if (!vao) {
		printf("***Warning: no VAO found on the Renderer of Entity \"%s\". \n", entity->name.c_str());
		return;
	}

	vao->bindPipeline();

	BasicPipelineProgram* pipelineProgram = vao->pipelineProgram;
	// pass material data
	GLuint loc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "isLightingEnabled");
	glUniform1i(loc, int(useLight && SceneManager::getInstance()->isLightingEnabled));

	loc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "material.ambient");
	glUniform4f(loc, ambient[0], ambient[1], ambient[2], ambient[3]);
	loc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "material.diffuse");
	glUniform4f(loc, diffuse[0], diffuse[1], diffuse[2], diffuse[3]);
	loc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "material.specular");
	glUniform4f(loc, specular[0], specular[1], specular[2], specular[3]);
	loc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "material.shininess");
	glUniform1f(loc, shininess);

	// pass texture data
	loc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "textureImage2D");
	glUniform1i(loc, 0);
	loc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "textureImageCube");
	glUniform1i(loc, 1);

	if (texture) texture->load(pipelineProgram);

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

void Renderer::onUpdate() {
	// renderers are updated by SceneManager
}
#pragma endregion

#pragma region Physics
Physics::Physics(bool checkGround, float minDistance) {
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

	if (checkGround && position.y <= minY) {
		velocity.y = 0;
		position.y = minY;
		isOnGround = true;
	}

	entity->transform->setPosition(position, true);
}
#pragma endregion

#pragma region Camera
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
void Camera::view() {
	if (!isCurrentCamera()) return;

	// update view matrix
	vec3 position = entity->transform->getPosition(true);
	vec3 center = position + entity->transform->getForwardVector(true);
	vec3 up = entity->transform->getUpVector(true);
	viewMatrix = lookAt(position, center, up);
}
void Camera::onUpdate() {
	// cameras are updated by SceneManager
}
#pragma endregion

#pragma region Light
void Light::sendData(GLuint pipelineHandle) {
	GLuint loc;
	if (directionalLight) {
		vec3 direction = directionalLight->direction;
		vec4 ambient = directionalLight->ambient;
		vec4 diffuse = directionalLight->diffuse;
		vec4 specular = directionalLight->specular;

		loc = glGetUniformLocation(pipelineHandle, "directionalLight.direction");
		glUniform3f(loc, direction.x, direction.y, direction.z);
		loc = glGetUniformLocation(pipelineHandle, "directionalLight.ambient");
		glUniform4f(loc, ambient.x, ambient.y, ambient.z, ambient.w);
		loc = glGetUniformLocation(pipelineHandle, "directionalLight.diffuse");
		glUniform4f(loc, diffuse.x, diffuse.y, diffuse.z, diffuse.w);
		loc = glGetUniformLocation(pipelineHandle, "directionalLight.specular");
		glUniform4f(loc, specular.x, specular.y, specular.z, specular.w);
	}

	loc = glGetUniformLocation(pipelineHandle, "numOfPointLights");
	glUniform1i(loc, pointLights.size());
	for (unsigned int i = 0; i < pointLights.size(); i++) {
		PointLight* pointLight = pointLights[i];
		string target = "pointLights[" + to_string(i) + "]";

		vec3 position = pointLight->getEntity()->transform->getPosition(true);
		vec4 ambient = pointLight->ambient;
		vec4 diffuse = pointLight->diffuse;
		vec4 specular = pointLight->specular;
		vec3 attenuation = pointLight->attenuation;

		loc = glGetUniformLocation(pipelineHandle, (target + ".position").c_str());
		glUniform3f(loc, position.x, position.y, position.z);
		loc = glGetUniformLocation(pipelineHandle, (target + ".ambient").c_str());
		glUniform4f(loc, ambient.x, ambient.y, ambient.z, ambient.w);
		loc = glGetUniformLocation(pipelineHandle, (target + ".diffuse").c_str());
		glUniform4f(loc, diffuse.x, diffuse.y, diffuse.z, diffuse.w);
		loc = glGetUniformLocation(pipelineHandle, (target + ".specular").c_str());
		glUniform4f(loc, specular.x, specular.y, specular.z, specular.w);
		loc = glGetUniformLocation(pipelineHandle, (target + ".attenuation").c_str());
		glUniform3f(loc, attenuation.x, attenuation.y, attenuation.z);
	}
}
Light* Light::getDirectionalLight() {
	return directionalLight;
}
vector<PointLight*> Light::getPointLights() {
	return pointLights;
}
void Light::onUpdate() {

}
DirectionalLight::DirectionalLight(vec3 direction) {
	this->direction = direction;
	if (directionalLight) {
		printf("***Warning: already has a directional light on \"%s\"", directionalLight->entity->name.c_str());
	}
	directionalLight = this;
}
PointLight::PointLight(vec3 attenuation) {
	this->attenuation = attenuation;
	pointLights.push_back(this);
}
#pragma endregion

#pragma region PlayerController
PlayerController::PlayerController() {

}
void PlayerController::move(vec4 input, float verticalMove) {
	if (!isActivated) return;

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
	if (!isActivated) return;

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
	if (!pipelineProgram) {
		printf("!!!Error: pipeline program cannot be null. \n");
		return;
	}
	this->pipelineProgram = pipelineProgram;
	// generate vertex array
	glGenVertexArrays(1, &vertexArray);
}
VertexArrayObject::VertexArrayObject(BasicPipelineProgram* pipelineProgram,
									 vector<vec3> positions,
									 vector<vec4> colors,
									 vector<int> indices) : VertexArrayObject(pipelineProgram) {
	setPositions(positions);
	setColors(colors);
	setIndices(indices);
}
void VertexArrayObject::bindPipeline() {
	pipelineProgram->Bind();
}
void VertexArrayObject::setPositions(vector<vec3> positions) {
	numVertices = positions.size();
	if (numVertices == 0) {
		printf("!!!Error: the number of vertices cannot be 0. \n");
		return;
	}
	glBindVertexArray(vertexArray);

	// position data
	glGenBuffers(1, &positionBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * numVertices, &positions[0], GL_STATIC_DRAW);

	// position attribute
	GLuint loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
	glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);
}
void VertexArrayObject::setColors(vector<vec4> colors) {
	numColors = colors.size();
	if (numColors == 0) {
		printf("!!!Error: the number of colors cannot be 0. \n");
		return;
	}
	glBindVertexArray(vertexArray);

	// color data
	glGenBuffers(1, &colorBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec4) * numColors, &colors[0], GL_STATIC_DRAW);

	// color attribute
	GLuint loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "color");
	glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, (const void*)0);
}
void VertexArrayObject::setIndices(vector<int> indices) {
	numIndices = indices.size();
	if (numIndices == 0) {
		printf("!!!Error: the number of indices cannot be 0. \n");
		return;
	}
	glBindVertexArray(vertexArray);

	// index data
	glGenBuffers(1, &indexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int) * numIndices, &indices[0], GL_STATIC_DRAW);
}
void VertexArrayObject::setNormals(vector<vec3> normals) {
	numNormals = normals.size();
	if (numNormals == 0) {
		printf("!!!Error: the number of normals cannot be 0. \n");
		return;
	}if (numVertices != numNormals) {
		printf("!!!Error: the number of vertices %i does not match the number of normals %i. \n", numVertices, numNormals);
		return;
	}
	glBindVertexArray(vertexArray);

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
		return;
	}
	glBindVertexArray(vertexArray);

	glGenBuffers(1, &texCoordBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, texCoordBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec2) * numTexCoords, &texCoords[0], GL_STATIC_DRAW);

	GLuint loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "texCoord");
	glBindBuffer(GL_ARRAY_BUFFER, texCoordBuffer);
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, 0, (const void*)0);
}
void VertexArrayObject::draw(float* m, float* v, float* p, float* n, GLenum drawMode) {
	if (numVertices != numColors) {
		printf("***Warning: the number of vertices %i is not the same as the number of colors %i. \n", numVertices, numColors);
	}
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
RollerCoaster::RollerCoaster(vector<Spline> splines, bool closedPath, float scale, float maxLineLength) {
	this->closedPath = closedPath;
	this->maxLineLength = maxLineLength;

	Spline spline;
	vec3 offset = vec3(0);

	// put all splines together
	spline.points.push_back(scale * splines[0].points[0] + offset);
	spline.numControlPoints++;
	for (unsigned int i = 0; i < splines.size(); i++) {
		if (i > 0) offset -= splines[i].points[0];

		spline.numControlPoints += splines[i].numControlPoints;
		for (int j = 0; j < splines[i].numControlPoints; j++) {
			spline.points.push_back(scale * splines[i].points[j] + offset);

			if (i == splines.size() - 1 && j == splines[i].numControlPoints - 1) {
				if (closedPath) {
					spline.points.push_back(scale * splines[0].points[0] + offset);
					spline.points.push_back(scale * splines[0].points[0] + offset);
					spline.numControlPoints += 2;
				}
				else {
					spline.points.push_back(scale * splines[i].points[j] + offset);
					spline.numControlPoints++;
				}
			}
		}
		offset = splines[i].points[splines[i].numControlPoints - 1];
	}
	this->spline = spline;

	// calculate data from spline
	for (int j = 1; j < spline.numControlPoints - 2; j++) {
		if (j == spline.numControlPoints - 4) {
			startClosingIndex = vertexPositions.size() - 1;
		}
		mat3x4 control = mat3x4(
			spline.points[j - 1].x, spline.points[j].x, spline.points[j + 1].x, spline.points[j + 2].x,
			spline.points[j - 1].y, spline.points[j].y, spline.points[j + 1].y, spline.points[j + 2].y,
			spline.points[j - 1].z, spline.points[j].z, spline.points[j + 1].z, spline.points[j + 2].z
		);

		subdivide(0, 1, maxLineLength, control);
	}
	currentVertexIndex = 0;
	numOfVertices = vertexPositions.size();

	// calculate distances
	for (unsigned int i = 0; i < vertexPositions.size() - 1; i++) {
		float distanceToNext = distance(vertexPositions[i], vertexPositions[i + 1]);
		vertexDistances.push_back(distanceToNext);
	}
	vertexDistances.push_back(0);

	// deactivated at defualt
	setActive(false);
}	
vec3 RollerCoaster::getStartPosition() {
	return vertexPositions[0] + 3.0f * cross(vertexTangents[0], vertexNormals[0]) + entity->transform->getPosition(true);;
}
vec3 RollerCoaster::getCurrentPosition() {
	vec3 position;
	if (currentVertexIndex == numOfVertices - 1) {
		position = vertexPositions[currentVertexIndex];
	}
	else {
		position = mix(vertexPositions[currentVertexIndex], vertexPositions[currentVertexIndex + 1], currentSegmentProgress);
	}
	position += 0.7f * vertexNormals[currentVertexIndex];
	position += entity->transform->getPosition(true);
	return position;
}
void RollerCoaster::subdivide(float u0, float u1, float maxLength, mat3x4 control) {
	float umid = (u0 + u1) / 2;

	vec4 uVector0(u0 * u0 * u0, u0 * u0, u0, 1);
	vec3 point0 = uVector0 * BASIS * control;
	vec4 uVector1(u1 * u1 * u1, u1 * u1, u1, 1);
	vec3 point1 = uVector1 * BASIS * control;
	if (distance(point0, point1) > maxLength) {
		subdivide(u0, umid, maxLength, control);
		subdivide(umid, u1, maxLength, control);
	}
	else {
		vertexPositions.push_back(point0);
		vertexPositions.push_back(point1);

		vec4 uPrime0(3 * u0 * u0, 2 * u0, 1, 0);
		vec4 uPrime1(3 * u1 * u1, 2 * u1, 1, 0);
		vertexTangents.push_back(normalize(uPrime0 * BASIS * control));
		vertexTangents.push_back(normalize(uPrime1 * BASIS * control));
	}
}
vec3 RollerCoaster::getCurrentDirection() {
	return vertexTangents[currentVertexIndex];
}
vec3 RollerCoaster::getCurrentNormal() {
	return vertexNormals[currentVertexIndex];
}
void RollerCoaster::start(bool isRepeating) {
	this->isRepeating = isRepeating;
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
void RollerCoaster::render(vec3 normal, vec4 crossbarColor, vec4 trackColor, vec4 saddleColor, vec4 backColor) {
	Renderer* renderer = entity->getComponent<Renderer>();
	if (!renderer) {
		printf("!!!Error: cannot find Renderer to render the RollerCoaster. \n");
		return;
	}
	printf("Rendering roller-coaster \"%s\"...\n", entity->name.c_str());

	normal = normalize(normal);
	vector<vec3> pointNormals;
	vector<vec3> pointBinormals;
	for (int i = 0; i < numOfVertices; i++) {
		vec3 p = vertexPositions[i];
		vec3 t = vertexTangents[i];
		vec3 n;
		if (i == 0) {
			n = normal;
		}
		else if (closedPath && i >= startClosingIndex) { // interpolates normals
			n = mix(pointNormals[startClosingIndex - 1], pointNormals[0],
					(i - startClosingIndex) / (float)(numOfVertices - startClosingIndex));
		}
		else {
			n = normalize(cross(pointBinormals[i - 1], t));
		}
		vec3 b = normalize(cross(t, n));
		pointNormals.push_back(n);
		pointBinormals.push_back(b);

		if (i % 50 == 0) {
			Entity* crossbar = SceneManager::getInstance()->createEntity("Crossbar_" + to_string(i));
			crossbar->addComponent(new Renderer(renderer->vao->pipelineProgram, makeCube(1.0f, 0.5f, 0.1f), crossbarColor));
			crossbar->transform->setPosition(p, true);
			crossbar->transform->faceTo(crossbar->transform->getPosition(true) + t, n);
			crossbar->setParent(entity);
		}
	}
	vertexNormals = pointNormals;
	
	makeTrack(0.3f, 0.05f, -0.5f, trackColor);
	makeTrack(0.3f, 0.05f, 0.5f, trackColor);

	// set up seat
	seat = SceneManager::getInstance()->createEntity("Seat");
	seat->setParent(entity);
	moveSeat();

	Entity* saddle = SceneManager::getInstance()->createEntity("Saddle");
	saddle->addComponent(new Renderer(renderer->vao->pipelineProgram, makeCube(1.25f, 1, 0.2f), saddleColor));
	saddle->transform->setPosition(vec3(0, -0.55, 0), false);
	saddle->setParent(seat);

	Entity* back = SceneManager::getInstance()->createEntity("Back");
	back->addComponent(new Renderer(renderer->vao->pipelineProgram, makeCube(1, 0.1f, 1), backColor));
	back->transform->setPosition(vec3(0, 0, 0.45), false);
	back->setParent(seat);
}
void RollerCoaster::makeTrack(float width, float height, float offset, vec4 color) {
	vector<vec3> positions;
	vector<vec4> colors;
	vector<int> indices;
	vector<vec3> normals;

	float x = width / 2.0f;
	float y = height / 2.0f;
	Entity* track = SceneManager::getInstance()->createEntity("Track");
	// calculate normals and binormals
	for (int i = 0; i < numOfVertices; i++) {
		vec3 p = vertexPositions[i];
		vec3 t = vertexTangents[i];
		vec3 n = vertexNormals[i];
		vec3 b = normalize(cross(t, n));

		// calculate positions, colors and indices
		vec3 v0 = p + y * (-n) + x * b - b * offset;
		vec3 v1 = p + y * n + x * b - b * offset;
		vec3 v2 = p + y * n - x * b - b * offset;
		vec3 v3 = p + y * (-n) + x * (-b) - b * offset;

		if (closedPath) { // don't need to draw the cross-section faces at the start and end
			positions.insert(positions.end(), { v0, v1, v1, v2, v2, v3, v3, v0 });
			colors.insert(colors.end(), { color, color, color, color, color, color, color, color });
			normals.insert(normals.end(), { b, b, n, n, -b, -b, -n, -n });
			if (i > 0) {
				int index = 8 * (i - 1);
				indices.insert(indices.end(),
							   { index, index + 8, index + 1, index + 1, index + 8, index + 9,
							   index + 2, index + 10, index + 3, index + 3, index + 10, index + 11,
							   index + 4, index + 12, index + 5, index + 5, index + 12, index + 13,
							   index + 6, index + 14, index + 7, index + 7, index + 14, index + 15 });
			}
		}
		else { //draw the cross-section faces at the start and end
			if (i == 0) {
				positions.insert(positions.end(), { v0, v1, v2, v3 });
				colors.insert(colors.end(), { color, color, color, color });

				int index = 0;
				indices.insert(indices.end(), { index, index + 1, index + 2, index, index + 2, index + 3 });
				normals.insert(normals.end(), { -t, -t, -t, -t });
			}

			positions.insert(positions.end(), { v0, v1, v1, v2, v2, v3, v3, v0 });
			colors.insert(colors.end(), { color, color, color, color, color, color, color, color });
			normals.insert(normals.end(), { b, b, n, n, -b, -b, -n, -n });
			if (i > 0) {
				int index = 8 * (i - 1) + 4;
				indices.insert(indices.end(),
							   { index, index + 8, index + 1, index + 1, index + 8, index + 9,
							   index + 2, index + 10, index + 3, index + 3, index + 10, index + 11,
							   index + 4, index + 12, index + 5, index + 5, index + 12, index + 13,
							   index + 6, index + 14, index + 7, index + 7, index + 14, index + 15 });
			}
			if (i == numOfVertices - 1) {
				positions.insert(positions.end(), { v0, v1, v2, v3 });
				colors.insert(colors.end(), { color, color, color, color });

				int index = 8 * i + 12;
				indices.insert(indices.end(), { index, index + 1, index + 2, index, index + 3, index + 2 });
				normals.insert(normals.end(), { t, t, t, t });
			}
		}
	}

	VertexArrayObject* vao = new VertexArrayObject(entity->getComponent<Renderer>()->vao->pipelineProgram);
	vao->setPositions(positions);
	vao->setColors(colors);
	vao->setIndices(indices);
	vao->setNormals(normals);
	Renderer* renderer = new Renderer(vao, GL_TRIANGLES);
	track->addComponent(renderer);
	track->transform->setPosition(vec3(0, 0, 0), false);
	track->setParent(entity);
}
void RollerCoaster::moveSeat() {
	seat->transform->setPosition(getCurrentPosition(), true);
	seat->transform->faceTo(seat->transform->getPosition(true) + getCurrentDirection(), getCurrentNormal());
}
void RollerCoaster::onUpdate() {
	// physical calculation
	float deltaTime = Timer::getInstance()->getDeltaTime();
	float drag = dot(-vertexTangents[currentVertexIndex], Physics::GRAVITY);
	speed -= drag * deltaTime;
	if (speed < minSpeed) {
		speed = minSpeed;
	}
	float step = speed * deltaTime;

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
				start();
			}
		}
	}
	currentSegmentProgress += step / vertexDistances[currentVertexIndex];
	moveSeat();
}
#pragma endregion