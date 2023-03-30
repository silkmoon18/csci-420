#include "utility.h"


random_device rd;
mt19937 eng;  // or eng(r()); for non-deterministic random number
uniform_real_distribution<double> distrib(0.0, 1.0 - 1e-8);




void printProgress(Scene* scene) {
	string info;
	float deltaTime = 0.0f;
	while (true) {
		Timer::getInstance()->update();

		deltaTime += Timer::getInstance()->getDeltaTime();
		if (deltaTime < 1.0f) continue;

		deltaTime = 0.0f;
		printf("\r%s", string(info.length(), ' ').c_str());

		info = scene->getProgressInfo();
		printf("\r%s", info.c_str());

		scene->display();
		if (info.empty()) {
			printf("Done\n");
			break;
		}
	}
}
void parse_check(const char* expected, char* found) {
	if (strcasecmp(expected, found)) {
		printf("Expected '%s ' found '%s '\n", expected, found);
		printf("Parse error, abnormal abortion\n");
		exit(0);
	}
}
void parse_vec3(FILE* file, const char* check, vec3& vec) {
	char str[100];
	fscanf(file, "%s", str);
	parse_check(check, str);
	fscanf(file, "%f %f %f", &vec.x, &vec.y, &vec.z);
}
void parse_float(FILE* file, const char* check, float& f) {
	char str[512];
	int ret = fscanf(file, "%s", str);
	ASERT(ret == 1);
	parse_check(check, str);
	ret = fscanf(file, "%f", &f);
	ASERT(ret == 1);
}
void parse_rad(FILE* file, float* r) {
	char str[100];
	fscanf(file, "%s", str);
	parse_check("rad:", str);
	fscanf(file, "%f", r);
}
void parse_shi(FILE* file, float* shi) {
	char s[100];
	fscanf(file, "%s", s);
	parse_check("shi:", s);
	fscanf(file, "%f", shi);
}


int isPositive(float number) {
	if (number > 0) return 1;
	else return 0;
}
int sign(float number) {
	if (number > 0) return 1;
	if (number < 0) return -1;
	return 0;
}
float getRandom() {
	return distrib(eng);
}
float getRandom(float min, float max) {
	float f = getRandom();
	return min + f * (max - min);
}
vec3 getRandom(vec3 min, vec3 max) {
	return vec3(getRandom(min.x, max.x),
				getRandom(min.y, max.y),
				getRandom(min.z, max.z));
}
float calculateArea(vec3 a, vec3 b, vec3 c) {
	return 0.5f * (((b.x - a.x) * (c.y - a.y)) - ((c.x - a.x) * (b.y - a.y)));
}
int compare(float f1, float f2) {
	int result = 1;
	float diff = f1 - f2;
	if (diff < 0) result = -1;
	if (abs(diff) <= EPSILON) result = 0;
	return result;
}





#pragma region Timer
float Timer::getDeltaTime() {
	return deltaTime;
}
void Timer::update() {
	float currentTime = getCurrentTime();
	deltaTime = currentTime - previousTime;
	previousTime = currentTime;
}
float Timer::getCurrentTime() {
	return glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
}
#pragma endregion


