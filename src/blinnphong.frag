#version 120

varying vec3 Normal;
varying vec3 FragPos;
varying vec2 TexCoord;

uniform vec3 CamPos;
uniform sampler2D theTexture;
float length2( vec3 v ){ return dot(v,v); }

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
//	Blinn-Phong color
//
vec3 BlinnPhong(Light light, vec3 normal, vec3 fragPos, vec3 viewDir){

	// L vector (vector pointing toward light source)
	vec3 L = vec3(0,0,0);
	if( light.type==0 ){ L = normalize( light.position - fragPos ); } // point light
	else if( light.type==1 ){ L = -1.f*light.direction; } // directional light
	else if( light.type==2 ){ // spot light
		L = normalize( light.position - fragPos );
		float alpha = dot(L,-light.direction);
		if( alpha < light.halfangle ){ L=vec3(0,0,0); }
	}

	// Ambient
	float amb = 0.1f;
	vec3 ambient = amb * material.ambient * light.intensity;

	// Diffuse 
	float diff = max( dot(normal, L), 0.f );
	vec3 diffuse = diff * material.diffuse * light.intensity;

	// Specular
	vec3 halfVec = normalize(L + viewDir);
	float spec = pow( max( dot(normal, halfVec), 1e-6f ), material.shininess );
	if( length2(L)<=0.f ){ spec = 0.f; }
	vec3 specular = spec * material.specular * light.intensity;

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
	vec3 viewDir = normalize(CamPos - FragPos);
	if( dot(normal,viewDir) < 0.0 ){ normal *= -1.0; }

	for(int i = 0; i < num_lights; i++){ result += BlinnPhong( lights[i], normal, FragPos, viewDir ); }

	vec4 texColor = vec4(1,1,1,1);
	if( TexCoord[0]>0.f || TexCoord[1]>0.f ){ texColor = texture2D(theTexture, TexCoord); }
	gl_FragColor = texColor * vec4( result, 1.0 );

} 


