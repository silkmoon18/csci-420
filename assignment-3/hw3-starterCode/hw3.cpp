/* **************************
 * CSCI 420
 * Assignment 3 Raytracer
 * Name: Baihua Yang
 * *************************
*/

#include "utility.h"




Scene* scene = new OpticalScene();

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
#include <chrono>

void idle() {
	//hack to make it only draw once
	static int once = 0;
	if (!once) {
		std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
		scene->draw();
		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
		std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() / 1e-6 << "[s]" << std::endl;
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

