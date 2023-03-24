#include "utility.h"

int mode = MODE_DISPLAY;
char* filename = NULL;
unsigned char buffer[HEIGHT][WIDTH][3];

float getRandom(float min, float max) {
	float f = (float)rand() / RAND_MAX;
	return min + f * (max - min);
}
float calculateArea(vec3 a, vec3 b, vec3 c) {
	return 0.5f * (((b.x - a.x) * (c.y - a.y)) - ((c.x - a.x) * (b.y - a.y)));
}
int compare(float f1, float f2) {
	int result = 1;
	float diff = f1 - f2;
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
void parse_vec3(FILE* file, const char* check, vec3& vec) {
	char str[100];
	fscanf(file, "%s", str);
	parse_check(check, str);
	fscanf(file, "%f %f %f", &vec.x, &vec.y, &vec.z);
	printf("%s %f %f %f\n", check, vec.x, vec.y, vec.z);
}
void parse_float(FILE* file, const char* check, float& f) {
	char str[512];
	int ret = fscanf(file, "%s", str);
	ASERT(ret == 1);

	parse_check(check, str);

	ret = fscanf(file, "%f", &f);
	ASERT(ret == 1);

	printf("%s %f\n", check, f);
}

void parse_rad(FILE* file, float* r) {
	char str[100];
	fscanf(file, "%s", str);
	parse_check("rad:", str);
	fscanf(file, "%f", r);
	printf("rad: %f\n", *r);
}
void parse_shi(FILE* file, float* shi) {
	char s[100];
	fscanf(file, "%s", s);
	parse_check("shi:", s);
	fscanf(file, "%f", shi);
	printf("shi: %f\n", *shi);
}