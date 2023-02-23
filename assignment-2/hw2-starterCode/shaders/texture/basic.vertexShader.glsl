#version 150

const float eps = 0.00001f;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform mat4 normalMatrix;

in vec3 position;
in vec4 color;
in vec3 normal;
in vec2 texCoord;

out vec4 col;
out vec3 vertexNormal;
out vec3 fragmentPosition;
out vec2 textureCoord;

void main()
{
  vec3 pointPosition = vec3(modelMatrix * vec4(position, 1.0));
  fragmentPosition = pointPosition;
  vertexNormal = normalize(vec3(normalMatrix * vec4(normal, 1.0)));

  gl_Position = projectionMatrix * viewMatrix * vec4(pointPosition, 1.0);
  col = color / 255.0f;

  textureCoord = texCoord;
}

