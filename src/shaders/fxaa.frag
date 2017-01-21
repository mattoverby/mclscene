#version 330 core

// Just a rehash of the same FXAA implementation from Geeks3d:
// http://www.geeks3d.com/20110405/

in vec2 TexCoords;
out vec4 outcolor;
uniform sampler2D in_texture;
uniform vec2 screen_size;

#define FXAA_SPAN_MAX 8.0
#define FXAA_REDUCE_MUL 1.0/16.0 // 0 = most blurry, 1 = no blur
#define FXAA_REDUCE_MIN 1.0/128.0

void main( ) {

	vec2 fxaa_span_max = vec2( FXAA_SPAN_MAX, FXAA_SPAN_MAX );
	vec3 rgbNW=texture(in_texture,TexCoords+(vec2(-1.0,-1.0)/screen_size)).xyz;
	vec3 rgbNE=texture(in_texture,TexCoords+(vec2(1.0,-1.0)/screen_size)).xyz;
	vec3 rgbSW=texture(in_texture,TexCoords+(vec2(-1.0,1.0)/screen_size)).xyz;
	vec3 rgbSE=texture(in_texture,TexCoords+(vec2(1.0,1.0)/screen_size)).xyz;
	vec3 rgbM =texture(in_texture,TexCoords).xyz;

	vec3 luma=vec3(0.299, 0.587, 0.114);
	float lumaNW = dot(rgbNW, luma);
	float lumaNE = dot(rgbNE, luma);
	float lumaSW = dot(rgbSW, luma);
	float lumaSE = dot(rgbSE, luma);
	float lumaM  = dot(rgbM,  luma);

	float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
	float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

	vec2 dir;
	dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
	dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));

	float dirReduce = max( (lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_REDUCE_MUL), FXAA_REDUCE_MIN );
	float rcpDirMin = 1.0/(min(abs(dir.x), abs(dir.y)) + dirReduce);
	dir = min(fxaa_span_max, max(-fxaa_span_max, dir*rcpDirMin)) / screen_size;
	vec3 rgbA = 0.5*(texture(in_texture, TexCoords.xy + dir * (1.0/3.0-0.5)).xyz +  texture(in_texture, TexCoords.xy + dir * (2.0/3.0-0.5)).xyz);
	vec3 rgbB = rgbA*0.5 + 0.25*(texture(in_texture, TexCoords.xy + dir*-0.5).xyz + texture(in_texture, TexCoords.xy + dir*0.5).xyz);
	float lumaB = dot(rgbB, luma);

	outcolor.a = 1.0;
	if( (lumaB < lumaMin) || (lumaB > lumaMax) ){ outcolor.xyz=rgbA; }
	else{ outcolor.xyz=rgbB; }
}
