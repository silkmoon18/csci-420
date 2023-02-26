#version 150

//
// Skybox vertex shader
//

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

in vec3 position;
in vec4 color;

out vec4 col;
out vec3 textureCoord;

void main()
{
    textureCoord = position;
    mat4 view = mat4(mat3(viewMatrix));
    gl_Position = projectionMatrix * view * modelMatrix * vec4(position, 1.0);
    col = color / 255.0f;
}