#pragma region Scene
const vector<Object*>& Scene::getObjects() { return objects; }
const vector<Triangle*>& Scene::getTriangles() { return triangles; }
const vector<Sphere*>& Scene::getSpheres() { return spheres; }
const vector<Light*>& Scene::getLights() { return lights; }
void Scene::setAntiAliasingLevel(int antiAliasingLevel) {
	numOfSubpixelsPerSide = std::max(0, (int)pow(2, std::max(0, antiAliasingLevel)));
}
void Scene::setSoftShadowLevel(int softShadowLevel) {
	numOfSampleLights = std::max(1, (int)pow(2, std::max(0, softShadowLevel)));
}
void Scene::setNumOfThreads(int num) {
	numOfThreads = std::max(1, num);
}
void Scene::render() {
	initializePixels();
	sampleLights();

	startTime = Timer::getInstance()->getCurrentTime();
	printf("Rendering %s... \n", inputFilename);
	process();

	save();
}
void Scene::display() {
	// display only on display mode
	if (mode != MODE_DISPLAY) return;

	glPointSize(2.0);
	glBegin(GL_POINTS);
	for (auto& pixel : pixels) {
		glColor3f(pixel.color.x, pixel.color.y, pixel.color.z);
		glVertex2i(pixel.index.x, pixel.index.y);
	}
	glEnd();
	glFlush();
}
void Scene::save() {
	// always save
	filesystem::path inputPath = string(inputFilename);
	char outputFilename[100];
	sprintf(outputFilename, "%s\\%s-aa%d-ls%d.jpg",
			inputPath.parent_path().string().c_str(), 
			inputPath.stem().string().c_str(), 
			numOfSubpixelsPerSide, 
			numOfSampleLights);

	printf("Saving JPEG file: %s\n", outputFilename);
	for (auto& pixel : pixels) {
		vec3 color = clamp(pixel.color * 255.0f, vec3(0.0f), vec3(255.0f));
		buffer[pixel.index.y][pixel.index.x][0] = (int)color.x;
		buffer[pixel.index.y][pixel.index.x][1] = (int)color.y;
		buffer[pixel.index.y][pixel.index.x][2] = (int)color.z;
	}

	ImageIO img(WIDTH, HEIGHT, 3, &buffer[0][0][0]);
	if (img.save(outputFilename, ImageIO::FORMAT_JPEG) != ImageIO::OK)
		printf("Error in Saving\n");
	else
		printf("File saved Successfully\n");
}
void Scene::clear() {
	for (auto& obj : objects) {
		delete obj;
	}
	objects.clear();
	spheres.clear();
	triangles.clear();

	for (auto& l : lights) {
		delete l;
	}
	lights.clear();

	numOfCompletedPixels = 0;
}
string Scene::getProgressInfo() {
	string info;

	int progress = (float)numOfCompletedPixels / pixels.size() * 100;
	if (progress < 100) {
		float speed = numOfCompletedPixels / (Timer::getInstance()->getCurrentTime() - startTime);
		int time = std::max(0, int((pixels.size() - numOfCompletedPixels) / speed));
		int minutes = time / 60;
		int seconds = time % 60;

		char buffer[200];
		sprintf(buffer, "\r%d%% (%d / %lu), speed: %d pixels / s, %d Minutes %d Seconds remaining",
				progress, (int)numOfCompletedPixels, pixels.size(), (int)speed, minutes, seconds);
		info = string(buffer);
	}

	return info;
}
void Scene::sampleLights() {
	vector<Light*> lightSamples;
	for (int i = 0; i < lights.size(); i++) {
		auto samples = lights[i]->getSamples(numOfSampleLights);
		for (auto& s : samples) {
			lightSamples.push_back(s);
		}
	}
	lights = lightSamples;
}
void Scene::initializePixels() {
	vec3 startPosition;
	startPosition.z = -1;
	startPosition.y = -tan(radians(FOV / 2));
	startPosition.x = ASPECT_RATIO * startPosition.y;
	float pixelSize = abs(2 * startPosition.y / HEIGHT); // calculate pixel size
	startPosition -= vec3(pixelSize / 2, pixelSize / 2, 0);

	Pixel base;
	base.position = startPosition;
	for (unsigned int x = 0; x < WIDTH; x++) {
		base.position.x += pixelSize;
		base.position.y = startPosition.y;

		for (unsigned int y = 0; y < HEIGHT; y++) {
			base.position.y += pixelSize;

			Pixel pixel;
			pixel.position = base.position;
			pixel.index = vec2(x, y);
			pixel.size = pixelSize;
			pixel.color = vec3(0);
			pixels.push_back(pixel);
		}
	}
}
void Scene::drawPixels(int threadIndex) {
	int pixelsPerThread = pixels.size() / numOfThreads;
	int start = threadIndex * pixelsPerThread;
	int end = (threadIndex + 1) * pixelsPerThread;

	for (int i = start; i < end; i++) {
		calculatePixelColor(pixels[i]);
		numOfCompletedPixels++;
	}
}
void Scene::process() {
	vector<thread> threads;
	for (int i = 0; i < numOfThreads; i++) {
		threads.emplace_back(&Scene::drawPixels, this, i);
	}
	printProgress(this);
	for (auto& thread : threads) {
		thread.join();
	}
}
#pragma endregion


