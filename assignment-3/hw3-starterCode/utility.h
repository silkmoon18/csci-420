#ifndef _UTILITY_H_
#define _UTILITY_H_

#ifdef WIN32
#include <windows.h>
#endif

#if defined(WIN32) || defined(linux)
#include <GL/gl.h>
#include <GL/glut.h>
#elif defined(__APPLE__)
#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
#define strcasecmp _stricmp
#endif

#include <chrono>
#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <imageIO.h>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>

#define EPSILON 0.0001f
#define PI 3.14159265358979323846264338327950288
#define MAX_DEPTH 3
#define MAX_TRIANGLES 20000
#define MAX_SPHERES 100
#define MAX_LIGHTS 100

// display modes
#define MODE_DISPLAY 1
#define MODE_JPEG 2

#define WIDTH 640
#define HEIGHT 480
#define ASPECT_RATIO (WIDTH / (double)HEIGHT)
#define FOV 60.0

#define ASERT(cond)                                                            \
    do {                                                                       \
        if ((cond) == false) {                                                 \
            std::cerr << #cond << " failed at line " << __LINE__ << std::endl; \
            exit(1);                                                           \
        }                                                                      \
    } while (0)

using namespace std;
using namespace glm;

struct ProgressInfo;
struct Pixel;
struct Vertex;

template <class T>
class Singleton;
class Timer;
class Scene;
class PhongScene;
class OpticalScene;
class Material;
class PhongMaterial;
class OpticalMaterial;
class Object;
class Triangle;
class Sphere;
class Ray;
class Light;

string secondsToHMS(int seconds); // convert seconds to hour, minutes and seconds
void printProgress(Scene* scene); // print progress of a scene
void parse_check(const char* expected, char* found);
void parse_vec3(FILE* file, const char* check, vec3& vec);
void parse_float(FILE* file, const char* check, float& f);
void parse_rad(FILE* file, float* r);
void parse_shi(FILE* file, float* shi);

int isPositive(float number); // check if the number is positive
int sign(float number); // get sign of the number
float getRandom(); // get a random (0, 1)
float getRandom(float min, float max); // get a random (min, max)
vec3 getRandom(vec3 min, vec3 max); // get a random vec3 (min, max)
float calculateArea(vec3 a, vec3 b, vec3 c); // calculate area of the triangle with three points
int compare(float f1, float f2); // compare f1 and f2

struct Pixel {
    ivec2 index;
    vec3 position;
    float size = 0.0f;
    vec3 color; // final color
    vec3 accumulatedColor; // added color when sampling
};

struct Vertex {
    vec3 position;
    Material* material = nullptr;
};

template <class T>
class Singleton {
protected:
    static inline T* instance = nullptr;

    Singleton() noexcept = default;
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    virtual ~Singleton() = default;

public:
    static T* getInstance() {
        if (!instance) {
            instance = new T();
        }
        return instance;
    }
};

class Timer : public Singleton<Timer> {
public:
    // Get time in seconds between this and previous frame
    float getDeltaTime();
    // Get current time in seconds
    float getCurrentTime();
    // Update the current time
    void update();

private:
    float previousTime = 0.0f; // in s
    float deltaTime = 0.0f; // in s
};

class Scene {
public:
    int mode = MODE_JPEG; // don't display at defualt

    vec3 ambient_light;
    vec3 backgroundColor = vec3(0.93f, 0.93f, 0.95f);
    vec3 F0;

    bool isGlobalLightingEnabled = true;

    const vector<Object*>& getObjects();
    const vector<Triangle*>& getTriangles();
    const vector<Sphere*>& getSpheres();
    const vector<Light*>& getLights();

    void setAntiAliasingLevel(int antiAliasingLevel); // # subpixels = 2 ^ antiAliasingLevel
    void setNumOfThreads(int num);

    void render(); // start rendering the image
    void display(); // display image
    void save(); // save image
    void drawPixelsThread(int threadIndex); // thread to draw pixels
    virtual void clear(); // clear everything

    virtual int load(const char* argv) = 0; // load a scene file
    virtual char* getOutputFilename() = 0; // get output filename
    virtual string getProgressInfo() = 0; // get current progress info

protected:
    char* inputFilename = NULL; // input scene file

    vector<Object*> objects;
    vector<Triangle*> triangles;
    vector<Sphere*> spheres;
    vector<Light*> lights;

    vector<Pixel> pixels;
    atomic_int numOfCompletedPixels = 0;

    int numOfSubpixelsPerSide = 1; // 1 means no anti-aliasing

    int numOfThreads = 32;
    float startTime = 0.0f;

