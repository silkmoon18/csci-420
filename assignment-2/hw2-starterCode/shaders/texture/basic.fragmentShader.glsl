#version 150

//
// Texture fragment shader
//
struct DirectionalLight {
    vec3 direction;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
};  
struct PointLight { 
    vec3 position;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    vec3 attenuation;
};
struct Material {
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    float shininess; 
};


const int MAX_LIGHT_AMOUNT = 100;

uniform int numOfPointLights;
uniform DirectionalLight directionalLight;
uniform PointLight pointLights[MAX_LIGHT_AMOUNT];
uniform Material material;
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


vec4 calculateDirectionalLight() {
    vec4 ambient;
    vec4 diffuse; 
    vec4 specular;
    vec3 lightVector = normalize(-directionalLight.direction);
    
    ambient = material.ambient * directionalLight.ambient; 

    float ndotl = max(dot(lightVector, vertexNormal), 0.0); 
    diffuse = material.diffuse * directionalLight.diffuse * ndotl;
  
    vec3 R = normalize(reflect(-lightVector, vertexNormal));
    vec3 eyeVector = normalize(eyePosition - fragmentPosition);
    float rdotv = max(dot(R, eyeVector), 0.0);
       
    if (material.shininess > 0) {
        specular = material.specular * directionalLight.specular * pow(rdotv, material.shininess);
    }
    return ambient + diffuse + specular;
}

vec4 calculatePointLight(PointLight light) {
    vec4 ambient;
    vec4 diffuse; 
    vec4 specular;
    vec3 lightVector = normalize(light.position - fragmentPosition);
    float q = distance(light.position, fragmentPosition);
    float att = 1.0 / (light.attenuation.x + light.attenuation.y * q + light.attenuation.z * q * q);
    
    ambient = material.ambient * light.ambient; 

    float ndotl = max(dot(lightVector, vertexNormal), 0.0); 
    diffuse = material.diffuse * light.diffuse * ndotl;
  
    vec3 R = normalize(reflect(-lightVector, vertexNormal));
    vec3 eyeVector = normalize(eyePosition - fragmentPosition);
    float rdotv = max(dot(R, eyeVector), 0.0);
       
    if (material.shininess > 0) {
        specular = material.specular * light.specular * pow(rdotv, material.shininess);
    }
    return (ambient + diffuse + specular) * att;
}

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
        vec4 lighting = calculateDirectionalLight();

        for (int i = 0; i < numOfPointLights; i++) {
            lighting += calculatePointLight(pointLights[i]);
        }
        c = color * lighting;
    }
    else {
        c = color;
    }
    c.w = 1;
}

