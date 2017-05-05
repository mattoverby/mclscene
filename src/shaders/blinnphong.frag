#version 330 core

out vec4 out_fragcolor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 eye;
uniform sampler2D theTexture;

//
//	Lights
//
#define MAX_NUM_LIGHTS 8
struct Light {
	vec3 position;
	vec3 direction;
	vec3 intensity;
	vec3 falloff;
	float halfangle;
	int type; // 0 = spot, 1 = directional, 2 = spot
};
uniform int num_lights;
uniform Light lights[MAX_NUM_LIGHTS];

//
//	Materials
//
struct Material {
	vec3 ambient;
	vec3 diffuse;
	vec4 specular; // 4th term shininess
}; 
uniform Material material;


//
//	Color from a point light
//
vec3 blinnphong(Light light, vec3 normal, vec3 fragPos, vec3 viewDir){

	// L vector (vector pointing toward light source)
	vec3 lightDir = vec3(0,0,0);
	if( light.type==0 ){ lightDir = normalize( light.position - fragPos ); } // point light
	else if( light.type==1 ){ lightDir = -1.f*light.direction; } // directional light
	else if( light.type==2 ){ // spot light
		lightDir = normalize( light.position - fragPos );
		if( dot(lightDir,-light.direction) < light.halfangle ){ lightDir=vec3(0,0,0); }
	}

	// Ambient
	float amb = 0.1f;
	vec3 ambient = amb * material.ambient * light.intensity;

	// Diffuse 
	float diff = max( dot(normal, lightDir), 0.f );
	vec3 diffuse = diff * material.diffuse * light.intensity;

	// Specular
	vec3 halfVec = normalize(lightDir + viewDir);
	float spec = pow( max( dot(normal, halfVec), 1e-6f ), material.specular[3] );
	vec3 specular = spec * material.specular.xyz * light.intensity;

	// Attenuation (falloff)
	float dist = length( light.position - fragPos )*0.25f;
	float atten = 1.f / (light.falloff[0] + light.falloff[1]*dist + light.falloff[2]*dist*dist);

	// Final color
	return ( ambient + diffuse + specular )*atten;
}


//
//	Main
//
void main(){

	vec3 result = vec3(0,0,0);
	vec3 normal = normalize(Normal);
	vec3 viewDir = normalize(eye - FragPos);

	for(int i = 0; i < num_lights; ++i){
		result += blinnphong( lights[i], normal, FragPos, viewDir );
	}

	vec4 texColor = texture(theTexture, TexCoords);
	out_fragcolor = texColor*vec4( result, 1.0 );
} 


