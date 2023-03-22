/* **************************
 * CSCI 420
 * Assignment 3 Raytracer
 * Name: <Your name here>
 * *************************
*/

#ifdef WIN32
#include <windows.h>
#endif

#if defined(WIN32) || defined(linux)
#include <GL/gl.h>
#include <GL/glut.h>
#elif defined(__APPLE__)
#include <OpenGL/gl.h>
#include <GLUT/glut.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
#define strcasecmp _stricmp
#endif

#include <imageIO.h>
#include <glm/glm.hpp>
#include <iostream>
#include <vector>
using namespace std;

using namespace glm;

#define EPSILON 0.0001f
#define MAX_REFLECTION 4
#define MAX_TRIANGLES 20000
#define MAX_SPHERES 100
#define MAX_LIGHTS 100

char* filename = NULL;

//different display modes
#define MODE_DISPLAY 1
#define MODE_JPEG 2

int mode = MODE_DISPLAY;

//you may want to make these smaller for debugging purposes
#define WIDTH 7
#define HEIGHT 3
#define WIDTH 640
#define HEIGHT 480
double aspectRatio = WIDTH / (double)HEIGHT;

//the field of view of the camera
#define fov 60.0

unsigned char buffer[HEIGHT][WIDTH][3];


struct Material;
struct Vertex;
class Object;
class Triangle;
class Sphere;
struct Light;
class Ray;

vec3 calculateSSAA(int gridSize, float cellSize, vec3 pixelPosition);
vec3 calculateRayColor(Ray& ray, int numOfReflection);
vec3 calculatePhongShading(vec3 position, Light light, Material material, vec3 normal);
float calculateArea(vec3 a, vec3 b, vec3 c);

vec3 toVec3(double array[3]);
int compare(double d1, double d2);
void plot_pixel_display(int x, int y, unsigned char r, unsigned char g, unsigned char b);
void plot_pixel_jpeg(int x, int y, unsigned char r, unsigned char g, unsigned char b);
void plot_pixel(int x, int y, vec3 color);

struct Material {
	double color_diffuse[3];
	double color_specular[3];
	double shininess;
};

struct Vertex {
	double position[3];
	double normal[3];
	Material material;
};

class Object {
public:
	virtual vec3 getNormal(vec3 position) = 0;
	virtual vec3 calculateLighting(vec3 position) = 0;
	virtual float intersects(Ray* ray) = 0;
};
class Triangle : public Object {
public:
	Vertex v[3];

	virtual vec3 getNormal(vec3 position) override;
	vec3 calculateLighting(vec3 position) override;
	float intersects(Ray* ray) override;

private:
	vec3 getBarycentricCoords(vec3 p);
};

class Sphere : public Object {
public:
	double position[3];
	double radius;
	Material material;

	virtual vec3 getNormal(vec3 position) override;
	vec3 calculateLighting(vec3 position) override;
	float intersects(Ray* ray) override;
};

struct Light {
	double position[3];
	double color[3];
};

class Ray {
public:
	vec3 start;
	vec3 direction;

	Ray(vec3 start, vec3 target);

	vec3 getPosition(float t);
	Object* getFirstIntersectedObject(vec3& position);
	bool checkIfBlocked();
	bool checkIfBlocked(vec3 target);
};

vector<Object*> objects;
Triangle triangles[MAX_TRIANGLES];
Sphere spheres[MAX_SPHERES];
Light lights[MAX_LIGHTS];

int num_triangles = 0;
int num_spheres = 0;
int num_lights = 0;

double ambient_light[3];
vec3 backgroundColor = vec3(0);

bool useSSAA = false;
int SSAA_level = 3;





void draw_scene() {
	// calculate pixel start position, which is at x = -1, y = -1 (outside of the image).
	vec3 startPosition;
	startPosition.z = -1;
	startPosition.y = -tan(radians(fov / 2));
	startPosition.x = aspectRatio * startPosition.y;
	float pixelSize = abs(2 * startPosition.y / HEIGHT); // calculate pixel size
	startPosition -= vec3(pixelSize / 2, pixelSize / 2, 0);

	vec3 pixelPosition = startPosition;
	for (unsigned int x = 0; x < WIDTH; x++) {
		pixelPosition.x += pixelSize;
		pixelPosition.y = startPosition.y;

		glPointSize(2.0);
		glBegin(GL_POINTS);
		for (unsigned int y = 0; y < HEIGHT; y++) {
			pixelPosition.y += pixelSize;

			vec3 color;
			if (useSSAA) {
				color = calculateSSAA(SSAA_level, pixelSize, pixelPosition);
			}
			else {
				Ray cameraRay(vec3(0), pixelPosition);
				color = calculateRayColor(cameraRay, 0);
			}

			color = clamp(color * 255.0f, vec3(0.0f), vec3(255.0f));
			plot_pixel(x, y, color);
		}
		glEnd();
		glFlush();
	}
	printf("Done!\n"); fflush(stdout);
}

