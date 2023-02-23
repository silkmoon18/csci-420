#version 150

const int MAX_LIGHT_AMOUNT = 10;

uniform int numOfLights;
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

in vec4 col;
in vec3 vertexNormal;
in vec3 fragmentPosition;

out vec4 c;

void main()
{
  if (isLightingEnabled == 1) {
      vec4 ambient = vec4(0, 0, 0, 1);
      vec4 diffuse = vec4(0, 0, 0, 1);; 
      vec4 specular = vec4(0, 0, 0, 1);
      for (int i = 0; i < numOfLights; i++) {
          vec3 lightVector = normalize(lightPositions[i] - fragmentPosition);
          ambient += ambientCoef * lightAmbients[i]; 

          float ndotl = max(dot(lightVector, vertexNormal), 0.0); 
          diffuse += diffuseCoef * lightDiffuses[i] * ndotl;
  
          vec3 R = normalize(reflect(-lightVector, vertexNormal));
          vec3 eyeVector = normalize(eyePosition - fragmentPosition);
          float rdotv = max(dot(R, eyeVector), 0.0);
       
          if (materialShininess > 0) {
            specular += specularCoef * lightSpeculars[i] * pow(rdotv, materialShininess);
          }      
      }
      // compute the final pixel color
      c = (ambient + diffuse) * col + specular;
  }
  else {
    c = col;
  }
  // debug
//  c = col;
}

