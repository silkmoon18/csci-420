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
using namespace std;

using namespace glm;

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



int compare(double d1, double d2);
void plot_pixel_display(int x, int y, unsigned char r, unsigned char g, unsigned char b);
void plot_pixel_jpeg(int x, int y, unsigned char r, unsigned char g, unsigned char b);
void plot_pixel(int x, int y, vec3 color);

struct Vertex {
	double position[3];
	double color_diffuse[3];
	double color_specular[3];
	double normal[3];
	double shininess;
};

struct Triangle {
	Vertex v[3];
};

struct Sphere {
	double position[3];
	double color_diffuse[3];
	double color_specular[3];
	double shininess;
	double radius;
};

struct Light {
	double position[3];
	double color[3];
};

class Ray {
public:
	Ray(vec3 start, vec3 direction) {
		this->start = start;
		this->direction = normalize(direction);
	}

	bool intersects(Sphere &sphere, vec3 &point) {
		vec3 difference = start - vec3(sphere.position[0], sphere.position[1], sphere.position[2]);

		double a = 1;
		double b = 2 * dot(difference, direction);
		double c = dot(difference, difference) - sphere.radius * sphere.radius;

		double checker = b * b - 4 * c;
		if (checker < 0) return false;

		double t0 = 0.5 * (-b + sqrt(checker));
		double t1 = 0.5 * (-b - sqrt(checker));

		double t = std::min(t0, t1);
		if (t < 0) return false;

		point = start + direction * (float)t;
		return true;
	}

	bool intersects(Triangle triangle, vec3& point) {
		vec3 a(triangle.v[0].position[0], triangle.v[0].position[1], triangle.v[0].position[2]);
		vec3 b(triangle.v[1].position[0], triangle.v[1].position[1], triangle.v[1].position[2]);
		vec3 c(triangle.v[2].position[0], triangle.v[2].position[1], triangle.v[2].position[2]);

		vec3 n = cross(b - a, c - a);
		vec3 n_normalized = normalize(n);

		double ndotd = dot(n_normalized, direction);
		if (compare(ndotd, 0) == 0) return false;

		double d = -dot(n_normalized, a);
		double t = -(dot(n_normalized, start) + d) / ndotd;
		//double  t = -(dot((start - a), n_normalized) / (dot(n_normalized, direction)));
		if (t < 0) return false;

		vec3 p = start + direction * (float)t;

		if (dot(n_normalized, cross(b - a, p - a)) < 0 ||
			dot(n_normalized, cross(c - b, p - b)) < 0 ||
			dot(n_normalized, cross(a - c, p - c)) < 0) return false;

		point = p;
		return true;
	}

private:
	vec3 start;
	vec3 direction;
};

Triangle triangles[MAX_TRIANGLES];
Sphere spheres[MAX_SPHERES];
Light lights[MAX_LIGHTS];
double ambient_light[3];

int num_triangles = 0;
int num_spheres = 0;
int num_lights = 0;





void draw_scene() {
	// calculate pixel start position, which is at x = -1, y = -1 (outside of the image).
	vec3 startPosition;
	startPosition.z = -1;
	startPosition.y = -tan(radians(fov / 2));
	startPosition.x = aspectRatio * startPosition.y;
	double pixelSize = abs(2 * startPosition.y / HEIGHT); // calculate pixel size
	startPosition -= vec3(pixelSize / 2, pixelSize / 2, 0);

	vec3 pixelPosition = startPosition;
	for (unsigned int x = 0; x < WIDTH; x++) {
		pixelPosition.x += pixelSize;
		pixelPosition.y = startPosition.y;

		glPointSize(2.0);
		glBegin(GL_POINTS);
		for (unsigned int y = 0; y < HEIGHT; y++) {
			pixelPosition.y += pixelSize;
			Ray cameraRay(vec3(0), pixelPosition);

			// debug
			vec3 color(0);
			for (int i = 0; i < num_triangles; i++) {
				if (cameraRay.intersects(triangles[i], vec3())) {
					color = vec3(255);
					break;
				}
			}

			plot_pixel(x, y, color);
		}
		glEnd();
		glFlush();
	}
	printf("Done!\n"); fflush(stdout);
}

int compare(double d1, double d2) {
	int result = 1;

	double diff = d1 - d2;
	if (diff < 0) result = -1;

	if (abs(diff) <= numeric_limits<double>::epsilon()) result = 0;

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
	Triangle t;
	Sphere s;
	Light l;
	fscanf(file, "%i", &number_of_objects);

	printf("number of objects: %i\n", number_of_objects);

	parse_doubles(file, "amb:", ambient_light);

	for (int i = 0; i < number_of_objects; i++) {
		fscanf(file, "%s\n", type);
		printf("%s\n", type);
		if (strcasecmp(type, "triangle") == 0) {
			printf("found triangle\n");
			for (int j = 0; j < 3; j++) {
				parse_doubles(file, "pos:", t.v[j].position);
				parse_doubles(file, "nor:", t.v[j].normal);
				parse_doubles(file, "dif:", t.v[j].color_diffuse);
				parse_doubles(file, "spe:", t.v[j].color_specular);
				parse_shi(file, &t.v[j].shininess);
			}

			if (num_triangles == MAX_TRIANGLES) {
				printf("too many triangles, you should increase MAX_TRIANGLES!\n");
				exit(0);
			}
			triangles[num_triangles++] = t;
		}
		else if (strcasecmp(type, "sphere") == 0) {
			printf("found sphere\n");

			parse_doubles(file, "pos:", s.position);
			parse_rad(file, &s.radius);
			parse_doubles(file, "dif:", s.color_diffuse);
			parse_doubles(file, "spe:", s.color_specular);
			parse_shi(file, &s.shininess);

			if (num_spheres == MAX_SPHERES) {
				printf("too many spheres, you should increase MAX_SPHERES!\n");
				exit(0);
			}
			spheres[num_spheres++] = s;
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

