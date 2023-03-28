/* **************************
 * CSCI 420
 * Assignment 3 Raytracer
 * Name: Baihua Yang
 * *************************
*/

#include "utility.h"



vector<const char*> inputFiles;
Scene* scene;



void init() {
	glMatrixMode(GL_PROJECTION);
	glOrtho(0, WIDTH, 0, HEIGHT, 1, -1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
}

void display() {
}

void idle() {
	//hack to make it only draw once
	static int once = 0;
	if (!once) {
		std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();


		scene->render();


		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
		printf("delta time = %fs\n", std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() * 1e-6);
	}
	once = 1;
}

void readMode() {
	int type = 0;

	cout << "\n1. Single file\n2. Multiple files\n";
	while (true) {
		cout << "Choose a process mode: \n";
		cin >> type;
		if (type == 1) {
			char ans;
			while (true) {
				cout << "Do you want to display the result? (Y/N) \n";
				cin >> ans;
				ans = tolower(ans);
				if (ans == 'y') {
					scene->mode = MODE_DISPLAY;
					break;
				}
				else if (ans == 'n') {
					scene->mode = MODE_JPEG;
					break;
				}
				cout << ans << " is not valid.\n";
			}
			cout << endl;
			break;
		}
		else if (type == 2) {
			scene->mode = MODE_JPEG;
			break;
		}
		cout << type << " is not valid.\n";
	}
	cout << endl;
}
void readSceneType() {
	int type = 0;

	cout << "\n1. Phong\n2. Optical\n";
	while (true) {
		cout << "Choose a scene type: \n";
		cin >> type;

		if (type == 1) {
			scene = new PhongScene();
			break;
		}
		else if (type == 2) {
			scene = new OpticalScene();
			break;
		}
		cout << type << " is not valid.\n";
	}
	cout << endl;
}
void readFiles() {
	string path;
	while (true) {
		cout << "Enter the path to a scene file or a directory containing scene files: \n";
		cin >> path;

		filesystem::path input = path;
		if (filesystem::is_directory(input)) {
			for (const auto& entry : filesystem::directory_iterator(input)) {
				filesystem::path p = entry.path();

				string extension = p.extension().string();
				if (extension == ".scene") {
					inputFiles.push_back(p.string().c_str());
				}
			}
			int size = inputFiles.size();
			if (size > 0) {
				printf("Found %d scene files in %s. \n", size, path.c_str());
				break;
			}
			printf("No scene files are found in %s. \n", path.c_str());
		}
		else if (input.extension().string() == ".scene") {
			inputFiles.push_back(input.string().c_str());
			printf("Input file: %s\n", input.string().c_str());
			break;
		}
		printf("%s is not a valid path. \n", path.c_str());
	}
}
void readAntiAliasingLevel() {
	int level = 0;
	cout << "Enter the level of anti-aliasing (# subpixels = 2 ^ antiAliasingLevel): \n";
	cin >> level;
	scene->setAntiAliasingLevel(level);
}
void readSoftShadowLevel() {
	int level = 0;
	cout << "Enter the level of soft shadows (# light samples = 2 ^ softShadowLevel): \n";
	cin >> level;
	scene->setSoftShadowLevel(level);
}
void readThreadNumber() {
	int num = 0;
	cout << "Enter the number of threads to use: \n";
	cin >> num;
	scene->setNumOfThreads(num);
}

int main(int argc, char** argv) {
	readSceneType();

	readMode();

	readFiles();

	readAntiAliasingLevel();

	readSoftShadowLevel();

	readThreadNumber();



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

