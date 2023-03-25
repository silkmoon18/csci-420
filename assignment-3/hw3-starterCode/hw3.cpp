/* **************************
 * CSCI 420
 * Assignment 3 Raytracer
 * Name: Baihua Yang
 * *************************
*/

#include "utility.h"


//int numOfSublights = 12;
//vector<Light*> sampleLights() {
//	vector<Light*> lightSamples;
//
//	for (int i = 0; i < lights.size(); i++) {
//		Light* light = lights[i];
//		for (int j = 0; j < numOfSublights; j++) {
//
//			float radius = ((double)rand() / (RAND_MAX)) / 10.0;
//			//randomly sample theta
//			float theta = radians((float)fmod(rand(), 360));
//			//randomly sample phi
//			float phi = radians((float)fmod(rand(), 360));
//
//			float x = radius * std::sin(theta) * std::cos(phi);
//			float y = radius * std::sin(theta) * std::sin(phi);
//			float z = radius * std::cos(theta);
//
//			//float x = rand() / float(RAND_MAX) * 0.1;
//			//float y = rand() / float(RAND_MAX) * 0.1;
//			//float z = rand() / float(RAND_MAX) * 0.1;
//
//			float xPos = light->position[0] + x;
//			float yPos = light->position[1] + y;
//			float zPos = light->position[2] + z;
//			vec3 position(xPos, yPos, zPos);
//
//			vec3 color = light->color / (float)numOfSublights;
//
//			lightSamples.push_back(new Light(position, color));
//		}
//	}
//	return lightSamples;
//}


Scene* scene = new PhongScene();

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
		scene->draw();
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

