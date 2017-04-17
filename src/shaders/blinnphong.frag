#version 330 core

out vec4 out_fragcolor;
in vec3 FragPos;

//
//	Main
//
void main(){

//	vec3 result = vec3(0,0,0);
//	vec3 normal = normalize(Normal);
//	vec3 viewDir = normalize(CamPos - FragPos);

//	for(int i = 0; i < num_point_lights; i++){
//		result += CalcPointLight( pointLights[i], normal, FragPos, viewDir );
//	}

//	vec4 texColor = vec4(1,1,1,1);
//	if( TexCoord[0]>0.f || TexCoord[1]>0.f ){ texColor = texture2D(theTexture, TexCoord); }
//	gl_FragColor = texColor * vec4( result, 1.0 );
	out_fragcolor = vec4(1,0,0,1);
} 


