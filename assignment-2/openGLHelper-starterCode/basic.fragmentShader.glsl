#version 150

const int MAX_LIGHT_AMOUNT = 10;

uniform int numOfLights;
uniform vec4 lightAmbients[MAX_LIGHT_AMOUNT];
uniform vec4 lightDiffuses[MAX_LIGHT_AMOUNT];
uniform vec4 lightSpeculars[MAX_LIGHT_AMOUNT];
uniform vec4 ambientCoef;
uniform vec4 diffuseCoef;
uniform vec4 specularCoef;
uniform float materialShininess; 

in vec4 col;
in vec3 eyeVector; 
in vec3 vertexNormal;
in vec3 lightVectors[MAX_LIGHT_AMOUNT];

out vec4 c;

void main()
{
  // calculate lighting
  vec4 ambient, diffuse; 
  vec4 specular = vec4(0, 0, 0, 1);
  for (int i = 0; i < numOfLights; i++) {
      ambient += ambientCoef * lightAmbients[i]; 

      float ndotl = max(dot(vertexNormal, lightVectors[i]), 0.0); 
      diffuse += diffuseCoef * lightDiffuses[i] * ndotl;
  
      vec3 R = normalize(reflect(-lightVectors[i], vertexNormal));
      float rdotv = max(dot(R, eyeVector), 0.0);
  
      if (ndotl > 0.0) {
        specular += specularCoef * lightSpeculars[i] * pow(rdotv, materialShininess);
      }
  }

  // compute the final pixel color
  c = (ambient + diffuse + specular) * col;
  c.w = 1;
  // debug
}

