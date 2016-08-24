//#version 330 core
out vec3 Normal;
out vec3 FragPos;
out vec2 TexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform int hastex;

void main()
{
	gl_Position = projection * view * model * gl_Vertex;
	Normal = gl_Normal;//transpose(inverse(model)) * vec4( gl_Normal, 0.f );
	FragPos = gl_Vertex;//vec3( model * gl_Vertex );
	TexCoord = vec2(0,0);
	if( hastex>0 ){ TexCoord = gl_MultiTexCoord0; }
}
