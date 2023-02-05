#version 150

in vec4 col;
out vec4 c;

in vec4 eyePosition; 
in vec3 vertexNormal;
in vec3 lightVector;

    
uniform vec4 lightAmbient; 
uniform vec4 lightDiffuse; 
uniform vec4 lightSpecular;
    
uniform vec4 ambientCoef;
uniform vec4 diffuseCoef;
uniform vec4 specularCoef;
uniform float materialShininess; 

void main()
{
  vec3 eye_vector = normalize(-vec3(eyePosition));
  
  vec4 ambient = ambientCoef * lightAmbient; 
  float ndotl = max(dot(vertexNormal, lightVector), 0.0); 
  
  vec4 diffuse = diffuseCoef * lightDiffuse * ndotl;
  
  vec3 R = normalize(2.0 * ndotl * vertexNormal - lightVector);
  float rdotv = max(dot(R, eye_vector), 0.0);
  
  vec4 specular = vec4(0, 0, 0, 1);
  if (ndotl > 0.0) {
    specular = specularCoef * lightSpecular * pow(rdotv, materialShininess);
  }

  // compute the final pixel color
  c = (ambient + diffuse) * col + specular;
}

