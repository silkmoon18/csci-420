#version 150

//
// Skybox fragment shader
//

uniform samplerCube skybox;

in vec3 textureCoord;

out vec4 c;

void main()
{
    c = texture(skybox, textureCoord);
}

