#include "basicPipelineProgram.h"
#include "openGLHeader.h"
#include <iostream>
#include <cstring>

using namespace std;

int BasicPipelineProgram::Init(const char* shaderBasePath) {
	if (BuildShadersFromFiles(shaderBasePath, "basic.vertexShader.glsl", "basic.fragmentShader.glsl") != 0) {
		cout << "Failed to build the pipeline program." << endl;
		return 1;
	}

	cout << "Successfully built the pipeline program." << endl;
	return 0;
}

void BasicPipelineProgram::SetModelMatrix(const float* m) {
	// Pass "m" to the pipeline program, as the model matrix.
	glUniformMatrix4fv(h_modelMatrix, 1, GL_FALSE, m);
}
void BasicPipelineProgram::SetViewMatrix(const float* m) {
	// Pass "m" to the pipeline program, as the view matrix.
	glUniformMatrix4fv(h_viewMatrix, 1, GL_FALSE, m);
}
void BasicPipelineProgram::SetProjectionMatrix(const float* m) {
	// Pass "m" to the pipeline program, as the projection matrix.
	glUniformMatrix4fv(h_projectionMatrix, 1, GL_FALSE, m);
}
void BasicPipelineProgram::SetNormalMatrix(const float* m) {
	// Pass "m" to the pipeline program, as the view matrix.
	glUniformMatrix4fv(h_normalMatrix, 1, GL_FALSE, m);
}
int BasicPipelineProgram::SetShaderVariableHandles() {
	// Set h_modelMatrix, h_viewMatrix, h_projectionMatrix and h_normalMatrix.
	SET_SHADER_VARIABLE_HANDLE(modelMatrix);
	SET_SHADER_VARIABLE_HANDLE(viewMatrix);
	SET_SHADER_VARIABLE_HANDLE(projectionMatrix);
	SET_SHADER_VARIABLE_HANDLE(normalMatrix);
	return 0;
}

