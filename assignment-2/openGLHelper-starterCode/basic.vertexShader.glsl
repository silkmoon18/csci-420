#version 150

const float eps = 0.00001f;

in vec3 position;
in vec4 color;
out vec4 col;

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;

uniform vec4 lightPosition;

out vec4 eyePosition;
out mat4 normalMatrix;
out vec3 vertexNormal;
out vec3 lightVector;

void main()
{
  // calculate lighting 
  eyePosition = modelViewMatrix * vec4(position, 1.0);
  normalMatrix = transpose(inverse(modelViewMatrix));
  vertexNormal = normalize(vec3(normalMatrix * vec4(position, 0.0)));
  lightVector = normalize(vec3(lightPosition - eyePosition));

  gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0f);
  col = color / 255.0f;
  col.w = 1.0f;
}

