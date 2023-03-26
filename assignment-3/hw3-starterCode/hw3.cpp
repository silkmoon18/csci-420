/* **************************
 * CSCI 420
 * Assignment 3 Raytracer
 * Name: Baihua Yang
 * *************************
*/

#include "utility.h"
#include <chrono>




OpticalScene* scene = new OpticalScene();

struct Pixel {
	vec2 index;
	vec3 position;
	float size;
};

vector<Pixel> pixels;
const int numOfThreads = 16;


void getPixels() {
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
			pixels.push_back(pixel);
		}
	}
}

float num = 0;
float per;
void drawPixel(Pixel pixel) {
	scene->draw(pixel.index.x, pixel.index.y, pixel.size, pixel.position);
	//num++;

	//per = num / pixels.size();
	//printf("\r%f%%", per * 100);
	
}

void drawPixels(int id) {
	int pixelsPerThread = pixels.size() / numOfThreads;
	int startPixel = id * pixelsPerThread;
	int endPixel = (id + 1) * pixelsPerThread;

	for (int i = startPixel; i < endPixel; i++) {
		int x = i % WIDTH;
		int y = i / WIDTH;

		drawPixel(pixels[i]);
	}
}

void draw_test() {
	scene->sampleLights();

	std::vector<std::thread> threads;
	for (int i = 0; i < numOfThreads; i++) {
		std::thread t(drawPixels, i);
		threads.push_back(std::move(t));
	}

	for (auto& thread : threads) {
		thread.join();
	}
	for (unsigned int x = 0; x < WIDTH; x++) {
		glPointSize(2.0);
		glBegin(GL_POINTS);
		for (unsigned int y = 0; y < HEIGHT; y++) {
			unsigned char r = scene->buffer[y][x][0];
			unsigned char g = scene->buffer[y][x][1];
			unsigned char b = scene->buffer[y][x][2];
			glColor3f(((float)r) / 255.0f, ((float)g) / 255.0f, ((float)b) / 255.0f);
			glVertex2i(x, y);
		}
		//printf("\r%f%%", ceil(x / (float)WIDTH) * 100.0f) / 100.0f;

		glEnd();
		glFlush();
	}
}

void display() {
}

void init() {
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
		std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

		draw_test();


		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
		printf("delta time = %fs\n", std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() * 1e-6);
		if (scene->mode == MODE_JPEG)
			scene->save_jpg();
	}
	once = 1;
}

int main(int argc, char** argv) {
	if ((argc < 2) || (argc > 3)) {
		printf("Usage: %s <input scenefile> [output jpegname]\n", argv[0]);
		exit(0);
	}
	if (argc == 3) {
		scene->mode = MODE_JPEG;
		scene->filename = argv[2];
	}
	else if (argc == 2) {
		scene->mode = MODE_DISPLAY;
	}
	printf("Input file: %s\n", argv[1]);

	glutInit(&argc, argv);
	
	getPixels();
	scene->load(argv[1]);

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

