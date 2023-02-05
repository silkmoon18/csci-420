#include "basicPipelineProgram.h"
#include "openGLMatrix.h"
#include "imageIO.h"
#include "openGLHeader.h"
#include "glutHeader.h"

#include <iostream>
#include <cstring>
#include <vector>
#include <filesystem>

using namespace std;
using namespace glm;

vec4 calculateColor(float value);
void FindImages();


extern class VertexArrayObject {
public:

	void Draw();
};