//#version 330 core
out vec3 Normal;
out vec3 FragPos;
out vec3 EyePos;

//uniform mat4 model;
//uniform mat4 view;
//uniform mat4 projection;

void main()
{
	gl_Position = ftransform();
	Normal = gl_Normal;
	FragPos = gl_Position;
	EyePos = vec3(gl_ModelViewMatrix * gl_Vertex);
}