#pragma region PhongScene
int PhongScene::load(const char* argv) {
	FILE* file = fopen(argv, "r");
	inputFilename = (char*)argv;
	int number_of_objects;
	char type[50];
	fscanf(file, "%i", &number_of_objects);

	printf("\nNumber of objects: %i\n", number_of_objects);

	parse_vec3(file, "amb:", ambient_light);

	for (int i = 0; i < number_of_objects; i++) {
		fscanf(file, "%s\n", type);
		if (strcasecmp(type, "triangle") == 0) {
			Triangle* t = parseTriangle(file);

			if (triangles.size() == MAX_TRIANGLES) {
				printf("too many triangles, you should increase MAX_TRIANGLES!\n");
				exit(0);
			}
			triangles.push_back(t);
			objects.push_back(t);
		}
		else if (strcasecmp(type, "sphere") == 0) {
			Sphere* s = parseSphere(file);

			if (spheres.size() == MAX_SPHERES) {
				printf("too many spheres, you should increase MAX_SPHERES!\n");
				exit(0);
			}
			spheres.push_back(s);
			objects.push_back(s);
		}
		else if (strcasecmp(type, "light") == 0) {
			Light* l = parseLight(file);

			if (lights.size() == MAX_LIGHTS) {
				printf("too many lights, you should increase MAX_LIGHTS!\n");
				exit(0);
			}
			lights.push_back(l);
		}
		else {
			printf("unknown type in scene description:\n%s\n", type);
			exit(0);
		}
	}

	return 0;
}
void PhongScene::calculatePixelColor(Pixel& pixel) {
	pixel.color = superSample(numOfSubpixelsPerSide, pixel.size, pixel.position);
}
Triangle* PhongScene::parseTriangle(FILE* file) {
	vec3 position, normal, diffuse, specular;
	float shininess;
	vector<Vertex> vertices;
	for (int j = 0; j < 3; j++) {
		parse_vec3(file, "pos:", position);
		parse_vec3(file, "nor:", normal);
		parse_vec3(file, "dif:", diffuse);
		parse_vec3(file, "spe:", specular);
		parse_shi(file, &shininess);
		Material* material = new PhongMaterial(normal, diffuse, specular, shininess);
		Vertex v;
		v.position = position;
		v.material = material;
		vertices.push_back(v);
	}
	return new Triangle(vertices);
}
Sphere* PhongScene::parseSphere(FILE* file) {
	vec3 position, diffuse, specular;
	float radius, shininess;
	parse_vec3(file, "pos:", position);
	parse_rad(file, &radius);
	parse_vec3(file, "dif:", diffuse);
	parse_vec3(file, "spe:", specular);
	parse_shi(file, &shininess);
	Material* material = new PhongMaterial(vec3(0), diffuse, specular, shininess);
	return new Sphere(position, radius, material);
}
Light* PhongScene::parseLight(FILE* file) {
	vec3 position, color;
	parse_vec3(file, "pos:", position);
	parse_vec3(file, "col:", color);
	return new Light(position, color);
}
vec3 PhongScene::superSample(int numOfSubpixelsPerSide, float pixelSize, vec3 pixelPosition) {
	vec3 color = ambient_light;

	float cellSize = pixelSize / numOfSubpixelsPerSide;
	vec3 startPosition = pixelPosition;
	float offset = 0.5f * cellSize * (numOfSubpixelsPerSide + 1);
	startPosition -= vec3(offset, offset, 0);

	vec3 cellPosition = startPosition;
	for (unsigned int x = 0; x < numOfSubpixelsPerSide; x++) {
		cellPosition.x += cellSize;
		cellPosition.y = startPosition.y;

		for (unsigned int y = 0; y < numOfSubpixelsPerSide; y++) {
			cellPosition.y += cellSize;

			Ray cameraRay(vec3(0), cellPosition);
			color += cameraRay.calculateRayColor(this);
		}
	}
	color /= numOfSubpixelsPerSide * numOfSubpixelsPerSide;
	return color;
}
#pragma endregion


