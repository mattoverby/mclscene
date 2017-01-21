#version 330 core

layout(location=0) out vec4 out_fragcolor;

uniform vec3 eye;
in vec3 vposition;
in vec3 vnormal;
in vec2 vtexcoord;
uniform sampler2D texSampler;

float mydot( vec3 a, vec3 b ){ return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]; }

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
vec3 BlinnPhong(Light light, vec3 N, vec3 fragPos, vec3 V){

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
	vec3 ambient = material.ambient * light.intensity;

	// Diffuse 
	float diff = max( dot(N, L), 0.f );
	vec3 diffuse = diff * material.diffuse * light.intensity;

	// Specular
	vec3 H = normalize(L + V);
	float shini = max( 1.f, material.shininess ); // glsl pow function needs exp>=1
	float spec = pow( max( mydot(N, H), 0.f), shini );
	vec3 specular = spec * material.specular * light.intensity;

	// Attenuation (falloff)
	float dist = length( light.position - fragPos )*0.25f;
	float atten = 1.f / (light.falloff[0] + light.falloff[1]*dist + light.falloff[2]*dist*dist);

	// Adjust min/max
	vec3 result = ( ambient + diffuse + specular )*atten;
//	vec3 result = min( ( ambient + diffuse + specular )*atten, vec3(1,1,1) );
//	result = max( result, vec3(0,0,0) );

	// Final color
	return result;
}


void main(){

	vec3 result = vec3(0,0,0);
	vec3 N = normalize(vnormal);
	vec3 V = normalize(eye - vposition);
	for(int i = 0; i < num_lights; i++){ result += BlinnPhong( lights[i], N, vposition, V ); }
	vec4 texcolor = texture( texSampler, vtexcoord );
	out_fragcolor = texcolor * vec4( result, 1.0 );
} 


