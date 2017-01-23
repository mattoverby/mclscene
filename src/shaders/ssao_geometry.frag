#version 330 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gDiffuse;
layout (location = 3) out vec4 gSpec;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;
uniform vec4 diff_color; // color + ambient coeff
uniform vec4 spec_color; // color + shininess

void main()
{    
    // Store the fragment position vector in the first gbuffer texture
    gPosition = FragPos;
    // Also store the per-fragment normals into the gbuffer
    gNormal = normalize(Normal);
    // And the diffuse per-fragment color
    gDiffuse = diff_color;
    // The specular per-frag color
    gSpec = spec_color;
}
