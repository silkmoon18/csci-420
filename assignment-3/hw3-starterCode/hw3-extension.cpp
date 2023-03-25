///* **************************
// * CSCI 420
// * Assignment 3 Raytracer
// * Name: <Your name here>
// * *************************
//*/
//
//#include "utility.h"
//
//vec3 F0;
//
//
//vec3 calculateSSAA(int gridSize, float cellSize, vec3 pixelPosition);
//void calculateRayColor(vec3& finalColor, Ray& ray, int numOfReflection);
//vec3 calculatePhongShading(vec3 position, Light* light, Material material);
//
//struct Material {
//	vec3 normal;
//	vec3 diffuse;
//	float roughness;
//	float metallic;
//
//
//	float BRDF(vec3 p, vec3 w_i, vec3 w_o) {
//		float alpha = roughness * roughness;
//	}
//};
//
//struct Vertex {
//	vec3 position;
//	Material material;
//};
//
//class Object {
//public:
//	virtual Material getMaterial(vec3 position) = 0;
//	virtual float intersects(Ray* ray) = 0;
//};
//class Triangle : public Object {
//public:
//	Vertex v[3];
//
//	Material getMaterial(vec3 position) override;
//	float intersects(Ray* ray) override;
//
//private:
//	vec3 getBarycentricCoords(vec3 p);
//};
//
//class Sphere : public Object {
//public:
//	vec3 position;
//	float radius;
//	Material baseMaterial;
//
//	Material getMaterial(vec3 position) override;
//	float intersects(Ray* ray) override;
//};
//
//class Light {
//public:
//	vec3 position;
//	vec3 normal;
//	vec3 color;
//	vec3 p[4];
//
//	Light() {}
//	Light(vec3 position, vec3 color) {
//		this->position = position;
//		this->color = color;
//	}
//};
//
//class Ray {
//public:
//	vec3 start;
//	vec3 direction;
//
//	Ray(vec3 start, vec3 target);
//
//	vec3 getPosition(float t);
//	Object* getFirstIntersectedObject(vec3& position);
//	bool checkIfBlocked();
//	bool checkIfBlocked(vec3 target);
//};
//
//vector<Object*> objects;
//vector<Triangle*> triangles;
//vector<Sphere*> spheres;
//vector<Light*> lights;
//
//vec3 ambient_light;
//vec3 backgroundColor = vec3(0.93f, 0.93f, 0.95f);
////vec3 backgroundColor = vec3(0);
//
//bool useGlobalLighting = true;
//bool useSSAA = false;
//int numOfSubpixelsPerSide = 3;
//
//
//void draw_scene() {
//	//debug 
//	useSSAA = false;
//	useGlobalLighting = true;
//
//	// calculate pixel start position, which is at x = -1, y = -1 (outside of the image).
//	vec3 startPosition;
//	startPosition.z = -1;
//	startPosition.y = -tan(radians(FOV / 2));
//	startPosition.x = ASPECT_RATIO * startPosition.y;
//	float pixelSize = abs(2 * startPosition.y / HEIGHT); // calculate pixel size
//	startPosition -= vec3(pixelSize / 2, pixelSize / 2, 0);
//
//	vec3 pixelPosition = startPosition;
//	for (unsigned int x = 0; x < WIDTH; x++) {
//		pixelPosition.x += pixelSize;
//		pixelPosition.y = startPosition.y;
//
//		glPointSize(2.0);
//		glBegin(GL_POINTS);
//		for (unsigned int y = 0; y < HEIGHT; y++) {
//			pixelPosition.y += pixelSize;
//
//			vec3 color = ambient_light;
//			if (useSSAA) {
//				color = calculateSSAA(numOfSubpixelsPerSide, pixelSize, pixelPosition);
//			}
//			else {
//				calculateRayColor(color, Ray(vec3(0), pixelPosition), 0);
//			}
//
//			color = clamp(color * 255.0f, vec3(0.0f), vec3(255.0f));
//			plot_pixel(x, y, color);
//		}
//		glEnd();
//		glFlush();
//	}
//	printf("Done!\n"); fflush(stdout);
//}
//
//vec3 calculateSSAA(int numOfSubpixelsPerSide, float pixelSize, vec3 pixelPosition) {
//	// stratified sampling
//	vec3 color = ambient_light;
//
//	float cellSize = pixelSize / numOfSubpixelsPerSide;
//	vec3 startPosition = pixelPosition;
//	float offset = pixelSize * 0.5f;
//	startPosition -= vec3(offset, offset, 0);
//
//	vec3 cellPosition = startPosition;
//	for (unsigned int x = 0; x < numOfSubpixelsPerSide; x++) {
//		cellPosition.x = startPosition.x + x * cellSize + getRandom(0, cellSize);
//
//		for (unsigned int y = 0; y < numOfSubpixelsPerSide; y++) {
//			cellPosition.y = startPosition.y + y * cellSize + getRandom(0, cellSize);
//
//			Ray cameraRay(vec3(0), cellPosition);
//			vec3 subcolor;
//			calculateRayColor(subcolor, cameraRay, 0);
//			color += subcolor;
//		}
//	}
//	color /= numOfSubpixelsPerSide * numOfSubpixelsPerSide;
//	return color;
//}
//
//#pragma region Triangles
//Material Triangle::getMaterial(vec3 position) {
//	Material material;
//
//	Material m0 = v[0].material;
//	Material m1 = v[1].material;
//	Material m2 = v[2].material;
//
//	vec3 bary = getBarycentricCoords(position);
//
//	material.normal = normalize(vec3(bary.x * m0.normal.x + bary.y * m1.normal.x + bary.z * m2.normal.x,
//									 bary.x * m0.normal.y + bary.y * m1.normal.y + bary.z * m2.normal.y,
//									 bary.x * m0.normal.z + bary.y * m1.normal.z + bary.z * m2.normal.z));
//
//	material.diffuse[0] = bary.x * m0.diffuse[0] + bary.y * m1.diffuse[0] + bary.z * m2.diffuse[0];
//	material.diffuse[1] = bary.x * m0.diffuse[1] + bary.y * m1.diffuse[1] + bary.z * m2.diffuse[1];
//	material.diffuse[2] = bary.x * m0.diffuse[2] + bary.y * m1.diffuse[2] + bary.z * m2.diffuse[2];
//
//	material.roughness = bary.x * m0.roughness + bary.y * m1.roughness + bary.z * m2.roughness;
//
//	material.metallic = bary.x * m0.metallic + bary.y * m1.metallic + bary.z * m2.metallic;
//
//	return material;
//}
//float Triangle::intersects(Ray* ray) {
//	vec3 a = v[0].position;
//	vec3 b = v[1].position;
//	vec3 c = v[2].position;
//
//	vec3 n = cross(b - a, c - a);
//	vec3 n_normalized = normalize(n);
//
//	double ndotd = dot(n_normalized, ray->direction);
//	if (compare(ndotd, 0) == 0) return -1;
//
//	double d = -dot(n_normalized, a);
//	double t = -(dot(n_normalized, ray->start) + d) / ndotd;
//	//double  t = -(dot((start - a), n_normalized) / (dot(n_normalized, direction)));
//	if (compare(t, 0) <= 0) return -1;
//
//	vec3 p = ray->getPosition(t);
//	if (dot(n_normalized, cross(b - a, p - a)) < 0 ||
//		dot(n_normalized, cross(c - b, p - b)) < 0 ||
//		dot(n_normalized, cross(a - c, p - c)) < 0) return -1;
//
//	return t;
//}
//vec3 Triangle::getBarycentricCoords(vec3 p) {
//	vec3 a = v[0].position;
//	vec3 b = v[1].position;
//	vec3 c = v[2].position;
//	float area = std::max(EPSILON, calculateArea(a, b, c));
//
//	float alpha = calculateArea(p, b, c) / area;
//	float beta = calculateArea(a, p, c) / area;
//	float gamma = 1 - alpha - beta;
//	return vec3(alpha, beta, gamma);
//}
//#pragma endregion
//
//
//#pragma region Sphere
//Material Sphere::getMaterial(vec3 position) {
//	Material material = baseMaterial;
//	material.normal = normalize(position - this->position);
//	return material;
//}
//float Sphere::intersects(Ray* ray) {
//	vec3 difference = ray->start - position;
//
//	double a = 1;
//	double b = 2 * dot(difference, ray->direction);
//	double c = dot(difference, difference) - radius * radius;
//
//	double checker = b * b - 4 * c;
//	if (checker < 0) return -1;
//
//	double t0 = 0.5 * (-b + sqrt(checker));
//	double t1 = 0.5 * (-b - sqrt(checker));
//	float t = std::min(t0, t1);
//	if (compare(t, 0) <= 0) return -1;
//
//	return t;
//}
//#pragma endregion
//
//
//#pragma region Ray
//Ray::Ray(vec3 start, vec3 target) {
//	this->start = start;
//	direction = normalize(target - start);
//}
//vec3 Ray::getPosition(float t) {
//	return start + direction * t;
//}
//Object* Ray::getFirstIntersectedObject(vec3& position) {
//	Object* object = nullptr;
//	float min_t = numeric_limits<float>::max();
//	for (int i = 0; i < objects.size(); i++) {
//		float t = objects[i]->intersects(this);
//		if (t > 0 && t < min_t) {
//			min_t = t;
//			object = objects[i];
//		}
//	}
//	position = getPosition(min_t);
//	return object;
//}
//bool Ray::checkIfBlocked() {
//	for (int i = 0; i < objects.size(); i++) {
//		if (objects[i]->intersects(this) > EPSILON) {
//			return true;
//		}
//	}
//	return false;
//}
//bool Ray::checkIfBlocked(vec3 target) {
//	float target_t = length(target - start);
//	for (int i = 0; i < objects.size(); i++) {
//		float t = objects[i]->intersects(this);
//		if (t > EPSILON && t <= target_t) {
//			return true;
//		}
//	}
//	return false;
//}
//#pragma endregion
//
//float calculateArea(Light* light) {
//	vec3 p0 = light->p[0];
//	vec3 p1 = light->p[1];
//	vec3 p2 = light->p[2];
//	vec3 p3 = light->p[3];
//	return distance(p1, p0) * distance(p0, p2);
//}
//vec3 sampleLight(Light* light) {
//	float U2 = getRandom();
//	float U3 = getRandom();
//
//	vec3 p0 = light->p[0];
//	vec3 p1 = light->p[1];
//	vec3 p2 = light->p[2];
//	vec3 p3 = light->p[3];
//	return (1 - U2) * (p0 * (1 - U3) + p1 * U3) + U2 * (p2 * (1 - U3) + p3 * U3); // sample point p_l
//}
//
//#pragma region Utility
//void calculateRayColor(vec3& finalColor, Ray& ray, int numOfReflection) {
//	if (numOfReflection > MAX_REFLECTION) return;
//
//	vec3 position;
//	Object* object = ray.getFirstIntersectedObject(position);
//	if (object) {
//		Material material = object->getMaterial(position);
//
//		vec3 Le(0);
//		for (int i = 0; i < lights.size(); i++) {
//			vec3 p_l = sampleLight(lights[i]);
//			vec3 lightDirection = normalize(p_l - position);
//
//			float wdotn = dot(lightDirection, material.normal);
//			if (compare(wdotn, 0) == 0) continue;
//
//			Ray shadowRay(position, position + lightDirection);
//			if (shadowRay.checkIfBlocked(p_l)) continue;
//
//			Le = lights[i]->color;
//
//			float pdf = pow(length(position - p_l), 2) / (abs(dot(lights[i]->normal, lightDirection)) * calculateArea(lights[i]));
//		}
//
//		if (useGlobalLighting) {
//			vec3 reflectionColor(0);
//			vec3 R = normalize(reflect(ray.direction, material.normal));
//			Ray reflectionRay(position, position + R);
//			calculateRayColor(reflectionColor, reflectionRay, numOfReflection + 1);
//
//			//finalColor = (1 - ks) * localColor + ks * reflectionColor;
//		}
//		else {
//			finalColor = localColor;
//		}
//	}
//	else {
//		finalColor = backgroundColor;
//	}
//}
//
//
//int loadScene(char* argv) {
//	FILE* file = fopen(argv, "r");
//	int number_of_objects;
//	char type[50];
//	fscanf(file, "%i", &number_of_objects);
//
//	printf("number of objects: %i\n", number_of_objects);
//
//	parse_vec3(file, "amb:", ambient_light);
//	parse_vec3(file, "f0:", F0);
//
//	for (int i = 0; i < number_of_objects; i++) {
//		fscanf(file, "%s\n", type);
//		printf("%s\n", type);
//		if (strcasecmp(type, "triangle") == 0) {
//			printf("found triangle\n");
//
//			Triangle* t = new Triangle();
//			for (int j = 0; j < 3; j++) {
//				parse_vec3(file, "pos:", t->v[j].position);
//				parse_vec3(file, "nor:", t->v[j].material.normal);
//				parse_vec3(file, "dif:", t->v[j].material.diffuse);
//
//				parse_float(file, "rou:", t->v[j].material.roughness);
//				parse_float(file, "met:", t->v[j].material.metallic);
//			}
//
//			if (triangles.size() == MAX_TRIANGLES) {
//				printf("too many triangles, you should increase MAX_TRIANGLES!\n");
//				exit(0);
//			}
//			triangles.push_back(t);
//			objects.push_back(t);
//		}
//		else if (strcasecmp(type, "sphere") == 0) {
//			printf("found sphere\n");
//
//			Sphere* s = new Sphere();
//			parse_vec3(file, "pos:", s->position);
//			parse_rad(file, &s->radius);
//			parse_vec3(file, "dif:", s->baseMaterial.diffuse);
//
//			parse_float(file, "rou:", s->baseMaterial.roughness);
//			parse_float(file, "met:", s->baseMaterial.metallic);
//
//			if (spheres.size() == MAX_SPHERES) {
//				printf("too many spheres, you should increase MAX_SPHERES!\n");
//				exit(0);
//			}
//			spheres.push_back(s);
//			objects.push_back(s);
//		}
//		else if (strcasecmp(type, "light") == 0) {
//			printf("found light\n");
//
//			Light* l = new Light();
//			parse_vec3(file, "p0:", l->p[0]);
//			parse_vec3(file, "p1:", l->p[1]);
//			parse_vec3(file, "p2:", l->p[2]);
//			parse_vec3(file, "p3:", l->p[3]);
//			parse_vec3(file, "pos:", l->position);
//			parse_vec3(file, "nrm:", l->normal);
//			parse_vec3(file, "col:", l->color);
//
//			if (lights.size() == MAX_LIGHTS) {
//				printf("too many lights, you should increase MAX_LIGHTS!\n");
//				exit(0);
//			}
//			lights.push_back(l);
//		}
//		else {
//			printf("unknown type in scene description:\n%s\n", type);
//			exit(0);
//		}
//	}
//
//	//lights = sampleLights();
//
//	return 0;
//}
//#pragma endregion
//
//void display() {
//}
//
//void init() {
//	glMatrixMode(GL_PROJECTION);
//	glOrtho(0, WIDTH, 0, HEIGHT, 1, -1);
//	glMatrixMode(GL_MODELVIEW);
//	glLoadIdentity();
//
//	glClearColor(0, 0, 0, 0);
//	glClear(GL_COLOR_BUFFER_BIT);
//}
//
//void idle() {
//	//hack to make it only draw once
//	static int once = 0;
//	if (!once) {
//		draw_scene();
//		if (mode == MODE_JPEG)
//			save_jpg();
//	}
//	once = 1;
//}
//
//int main(int argc, char** argv) {
//	if ((argc < 2) || (argc > 3)) {
//		printf("Usage: %s <input scenefile> [output jpegname]\n", argv[0]);
//		exit(0);
//	}
//	if (argc == 3) {
//		mode = MODE_JPEG;
//		filename = argv[2];
//	}
//	else if (argc == 2)
//
//		mode = MODE_DISPLAY;
//	printf("Input file: %s\n", argv[1]);
//
//	glutInit(&argc, argv);
//	loadScene(argv[1]);
//
//	glutInitDisplayMode(GLUT_RGBA | GLUT_SINGLE);
//	glutInitWindowPosition(0, 0);
//	glutInitWindowSize(WIDTH, HEIGHT);
//	int window = glutCreateWindow("Ray Tracer");
//#ifdef __APPLE__
//	// This is needed on recent Mac OS X versions to correctly display the window.
//	glutReshapeWindow(WIDTH - 1, HEIGHT - 1);
//#endif
//	glutDisplayFunc(display);
//	glutIdleFunc(idle);
//	init();
//	glutMainLoop();
//}
//