#pragma region OpticalScene
int OpticalScene::load(const char* argv) {
	FILE* file = fopen(argv, "r");
	inputFilename = (char*)argv;
	int number_of_objects;
	char type[50];
	fscanf(file, "%i", &number_of_objects);

	printf("\nnumber of objects: %i\n", number_of_objects);

	parse_vec3(file, "amb:", ambient_light);
	parse_vec3(file, "f0:", F0);

	for (int i = 0; i < number_of_objects; i++) {
		fscanf(file, "%s\n", type);
		if (strcasecmp(type, "triangle") == 0) {
			Triangle* t = parseTriangle(file);

			if (triangles.size() == MAX_TRIANGLES) {
				printf("too many triangles, you should increase MAX_TRIANGLES!\n");
				exit(0);
			}
			triangles.push_back(t);
			objects.push_back(t);
		}
		else if (strcasecmp(type, "sphere") == 0) {
			Sphere* s = parseSphere(file);
			if (spheres.size() == MAX_SPHERES) {
				printf("too many spheres, you should increase MAX_SPHERES!\n");
				exit(0);
			}
			spheres.push_back(s);
			objects.push_back(s);
		}
		else if (strcasecmp(type, "light") == 0) {
			Light* l = parseLight(file);

			if (lights.size() == MAX_LIGHTS) {
				printf("too many lights, you should increase MAX_LIGHTS!\n");
				exit(0);
			}
			lights.push_back(l);
		}
		else {
			printf("unknown type in scene description:\n%s\n", type);
			exit(0);
		}
	}
	//lights = sampleLights();

	return 0;
}
void OpticalScene::calculatePixelColor(Pixel& pixel) {
	pixel.color = stratifiedSample(numOfSubpixelsPerSide, pixel.size, pixel.position);
	pixel.color /= numOfSampleLights;
	pixel.color /= pixel.color + 1.0f;
}
Triangle* OpticalScene::parseTriangle(FILE* file) {
	vec3 position, normal, diffuse;
	float roughness, metallic;
	vector<Vertex> vertices;
	for (int j = 0; j < 3; j++) {
		parse_vec3(file, "pos:", position);
		parse_vec3(file, "nor:", normal);
		parse_vec3(file, "dif:", diffuse);
		parse_float(file, "rou:", roughness);
		parse_float(file, "met:", metallic);
		Material* material = new OpticalMaterial(normal, diffuse, roughness, metallic);
		Vertex v;
		v.position = position;
		v.material = material;
		vertices.push_back(v);
	}
	return new Triangle(vertices);
}
Sphere* OpticalScene::parseSphere(FILE* file) {
	vec3 position, diffuse;
	float radius, roughness, metallic;
	parse_vec3(file, "pos:", position);
	parse_rad(file, &radius);
	parse_vec3(file, "dif:", diffuse);
	parse_float(file, "rou:", roughness);
	parse_float(file, "met:", metallic);
	Material* material = new OpticalMaterial(vec3(0), diffuse, roughness, metallic);
	return new Sphere(position, radius, material);
}
Light* OpticalScene::parseLight(FILE* file) {
	vec3 position, color, normal;
	vector<vec3> p(4);
	parse_vec3(file, "p0:", p[0]);
	parse_vec3(file, "p1:", p[1]);
	parse_vec3(file, "p2:", p[2]);
	parse_vec3(file, "p3:", p[3]);
	parse_vec3(file, "pos:", position);
	parse_vec3(file, "nrm:", normal);
	parse_vec3(file, "col:", color);
	return new Light(position, color, normal, p);
}
vec3 OpticalScene::stratifiedSample(int numOfSubpixelsPerSide, float pixelSize, vec3 pixelPosition) {
	vec3 color = ambient_light;

	float cellSize = pixelSize / numOfSubpixelsPerSide;
	vec3 startPosition = pixelPosition;
	float offset = pixelSize * 0.5f;
	startPosition -= vec3(offset, offset, 0);

	vec3 cellPosition = startPosition;
	for (unsigned int x = 0; x < numOfSubpixelsPerSide; x++) {
		cellPosition.x = startPosition.x + x * cellSize + getRandom(0, cellSize);

		for (unsigned int y = 0; y < numOfSubpixelsPerSide; y++) {
			cellPosition.y = startPosition.y + y * cellSize + getRandom(0, cellSize);

			Ray cameraRay(vec3(0), cellPosition);
			color += cameraRay.calculateRayColor(this);
		}
	}
	color /= numOfSubpixelsPerSide * numOfSubpixelsPerSide;
	return color;
}
#pragma endregion


#pragma region Material
Material::Material(vec3 normal, vec3 diffuse) {
	this->normal = normal;
	this->diffuse = diffuse;
	specular = vec3(0.0f);
	shininess = 0.0f;
	roughness = 0.0f;
	metallic = 0.0f;
}
#pragma endregion


