#version 150

//
// Skybox vertex shader
//

uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

in vec3 position;

out vec3 textureCoord;

void main()
{
    textureCoord = position;
    gl_Position = projectionMatrix * viewMatrix * vec4(position, 1.0);
}

