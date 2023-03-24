#ifndef _UTILITY_H_
#define _UTILITY_H_

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

#define EPSILON 0.0001f
#define MAX_REFLECTION 3
#define MAX_TRIANGLES 20000
#define MAX_SPHERES 100
#define MAX_LIGHTS 100

//different display modes
#define MODE_DISPLAY 1
#define MODE_JPEG 2


//you may want to make these smaller for debugging purposes
#define WIDTH 640
#define HEIGHT 480
//the field of view of the camera
#define fov 60.0

#define ASERT(cond)                                                      \
  do {                                                                   \
    if ((cond) == false) {                                               \
      std::cerr << #cond << " failed at line " << __LINE__ << std::endl; \
      exit(1);                                                           \
    }                                                                    \
  } while (0)

using namespace std;
using namespace glm;


const float ASPECT_RATIO = WIDTH / (double)HEIGHT;
extern int mode;
extern char* filename;
extern unsigned char buffer[HEIGHT][WIDTH][3];


struct Material;
struct Vertex;
class Object;
class Triangle;
class Sphere;
class Light;
class Ray;

float getRandom(float min, float max);
float calculateArea(vec3 a, vec3 b, vec3 c);
int compare(float f1, float f2);

void plot_pixel_display(int x, int y, unsigned char r, unsigned char g, unsigned char b);
void plot_pixel_jpeg(int x, int y, unsigned char r, unsigned char g, unsigned char b);
void plot_pixel(int x, int y, vec3 color);
void save_jpg();

void parse_check(const char* expected, char* found);
void parse_vec3(FILE* file, const char* check, vec3& vec);
void parse_float(FILE* file, const char* check, float& f);
void parse_rad(FILE* file, float* r);
void parse_shi(FILE* file, float* shi);

#endif