#pragma region PhongMaterial
PhongMaterial::PhongMaterial(vec3 normal, vec3 diffuse, vec3 specular, float shininess)
	: Material(normal, diffuse) {
	this->specular = specular;
	this->shininess = shininess;
}
Material* PhongMaterial::clone() {
	return new PhongMaterial(normal, diffuse, specular, shininess);
}
vec3 PhongMaterial::calculateLighting(Scene* scene, Ray& ray, vec3 position) {
	vec3 color(0);

	float ks = (specular.x + specular.y + specular.z) / 3;

	vec3 local(0);
	auto lights = scene->getLights();
	for (int i = 0; i < lights.size(); i++) {
		Ray shadowRay(position, lights[i]->position);
		if (shadowRay.checkIfBlocked(scene->getObjects(), lights[i]->position)) continue;
		local += calculatePhongShading(position, lights[i]);
	}

	if (scene->isGlobalLightingEnabled) {
		ray.reflects(position, normal);
		vec3 reflection = ray.calculateRayColor(scene);
		color = (1 - ks) * local + ks * reflection;
	}
	else {
		color = local;
	}
	return color;
}
Material* PhongMaterial::interpolates(Material* m1, Material* m2, vec3 bary) {
	vec3 normal, diffuse, specular;
	float shininess = 0.0f;

	normal = normalize(vec3(bary.x * this->normal.x + bary.y * m1->normal.x + bary.z * m2->normal.x,
							bary.x * this->normal.y + bary.y * m1->normal.y + bary.z * m2->normal.y,
							bary.x * this->normal.z + bary.y * m1->normal.z + bary.z * m2->normal.z));

	diffuse.x = bary.x * this->diffuse.x + bary.y * m1->diffuse.x + bary.z * m2->diffuse.x;
	diffuse.y = bary.x * this->diffuse.y + bary.y * m1->diffuse.y + bary.z * m2->diffuse.y;
	diffuse.z = bary.x * this->diffuse[2] + bary.y * m1->diffuse[2] + bary.z * m2->diffuse[2];

	specular.x = bary.x * this->specular.x + bary.y * m1->specular.x + bary.z * m2->specular.x;
	specular.y = bary.x * this->specular.y + bary.y * m1->specular.y + bary.z * m2->specular.y;
	specular.z = bary.x * this->specular.z + bary.y * m1->specular.z + bary.z * m2->specular.z;

	shininess = bary.x * this->shininess + bary.y * m1->shininess + bary.z * m2->shininess;

	return new PhongMaterial(normal, diffuse, specular, shininess);
}
vec3 PhongMaterial::calculatePhongShading(vec3 position, Light* light) {
	vec3 lightVector = normalize(light->position - position);
	vec3 diffuseColor = diffuse;
	vec3 specularColor = specular;

	float ndotl = std::max(dot(lightVector, normal), 0.0f);
	vec3 diffuse = diffuseColor * ndotl;

	vec3 R = normalize(reflect(-lightVector, normal));
	vec3 eyeVector = normalize(-position);
	float rdotv = std::max(dot(R, eyeVector), 0.0f);
	vec3 specular = specularColor * (float)pow(rdotv, shininess);

	return light->color * (diffuse + specular);
}
#pragma endregion


#pragma region OpticalMaterial
OpticalMaterial::OpticalMaterial(vec3 normal, vec3 diffuse, float roughness, float metallic)
	: Material(normal, diffuse) {
	this->roughness = roughness;
	this->metallic = metallic;
}
Material* OpticalMaterial::clone() {
	return new OpticalMaterial(normal, diffuse, roughness, metallic);
}
vec3 OpticalMaterial::calculateLighting(Scene* scene, Ray& ray, vec3 position) {
	vec3 color(0);

	auto lights = scene->getLights();
	for (int i = 0; i < lights.size(); i++) {
		vec3 Le(0);

		vec3 w_i = normalize(lights[i]->position - position);
		if (dot(w_i, normal) > 0.0f) {
			Ray shadowRay(position, position + w_i);
			if (!shadowRay.checkIfBlocked(scene->getObjects(), lights[i]->position)) {
				Le = lights[i]->color;
			}
		}
		float pdf = pow(distance(position, lights[i]->position), 2) / (abs(dot(lights[i]->normal, w_i)) * lights[i]->area());

		float f0 = (scene->F0.x + scene->F0.y + scene->F0.z) / 3;
		color += BRDF(f0, Le, normal, pdf, position, w_i, -ray.direction);
	}
	return color;
}
Material* OpticalMaterial::interpolates(Material* m1, Material* m2, vec3 bary) {
	vec3 normal, diffuse;
	float roughness = 0.0f, metallic = 0.0f;

	normal = normalize(vec3(bary.x * this->normal.x + bary.y * m1->normal.x + bary.z * m2->normal.x,
							bary.x * this->normal.y + bary.y * m1->normal.y + bary.z * m2->normal.y,
							bary.x * this->normal.z + bary.y * m1->normal.z + bary.z * m2->normal.z));

	diffuse.x = bary.x * this->diffuse.x + bary.y * m1->diffuse.x + bary.z * m2->diffuse.x;
	diffuse.y = bary.x * this->diffuse.y + bary.y * m1->diffuse.y + bary.z * m2->diffuse.y;
	diffuse.z = bary.x * this->diffuse[2] + bary.y * m1->diffuse[2] + bary.z * m2->diffuse[2];

	roughness = bary.x * this->roughness + bary.y * m1->roughness + bary.z * m2->roughness;

	metallic = bary.x * this->metallic + bary.y * m1->metallic + bary.z * m2->metallic;

	return new OpticalMaterial(normal, diffuse, roughness, metallic);
}

