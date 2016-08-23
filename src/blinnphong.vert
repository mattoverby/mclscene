//#version 330 core
out vec3 Normal;
out vec3 FragPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	gl_Position = projection * view * model * gl_Vertex;
	Normal = gl_Normal;//transpose(inverse(model)) * vec4( gl_Normal, 0.f );
	FragPos = gl_Vertex;//vec3( model * gl_Vertex );
}
