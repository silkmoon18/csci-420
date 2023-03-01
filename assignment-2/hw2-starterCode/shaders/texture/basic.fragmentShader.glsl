#version 150

//
// Texture fragment shader
//

const int MAX_LIGHT_AMOUNT = 100;

uniform int numOfLights;
uniform int lightModes[MAX_LIGHT_AMOUNT];
uniform vec3 lightDirections[MAX_LIGHT_AMOUNT];
uniform vec3 lightPositions[MAX_LIGHT_AMOUNT];
uniform vec4 lightAmbients[MAX_LIGHT_AMOUNT];
uniform vec4 lightDiffuses[MAX_LIGHT_AMOUNT];
uniform vec4 lightSpeculars[MAX_LIGHT_AMOUNT];
uniform vec4 ambientCoef;
uniform vec4 diffuseCoef;
uniform vec4 specularCoef;
uniform float materialShininess; 
uniform vec3 eyePosition;
uniform int isLightingEnabled;
uniform sampler2D textureImage2D; 
uniform samplerCube textureImageCube;
uniform int textureTypeId;

in vec4 col;
in vec3 vertexNormal;
in vec3 fragmentPosition;
in vec2 planeTexCoord;
in vec3 cubeTexCoord;

out vec4 c;

void main()
{
    vec4 color;
    if (textureTypeId == 0) {
        color = texture(textureImage2D, planeTexCoord) * col;
    }
    else {
        color = texture(textureImageCube, cubeTexCoord) * col;
    }

    if (isLightingEnabled == 1) {
        vec4 ambient = vec4(0, 0, 0, 1);
        vec4 diffuse = vec4(0, 0, 0, 1);; 
        vec4 specular = vec4(0, 0, 0, 1);
        for (int i = 0; i < numOfLights; i++) {
        
            float q = distance(lightPositions[i], fragmentPosition);
            float att = 1.0 / (1 + 0.001 * q * q);

            vec3 lightVector;
            if (lightModes[i] == 0) {
                lightVector = normalize(-lightDirections[i]);
            }
            else if (lightModes[i] == 1) {
                lightVector = normalize(lightPositions[i] - fragmentPosition);
            }
            ambient += ambientCoef * lightAmbients[i] * att; 

            float ndotl = max(dot(lightVector, vertexNormal), 0.0); 
            diffuse += diffuseCoef * lightDiffuses[i] * ndotl * att;
  
            vec3 R = normalize(reflect(-lightVector, vertexNormal));
            vec3 eyeVector = normalize(eyePosition - fragmentPosition);
            float rdotv = max(dot(R, eyeVector), 0.0);
       
            if (materialShininess > 0) {
                specular += specularCoef * lightSpeculars[i] * pow(rdotv, materialShininess) * att;
            }      


        }
        // compute the final pixel color
        c = (ambient + diffuse) * color + specular;
    }
    else {
        c = color;
    }
    c.w = 1;
}