vec3 OpticalMaterial::BRDF(float F0, vec3 Le, vec3 n, float pdf, vec3 p, vec3 w_i, vec3 w_o) {
	float alpha2 = pow(roughness * roughness, 2);

	float w_idotn = dot(w_i, n);
	float w_odotn = dot(w_o, n);

	vec3 h = (float)sign(w_idotn) * normalize(w_i + w_o);
	float w_idoth = dot(w_i, h);
	float w_odoth = dot(w_o, h);

	float F = F0 + (1 - F0) * pow((1 - w_odoth), 5);
	float FD90 = 2 * pow(w_idoth, 2) * roughness + 0.5;

	float theta_v_1 = angle(w_i, n);
	float G1_1 = isPositive(w_idoth / w_idotn) * 2 / (1 + sqrt(1 + alpha2 * pow(tan(theta_v_1), 2)));

	float theta_v_2 = angle(w_o, n);
	float G1_2 = isPositive(w_odoth / w_odotn) * 2 / (1 + sqrt(1 + alpha2 * pow(tan(theta_v_2), 2)));

	float G = G1_1 * G1_2;

	float theta_m = angle(h, n);
	float D = alpha2 * isPositive(dot(h, n)) / (PI * pow(cos(theta_m), 4) * pow(alpha2 + pow(tan(theta_m), 2), 2));

	float fs = F * G * D / (4 * abs(w_idotn) * abs(w_odotn));
	float fd =
		(1 / PI)
		* (1 + (FD90 - 1) * pow((1 - (w_idotn)), 5)
		   * (1 + (FD90 - 1) * pow((1 - (w_odotn)), 5)
			  * (1 - metallic)));

	vec3 f = (fs + fd) * diffuse;

	return Le * f * w_idotn / pdf;
}
#pragma endregion


#pragma region Triangles
vector<Vertex> Triangle::getVertices() { return vertices; }
Triangle::Triangle(vector<Vertex> vertices) {
	this->vertices = vertices;
}
Material* Triangle::getMaterial(vec3 position) {
	Material* m0 = vertices[0].material;
	Material* m1 = vertices[1].material;
	Material* m2 = vertices[2].material;
	vec3 bary = getBarycentricCoords(position);
	return m0->interpolates(m1, m2, bary);
}
float Triangle::intersects(Ray* ray) {
	vec3 a = vertices[0].position;
	vec3 b = vertices[1].position;
	vec3 c = vertices[2].position;

	vec3 n = cross(b - a, c - a);
	vec3 n_normalized = normalize(n);

	double ndotd = dot(n_normalized, ray->direction);
	if (compare(ndotd, 0) == 0) return -1;

	double d = -dot(n_normalized, a);
	double t = -(dot(n_normalized, ray->start) + d) / ndotd;
	//double  t = -(dot((start - a), n_normalized) / (dot(n_normalized, direction)));
	if (compare(t, 0) <= 0) return -1;

	vec3 p = ray->getPosition(t);
	if (dot(n_normalized, cross(b - a, p - a)) < 0 ||
		dot(n_normalized, cross(c - b, p - b)) < 0 ||
		dot(n_normalized, cross(a - c, p - c)) < 0) return -1;

	return t;
}
vec3 Triangle::getBarycentricCoords(vec3 p) {
	vec3 a = vertices[0].position;
	vec3 b = vertices[1].position;
	vec3 c = vertices[2].position;
	float area = std::max(EPSILON, calculateArea(a, b, c));

	float alpha = calculateArea(p, b, c) / area;
	float beta = calculateArea(a, p, c) / area;
	float gamma = 1 - alpha - beta;
	return vec3(alpha, beta, gamma);
}
#pragma endregion