vec3 calculateSSAA(int SSAA_level, float pixelSize, vec3 pixelPosition) {
	vec3 color(0);

	float cellSize = pixelSize / SSAA_level;
	vec3 startPosition = pixelPosition;
	float offset = 0.5f * cellSize * (SSAA_level + 1);
	startPosition -= vec3(offset, offset, 0);

	vec3 cellPosition = startPosition;
	for (unsigned int x = 0; x < SSAA_level; x++) {
		cellPosition.x += cellSize;
		cellPosition.y = startPosition.y;

		for (unsigned int y = 0; y < SSAA_level; y++) {
			cellPosition.y += cellSize;

			Ray cameraRay(vec3(0), cellPosition);

			color += calculateRayColor(cameraRay, 0);
		}
	}
	color /= SSAA_level * SSAA_level;
	return color;
}

#pragma region Triangles
vec3 Triangle::getNormal(vec3 position) {
	vec3 bary = getBarycentricCoords(position);
	return normalize(vec3(bary.x * v[0].normal[0] + bary.y * v[1].normal[0] + bary.z * v[2].normal[0],
								 bary.x * v[0].normal[1] + bary.y * v[1].normal[1] + bary.z * v[2].normal[1],
								 bary.x * v[0].normal[2] + bary.y * v[1].normal[2] + bary.z * v[2].normal[2]));
}
vec3 Triangle::calculateLighting(vec3 position) {
	Material material;

	// copied from Triangle::getNormal() for performance reason
	vec3 bary = getBarycentricCoords(position);
	vec3 normal = normalize(vec3(bary.x * v[0].normal[0] + bary.y * v[1].normal[0] + bary.z * v[2].normal[0],
								 bary.x * v[0].normal[1] + bary.y * v[1].normal[1] + bary.z * v[2].normal[1],
								 bary.x * v[0].normal[2] + bary.y * v[1].normal[2] + bary.z * v[2].normal[2]));

	material.color_diffuse[0] = bary.x * v[0].material.color_diffuse[0] + bary.y * v[1].material.color_diffuse[0] + bary.z * v[2].material.color_diffuse[0];
	material.color_diffuse[1] = bary.x * v[0].material.color_diffuse[1] + bary.y * v[1].material.color_diffuse[1] + bary.z * v[2].material.color_diffuse[1];
	material.color_diffuse[2] = bary.x * v[0].material.color_diffuse[2] + bary.y * v[1].material.color_diffuse[2] + bary.z * v[2].material.color_diffuse[2];

	material.color_specular[0] = bary.x * v[0].material.color_specular[0] + bary.y * v[1].material.color_specular[0] + bary.z * v[2].material.color_specular[0];
	material.color_specular[1] = bary.x * v[0].material.color_specular[1] + bary.y * v[1].material.color_specular[1] + bary.z * v[2].material.color_specular[1];
	material.color_specular[2] = bary.x * v[0].material.color_specular[2] + bary.y * v[1].material.color_specular[2] + bary.z * v[2].material.color_specular[2];

	material.shininess = bary.x * v[0].material.shininess + bary.y * v[1].material.shininess + bary.z * v[2].material.shininess;

	vec3 color = toVec3(ambient_light);
	for (int i = 0; i < num_lights; i++) {
		vec3 lightPosition = toVec3(lights[i].position);
		Ray shadowRay(position, lightPosition);
		if (shadowRay.checkIfBlocked(lightPosition)) {
			shadowRay.checkIfBlocked(lightPosition);
			continue;
		}
		color += calculatePhongShading(position, lights[i], material, normal);
	}
	return color;
}
float Triangle::intersects(Ray* ray) {
	vec3 a = toVec3(v[0].position);
	vec3 b = toVec3(v[1].position);
	vec3 c = toVec3(v[2].position);

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
	vec3 a = toVec3(v[0].position);
	vec3 b = toVec3(v[1].position);
	vec3 c = toVec3(v[2].position);
	float area = std::max(EPSILON, calculateArea(a, b, c));

	float alpha = calculateArea(p, b, c) / area;
	float beta = calculateArea(a, p, c) / area;
	float gamma = 1 - alpha - beta;
	return vec3(alpha, beta, gamma);
}
#pragma endregion


