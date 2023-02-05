#version 150

const float eps = 0.00001f;

in vec3 position;
in vec4 color;
out vec4 col;

in vec4 neighborHeights;

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;

uniform vec4 lightPosition;

uniform int mode = 0;

out vec4 eyePosition;
out mat4 normalMatrix;
out vec3 vertexNormal;
out vec3 lightVector;

void main()
{

  eyePosition = modelViewMatrix * vec4(position, 1.0);
  normalMatrix = transpose(inverse(modelViewMatrix));

  vertexNormal = normalize(vec3(normalMatrix * vec4(position, 0.0)));

  lightVector = normalize(vec3(lightPosition - eyePosition));

  // compute the transformed and projected vertex position (into gl_Position) 
  // compute the vertex color (into col)
  switch (mode) {
	case 0:
		gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0f);
		col = color;
		break;

	case 1:
		float smoothenedHeight = (neighborHeights.x + neighborHeights.y + neighborHeights.z + neighborHeights.w) / 4.0f;
		gl_Position = projectionMatrix * modelViewMatrix * vec4(position.x, smoothenedHeight, position.z, 1.0f);

		vec4 smoothenedColor = smoothenedHeight * max(color, vec4(eps)) / max(position.y, eps);
		col = smoothenedColor;

		break;

	default:
		break;
		
  }
  col /= 255.0f;
  col.w = 1.0f;
}

