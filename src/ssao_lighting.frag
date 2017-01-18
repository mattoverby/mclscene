#version 330 core
out vec4 out_fragcolor;
in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gDiffuse;
uniform sampler2D gSpec;
uniform sampler2D ssao;

uniform vec3 eye;
float mydot( vec3 a, vec3 b ){ return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]; }

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
vec3 BlinnPhong(Light light, vec3 N, vec3 fragPos, vec3 V, vec3 diff_albedo, vec4 spec_albedo, float mat_ambocc ){

	// L vector (vector pointing toward light source)
	vec3 L = vec3(0,0,0);
	if( light.type==0 ){ L = normalize( light.position - fragPos ); } // point light
	else if( light.type==1 ){ L = -1.f*light.direction; } // directional light
	else if( light.type==2 ){ // spot light
		L = normalize( light.position - fragPos );
		if( mydot(L,-light.direction) < light.halfangle ){ L=vec3(0,0,0); }
	}

	// Ambient
	float amb = 0.05f;
	vec3 ambient = amb * mat_ambocc * diff_albedo;

	// Diffuse 
	float diff = max( dot(N, L), 0.f );
	vec3 diffuse = diff * diff_albedo * light.intensity;

	// Specular
	vec3 H = normalize(L + V);
	float shini = max( 1.f, spec_albedo[3]*128.f ); // glsl pow function needs exp>=1
	float spec = pow( max( mydot(N, H), 0.f), shini );
	vec3 specular = spec * spec_albedo.rgb * light.intensity;

	// Attenuation (falloff)
	float dist = length( light.position - fragPos );
	float atten = 1.f / (light.falloff[0] + light.falloff[1]*dist + light.falloff[2]*dist*dist);

	// Final color
	vec3 result = ambient + ( diffuse + specular )*atten;
	return result;
}


void main(){

	// Retrieve data from gbuffer
	vec3 N = texture(gNormal, TexCoords).rgb;
	if(N == vec3(1.0, 1.0, 1.0)){ discard; } // Skip if no normal (e.g. background)

	vec3 vposition = texture(gPosition, TexCoords).rgb;
	vec3 diffuse = texture(gDiffuse, TexCoords).rgb;
	vec4 specular = texture(gSpec, TexCoords);
	float mat_ambocc = texture(ssao, TexCoords).r;

	// Loop through lighting
	vec3 result = vec3(0,0,0);
	vec3 V = normalize(eye - vposition); // assume eye is origin FIX LATER
	for(int i = 0; i < num_lights; i++){ result += BlinnPhong( lights[i], N, vposition, V, diffuse, specular, mat_ambocc ); }
	out_fragcolor = vec4( result, 1.0 );
}