    void initializePixels();

    virtual void process() = 0;
    virtual void calculatePixelColor(Pixel& pixel) = 0;
    virtual Triangle* parseTriangle(FILE* file) = 0;
    virtual Sphere* parseSphere(FILE* file) = 0;
    virtual Light* parseLight(FILE* file) = 0;
};

// phong shading
class PhongScene : public Scene {
public:
    PhongScene(int softShadowLevel);

    void setSoftShadowLevel(int softShadowLevel); // # light samples = 2 ^ softShadowLevel
    int load(const char* argv) override;
    void clear() override;
    char* getOutputFilename() override;
    string getProgressInfo() override;

protected:
    int numOfSampleLights = 1; // 1 means no sample lights
    int numOfCompletedSampleLights = 0;

    void process() override;
    void calculatePixelColor(Pixel& pixel) override;
    Triangle* parseTriangle(FILE* file) override;
    Sphere* parseSphere(FILE* file) override;
    Light* parseLight(FILE* file) override;
};

// monte-carlo
class OpticalScene : public Scene {
public:
    OpticalScene(int numOfSampleRays);

    void setNumOfSampleRays(int num);
    int load(const char* argv) override;
    void clear() override;
    char* getOutputFilename() override;
    string getProgressInfo() override;

protected:
    int numOfSampleRays = 1;
    int numOfCompletedSampleRays = 0;

    void process() override;
    void calculatePixelColor(Pixel& pixel) override;
    Triangle* parseTriangle(FILE* file) override;
    Sphere* parseSphere(FILE* file) override;
    Light* parseLight(FILE* file) override;
};

class Material {
public:
    vec3 normal;
    vec3 diffuse;

    vec3 specular;
    float shininess;

    float roughness;
    float metallic;

    Material(vec3 normal, vec3 diffuse);
    virtual ~Material() = default;

    virtual Material* clone() = 0;
    virtual vec3 calculateLighting(Scene* scene, Ray& ray, vec3 position) = 0;
    virtual Material* interpolates(Material* m1, Material* m2, vec3 bary) = 0; // interpolates a triangle material
};

class PhongMaterial : public Material {
public:
    PhongMaterial(vec3 normal, vec3 diffuse, vec3 specular, float shininess);

    Material* clone() override;
    vec3 calculateLighting(Scene* scene, Ray& ray, vec3 position) override;
    Material* interpolates(Material* m1, Material* m2, vec3 bary) override;

private:
    vec3 calculatePhongShading(vec3 position, vec3 lightPosition, vec3 lightColor);
};

class OpticalMaterial : public Material {
public:
    OpticalMaterial(vec3 normal, vec3 diffuse, float roughness, float metallic);

    Material* clone() override;
    vec3 calculateLighting(Scene* scene, Ray& ray, vec3 position) override;
    Material* interpolates(Material* m1, Material* m2, vec3 bary) override;

private:
    vec3 calculateBrdfShading(float F0, vec3 Le, vec3 n, float pdf, vec3 p, vec3 w_i, vec3 w_o);
};

class Object {
public:
    virtual ~Object() = default;

    virtual Material* getMaterial(vec3 position) = 0;
    virtual float intersects(Ray* ray) = 0; // get intersection param t
};

class Triangle : public Object {
public:
    Triangle(vector<Vertex> vertices);

    vector<Vertex> getVertices();
    Material* getMaterial(vec3 position) override;
    float intersects(Ray* ray) override;

private:
    vector<Vertex> vertices;

    vec3 getBarycentricCoords(vec3 p);
};

class Sphere : public Object {
public:
    Sphere(vec3 position, float radius, Material* material);

    Material* getMaterial(vec3 position) override;
    float intersects(Ray* ray) override;

private:
    vec3 position;
    float radius;
    Material* baseMaterial; // base material with normal = vec3(0)
};

class Light {
public:
    vec3 position;
    vec3 color;
    vec3 normal;
    vector<vec3> p;

    Light(vec3 position, vec3 color, vec3 normal = vec3(0), vector<vec3> p = vector<vec3>());

    float area(); // get light area, 0 if point light
    vec3 samplePosition(); // sample light position
};

class Ray {
public:
    vec3 start;
    vec3 direction;

    Ray(vec3 start, vec3 target);

    vec3 getPosition(float t);
    Object* getFirstIntersectedObject(const vector<Object*>& objects, vec3& intersectedPosition);
    bool checkIfBlocked(const vector<Object*>& objects, vec3 target);
    vec3 calculateRayColor(Scene* scene);

private:
    int depth = 0; // num of reflected times
};

#endif