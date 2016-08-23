//#version 330 core

in vec3 FragPos;
in vec3 Normal;
uniform vec3 CamPos;

//
//	Materials
//
struct Material {
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float shininess;
}; 
uniform Material material;

//
//	Lights
//
struct PointLight {
	vec3 position;
	vec3 intensity;
};
uniform int num_point_lights;
#define MAX_NUM_LIGHTS 8
uniform PointLight pointLights[MAX_NUM_LIGHTS];

//
//	Color from a point light
//
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir){

	vec3 lightDir = normalize( light.position - fragPos );

	// Ambient
	float amb = 0.1f;
	vec3 ambient = amb * material.ambient * light.intensity;

	// Diffuse 
	float diff = max( dot(normal, lightDir), 0.f );
	vec3 diffuse = diff * material.diffuse * light.intensity;

	// Specular
	vec3 halfVec = normalize(lightDir + viewDir);
	float spec = pow( max( dot(normal, halfVec), 0.f ), material.shininess );
	vec3 specular = spec * material.specular * light.intensity;

	// Attenuation (falloff)
	float dist = length( light.position - fragPos )*0.25f;
	float atten = 1.f / (1.f + 0.1f*dist + 0.01f*dist*dist);

	// Final color
	return ( ambient + diffuse + specular )*atten;
}

//
//	Main
//
void main(){

	vec3 result = (0,0,0);
	vec3 normal = normalize(Normal);
	vec3 viewDir = normalize(CamPos - FragPos);

	for(int i = 0; i < num_point_lights; i++){
		result += CalcPointLight(pointLights[i], normal, FragPos, viewDir );
	}

	gl_FragColor = vec4( result, 1.0 );
} 


