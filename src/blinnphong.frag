//#version 330 core
//out vec4 color;

in vec3 FragPos;
in vec3 Normal;
in vec3 EyePos;

uniform vec3 diffuse;
uniform vec3 specular;
uniform float shininess;
  
//uniform vec3 lightPos; 
//uniform vec3 viewPos;
//uniform vec3 lightColor;
//uniform vec3 objectColor;

void main()
{
	vec3 lightColor = vec3(1,1,1);
	vec3 lightPos = vec3(10,10,0);

	// Ambient
//	float ambientStrength = 0.1f;
//	vec3 c_amb = ambientStrength * lightColor;

	// Angle between light and surface normal
	vec3 norm = normalize(Normal);
	vec3 lightDir = normalize(lightPos - FragPos);
	float cos_theta = dot( norm, lightDir );

	// Diffuse color
	vec3 c_diff = diffuse * lightColor * max( 0.f, cos_theta );

	// Specular color coeff using Warn (1983) method
	vec3 h = normalize( EyePos + lightDir ); // half vector
	float hdn = max( 0.f, dot( h, norm ) );
	vec3 c_spec = specular * lightColor * pow( hdn, shininess );

	// Set the color
	vec3 color = c_diff + c_spec;// + c_amb;
	color.x = min( 1, color.x );
	color.y = min( 1, color.y );
	color.z = min( 1, color.z );
	gl_FragColor = vec4( color, 1.0 );
} 