#pragma region Sphere
vec3 Sphere::getNormal(vec3 position) {
	return normalize(position - toVec3(this->position));;
}
vec3 Sphere::calculateLighting(vec3 position) {
	vec3 normal = getNormal(position);
	vec3 color = toVec3(ambient_light);

	for (int i = 0; i < num_lights; i++) {
		Ray shadowRay(position, toVec3(lights[i].position));
		if (shadowRay.checkIfBlocked()) continue;
		color += calculatePhongShading(position, lights[i], material, normal);
	}
	return color;
}
float Sphere::intersects(Ray* ray) {
	vec3 difference = ray->start - toVec3(position);

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


#pragma region Ray
Ray::Ray(vec3 start, vec3 target) {
	this->start = start;
	direction = normalize(target - start);
}
vec3 Ray::getPosition(float t) {
	return start + direction * t;
}
Object* Ray::getFirstIntersectedObject(vec3& position) {
	Object* object = nullptr;
	float min_t = numeric_limits<float>::max();
	for (int i = 0; i < objects.size(); i++) {
		float t = objects[i]->intersects(this);
		if (t > 0 && t < min_t) {
			min_t = t;
			object = objects[i];
		}
	}
	position = getPosition(min_t);
	return object;
}
bool Ray::checkIfBlocked() {
	for (int i = 0; i < objects.size(); i++) {
		if (objects[i]->intersects(this) > EPSILON) {
			return true;
		}
	}
	return false;
}
bool Ray::checkIfBlocked(vec3 target) {
	float target_t = length(target - start);
	for (int i = 0; i < objects.size(); i++) {
		float t = objects[i]->intersects(this);
		if (t > EPSILON && t <= target_t) {
			return true;
		}
	}
	return false;
}
#pragma endregion


#pragma region Utility
vec3 calculateRayColor(Ray& ray, int numOfReflection) {
	vec3 color(0);
	if (numOfReflection > MAX_REFLECTION) return color;

	vec3 position;
	Object* object = ray.getFirstIntersectedObject(position);
	if (object) {
		vec3 R = normalize(reflect(ray.direction, object->getNormal(position)));

		color += object->calculateLighting(position);
		color += calculateRayColor(Ray(position, position + R), numOfReflection + 1);
	}
	else if (numOfReflection == 0) color = backgroundColor;



	return color;
}
vec3 calculatePhongShading(vec3 position, Light light, Material material, vec3 normal) {
	vec3 lightVector = normalize(toVec3(light.position) - position);
	vec3 diffuseColor = toVec3(material.color_diffuse);
	vec3 specularColor = toVec3(material.color_specular);

	float ndotl = std::max(dot(lightVector, normal), 0.0f);
	vec3 diffuse = diffuseColor * ndotl;

	vec3 R = normalize(reflect(-lightVector, normal));
	vec3 eyeVector = normalize(-position);
	float rdotv = std::max(dot(R, eyeVector), 0.0f);
	vec3 specular = specularColor * (float)pow(rdotv, material.shininess);

	return toVec3(light.color) * diffuse + specular;
}
float calculateArea(vec3 a, vec3 b, vec3 c) {
	return 0.5f * (((b.x - a.x) * (c.y - a.y)) - ((c.x - a.x) * (b.y - a.y)));
}
vec3 toVec3(double array[3]) {
	return vec3(array[0], array[1], array[2]);
}
int compare(double d1, double d2) {
	int result = 1;

	double diff = d1 - d2;
	if (diff < 0) result = -1;

	if (abs(diff) <= EPSILON) result = 0;

	return result;
}

void plot_pixel_display(int x, int y, unsigned char r, unsigned char g, unsigned char b) {
	glColor3f(((float)r) / 255.0f, ((float)g) / 255.0f, ((float)b) / 255.0f);
	glVertex2i(x, y);
}
void plot_pixel_jpeg(int x, int y, unsigned char r, unsigned char g, unsigned char b) {
	buffer[y][x][0] = r;
	buffer[y][x][1] = g;
	buffer[y][x][2] = b;
}
void plot_pixel(int x, int y, vec3 color) {
	unsigned char r = color.x;
	unsigned char g = color.y;
	unsigned char b = color.z;
	plot_pixel_display(x, y, r, g, b);
	if (mode == MODE_JPEG)
		plot_pixel_jpeg(x, y, r, g, b);
}
void save_jpg() {
	printf("Saving JPEG file: %s\n", filename);

	ImageIO img(WIDTH, HEIGHT, 3, &buffer[0][0][0]);
	if (img.save(filename, ImageIO::FORMAT_JPEG) != ImageIO::OK)
		printf("Error in Saving\n");
	else
		printf("File saved Successfully\n");
}
void parse_check(const char* expected, char* found) {
	if (strcasecmp(expected, found)) {
		printf("Expected '%s ' found '%s '\n", expected, found);
		printf("Parse error, abnormal abortion\n");
		exit(0);
	}
}
void parse_doubles(FILE* file, const char* check, double p[3]) {
	char str[100];
	fscanf(file, "%s", str);
	parse_check(check, str);
	fscanf(file, "%lf %lf %lf", &p[0], &p[1], &p[2]);
	printf("%s %lf %lf %lf\n", check, p[0], p[1], p[2]);
}
void parse_rad(FILE* file, double* r) {
	char str[100];
	fscanf(file, "%s", str);
	parse_check("rad:", str);
	fscanf(file, "%lf", r);
	printf("rad: %f\n", *r);
}
void parse_shi(FILE* file, double* shi) {
	char s[100];
	fscanf(file, "%s", s);
	parse_check("shi:", s);
	fscanf(file, "%lf", shi);
	printf("shi: %f\n", *shi);
}

int loadScene(char* argv) {
	FILE* file = fopen(argv, "r");
	int number_of_objects;
	char type[50];
	Triangle* t;
	Sphere* s;
	Light l;
	fscanf(file, "%i", &number_of_objects);

	printf("number of objects: %i\n", number_of_objects);

	parse_doubles(file, "amb:", ambient_light);

	for (int i = 0; i < number_of_objects; i++) {
		fscanf(file, "%s\n", type);
		printf("%s\n", type);
		if (strcasecmp(type, "triangle") == 0) {
			printf("found triangle\n");

			t = new Triangle();
			for (int j = 0; j < 3; j++) {
				parse_doubles(file, "pos:", t->v[j].position);
				parse_doubles(file, "nor:", t->v[j].normal);
				parse_doubles(file, "dif:", t->v[j].material.color_diffuse);
				parse_doubles(file, "spe:", t->v[j].material.color_specular);
				parse_shi(file, &t->v[j].material.shininess);
			}

			if (num_triangles == MAX_TRIANGLES) {
				printf("too many triangles, you should increase MAX_TRIANGLES!\n");
				exit(0);
			}
			triangles[num_triangles++] = *t;
			objects.push_back(t);
		}
		else if (strcasecmp(type, "sphere") == 0) {
			printf("found sphere\n");

			s = new Sphere();
			parse_doubles(file, "pos:", s->position);
			parse_rad(file, &s->radius);
			parse_doubles(file, "dif:", s->material.color_diffuse);
			parse_doubles(file, "spe:", s->material.color_specular);
			parse_shi(file, &s->material.shininess);

			if (num_spheres == MAX_SPHERES) {
				printf("too many spheres, you should increase MAX_SPHERES!\n");
				exit(0);
			}
			spheres[num_spheres++] = *s;
			objects.push_back(s);
		}
		else if (strcasecmp(type, "light") == 0) {
			printf("found light\n");
			parse_doubles(file, "pos:", l.position);
			parse_doubles(file, "col:", l.color);

			if (num_lights == MAX_LIGHTS) {
				printf("too many lights, you should increase MAX_LIGHTS!\n");
				exit(0);
			}
			lights[num_lights++] = l;
		}
		else {
			printf("unknown type in scene description:\n%s\n", type);
			exit(0);
		}
	}

	return 0;
}
#pragma endregion

void display() {
}

void debug() {

}

void init() {
	debug();

	glMatrixMode(GL_PROJECTION);
	glOrtho(0, WIDTH, 0, HEIGHT, 1, -1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
}

void idle() {
	//hack to make it only draw once
	static int once = 0;
	if (!once) {
		draw_scene();
		if (mode == MODE_JPEG)
			save_jpg();
	}
	once = 1;
}

int main(int argc, char** argv) {
	if ((argc < 2) || (argc > 3)) {
		printf("Usage: %s <input scenefile> [output jpegname]\n", argv[0]);
		exit(0);
	}
	if (argc == 3) {
		mode = MODE_JPEG;
		filename = argv[2];
	}
	else if (argc == 2)

		mode = MODE_DISPLAY;
	printf("Input file: %s\n", argv[1]);

	glutInit(&argc, argv);
	loadScene(argv[1]);

	glutInitDisplayMode(GLUT_RGBA | GLUT_SINGLE);
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(WIDTH, HEIGHT);
	int window = glutCreateWindow("Ray Tracer");
#ifdef __APPLE__
	// This is needed on recent Mac OS X versions to correctly display the window.
	glutReshapeWindow(WIDTH - 1, HEIGHT - 1);
#endif
	glutDisplayFunc(display);
	glutIdleFunc(idle);
	init();
	glutMainLoop();
}

