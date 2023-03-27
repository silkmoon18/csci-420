/* **************************
 * CSCI 420
 * Assignment 3 Raytracer
 * Name: Baihua Yang
 * *************************
*/

#include "utility.h"
#include <chrono>


unsigned char buffer[HEIGHT][WIDTH][3]; // rgb in (0, 255)
int mode = MODE_DISPLAY;
char* filename = NULL; // ouput jpg filename

Timer timer;
OpticalScene* scene = new OpticalScene();

vector<Pixel> pixels;
const int numOfThreads = 32;

atomic_int numOfCompletedPixels = 0;


void plot_pixel_display(int x, int y, unsigned char r, unsigned char g, unsigned char b) {
	glColor3f(((float)r) / 255.0f, ((float)g) / 255.0f, ((float)b) / 255.0f);
	glVertex2i(x, y);
}
void plot_pixel(int x, int y, vec3 color) {
	unsigned char r = color.x;
	unsigned char g = color.y;
	unsigned char b = color.z;
	plot_pixel_display(x, y, r, g, b);
}
void save_jpg() {
	printf("Saving JPEG file: %s\n", filename);

	for (auto& pixel : pixels) {
		vec3 color = clamp(pixel.color * 255.0f, vec3(0.0f), vec3(255.0f));
		buffer[pixel.index.y][pixel.index.x][0] = (int)color.x;
		buffer[pixel.index.y][pixel.index.x][1] = (int)color.y;
		buffer[pixel.index.y][pixel.index.x][2] = (int)color.z;
	}

	ImageIO img(WIDTH, HEIGHT, 3, &buffer[0][0][0]);
	if (img.save(filename, ImageIO::FORMAT_JPEG) != ImageIO::OK)
		printf("Error in Saving\n");
	else
		printf("File saved Successfully\n");
}



void printProgress() {
	while (true) {
		timer.setCurrentTime(glutGet(GLUT_ELAPSED_TIME));
		int progress = int((float)numOfCompletedPixels / pixels.size() * 100);
		printf("\r%d%% (%d / %d)", progress, (int)numOfCompletedPixels, pixels.size());

		if (progress == 100) {
			printf("\n");
			break;
		}
	}
}
void initializePixels() {
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

void drawPixel(Pixel& pixel) {
	pixel.color += scene->calculatePixelColor(pixel);

	numOfCompletedPixels++;
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
	for (int i = 0; i < 1; i++) {
		numOfCompletedPixels = 0;
		scene->numOfSampleLights = 1;
		scene->sampleLights();
		scene->numOfSampleLights = 1;

		std::vector<std::thread> threads;
		for (int j = 0; j < numOfThreads; j++) {
			std::thread t(drawPixels, j);
			threads.push_back(std::move(t));
		}
		threads.push_back(move(thread(printProgress)));

		for (auto& thread : threads) {
			thread.join();
		}
		threads.clear();

		cout << i + 1 << " done. " << endl;

		for (unsigned int x = 0; x < WIDTH; x++) {
			glPointSize(2.0);
			glBegin(GL_POINTS);
			for (unsigned int y = 0; y < HEIGHT; y++) {
				int index = x * HEIGHT + y;
				glColor3f(pixels[index].color.x, 
						  pixels[index].color.y, 
						  pixels[index].color.z);
				glVertex2i(x, y);
			}

			glEnd();
			glFlush();
		}
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
	else if (argc == 2) {
		mode = MODE_DISPLAY;
	}
	printf("Input file: %s\n", argv[1]);

	glutInit(&argc, argv);

	initializePixels();
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

