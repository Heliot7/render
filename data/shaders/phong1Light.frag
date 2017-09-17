#version 330

in vec4  passColour;
in vec2  passTexcoord;
in vec4  eyeVertexPosition;
in vec3  eyeNormal;
in vec3  lightDir;
in vec3  cameraVector;
in float attenuation;
in float depth;

// lighting:
uniform vec3 eyeLightPosition;
uniform vec3 ambientLight;
uniform vec3 diffuseLight;
uniform vec3 specularLight;

// material:
uniform sampler2D tex;
uniform vec3      ambientMaterial;
uniform vec3      diffuseMaterial;
uniform vec3      specularMaterial;
uniform float     shininessMaterial;

// out vec4 outColour;
layout (location = 0) out vec4 outColour;
layout (location = 1) out vec4 outDepth;

void main()
{
    // Phong lighing
	vec3 finalColour = passColour.rgb*ambientLight + attenuation*ambientLight*ambientMaterial;
    vec3 N = normalize(eyeNormal);
    vec3 L = normalize(lightDir);
    float lambertTerm = dot(N,L);

    if(lambertTerm > 0.0)
    {
		finalColour += (lambertTerm*attenuation) * (diffuseLight*diffuseMaterial);
		vec3 E = normalize(cameraVector.xyz); // xzy
		vec3 R = reflect(-L,N);
		float specular = pow( max(dot(R,E), 0.0), shininessMaterial);
		finalColour += (specular*attenuation)*(specularLight*specularMaterial);
    }

    // Diffuse lighting
	vec4 texColour = texture(tex, passTexcoord);
	vec3 mixColour = (finalColour.rgb + texColour.a*texColour.rgb) / (1.0 + texColour.a);
	outColour = vec4(mixColour, 1.0);
	outDepth = vec4(depth/10.0, depth/10.0, depth/10.0, 1.0);
}
