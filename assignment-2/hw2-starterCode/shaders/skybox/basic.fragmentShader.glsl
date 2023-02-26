#version 150

//
// Skybox fragment shader
//

uniform samplerCube textureImageCube;

in vec4 col;
in vec3 textureCoord;

out vec4 c;

void main()
{
    c = texture(textureImageCube, textureCoord) * col;
}