#pragma region Sphere
Sphere::Sphere(vec3 position, float radius, Material* material) {
	this->position = position;
	this->radius = radius;
	this->baseMaterial = material;
}
Material* Sphere::getMaterial(vec3 position) {
	Material* material = baseMaterial->clone();
	material->normal = normalize(position - this->position);
	return material;
}
float Sphere::intersects(Ray* ray) {
	vec3 difference = ray->start - position;

	double a = 1;
	double b = 2 * dot(difference, ray->direction);
	double c = dot(difference, difference) - radius * radius;

	double checker = b * b - 4 * c;
	if (checker < 0) return -1;

	double t0 = 0.5 * (-b + sqrt(checker));
	double t1 = 0.5 * (-b - sqrt(checker));
	float t = std::min(t0, t1);
	if (compare(t, 0) <= 0) return -1;

	return t;
}
#pragma endregion


#pragma region Light
Light::Light(vec3 position, vec3 color, vec3 normal, vector<vec3> p) {
	this->position = position;
	this->color = color;
	this->normal = normal;
	this->p = p;
}
float Light::area() {
	if (p.size() < 4) return 0;
	return distance(p[1], p[0]) * distance(p[0], p[2]);
}
vector<Light*> Light::getSamples(int numOfSamples) {
	vector<Light*> samples;

	if (numOfSamples <= 1) {
		samples.push_back(this);
	}
	else {
		for (int j = 0; j < numOfSamples; j++) {
			vec3 pos;
			float U1 = getRandom();
			float U2 = getRandom();
			if (p.size() < 3) {
				float U3 = getRandom();
				U1 -= 0.5f;
				U2 -= 0.5f;
				U3 -= 0.5f;
				pos = position + vec3(U1, U2, U3);
			}
			else {
				vec3 p0 = p[0];
				vec3 p1 = p[1];
				vec3 p2 = p[2];
				vec3 p3 = p[3];
				pos = (1 - U1) * (p0 * (1 - U2) + p1 * U2) + U1 * (p2 * (1 - U2) + p3 * U2);
			}
			Light* sample = new Light(pos, color / (float)numOfSamples, normal, p);
			samples.push_back(sample);
		}
	}
	return samples;
}
#pragma endregion


#pragma region Ray
Ray::Ray(vec3 start, vec3 target) {
	this->start = start;
	direction = normalize(target - start);
}
vec3 Ray::getPosition(float t) {
	return start + direction * t;
}
Object* Ray::getFirstIntersectedObject(const vector<Object*>& objects, vec3& intersectedPosition) {
	Object* object = nullptr;
	float min_t = numeric_limits<float>::max();

	for (int i = 0; i < objects.size(); i++) {
		float t = objects[i]->intersects(this);
		if (t > 0 && t < min_t) {
			min_t = t;
			object = objects[i];
		}
	}
	intersectedPosition = getPosition(min_t);
	return object;
}
bool Ray::checkIfBlocked(const vector<Object*>& objects, vec3 end) {
	float target_t = length(end - start);
	for (int i = 0; i < objects.size(); i++) {
		float t = objects[i]->intersects(this);
		if (t > EPSILON && t <= target_t) {
			return true;
		}
	}
	return false;
}
void Ray::reflects(vec3 position, vec3 normal) {
	if (numOfReflection > MAX_REFLECTION) return;
	start = position;
	direction = normalize(reflect(direction, normal));
	numOfReflection++;
}
vec3 Ray::calculateRayColor(Scene* scene) {
	if (numOfReflection > MAX_REFLECTION) return vec3(0);

	vec3 color(0);
	vec3 position;
	Object* object = getFirstIntersectedObject(scene->getObjects(), position);
	if (object) {
		Material* material = object->getMaterial(position);
		color += material->calculateLighting(scene, *this, position);
		delete material;
	}
	else {
		color = scene->backgroundColor;
	}
	return color;
}
#pragma endregion