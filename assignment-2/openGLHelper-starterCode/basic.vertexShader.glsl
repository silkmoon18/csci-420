#version 150

const int MAX_LIGHT_AMOUNT = 10;
const float eps = 0.00001f;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform mat4 normalMatrix;
uniform int numOfLights;
uniform vec3 lightPositions[MAX_LIGHT_AMOUNT];
uniform vec3 eyePosition;

in vec3 position;
in vec4 color;

out vec4 col;
out vec3 eyeVector;
out vec3 vertexNormal;
out vec3 lightVectors[MAX_LIGHT_AMOUNT];

void main()
{
  // calculate lighting 
  vec3 pointPosition = vec3(modelMatrix * vec4(position, 1.0));
  eyeVector = normalize(eyePosition - pointPosition);
  vertexNormal = normalize(vec3(normalMatrix * vec4(position, 1.0)));
  for (int i = 0; i < numOfLights; i++) {
	lightVectors[i] = normalize(lightPositions[i] - pointPosition);
  }

  gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(position, 1.0f);
  col = color / 255.0f;
  col.w = 1.0f;
}

