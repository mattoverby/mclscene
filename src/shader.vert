#version 330 core

layout(location=0) in vec3 in_position;
layout(location=1) in vec3 in_color;
layout(location=2) in vec3 in_normal;
layout(location=3) in vec2 in_texcoord;

out vec3 vposition;
out vec3 vcolor;
out vec3 vnormal;
out vec2 vtexcoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	vec4 pos = projection * view * model * vec4(in_position,1.0);
	vcolor = in_color;
	vnormal = in_normal;
	vposition = vec3( pos );
	vtexcoord = in_texcoord;
	gl_Position = pos;
}