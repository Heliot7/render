#version 330
#define MAX_LIGHTS 8

in vec4  passColour;
in vec2  passTexcoord;
in vec4  eyeVertexPosition;
in vec3  eyeNormal;
in vec3  lightDir[MAX_LIGHTS];
in vec3  cameraVector;
in float attenuation[MAX_LIGHTS];
in float depth;

// lighting:
uniform vec3 eyeLightPosition;
uniform vec3 ambientLight;
uniform vec3 diffuseLight[MAX_LIGHTS];
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
    // PHONG LIGHTING
	vec3 finalColour = vec3(0,0,0);
	vec3 partialTex = vec3(0,0,0);
	vec4 texColour = texture(tex, passTexcoord);
	vec3 N = normalize(eyeNormal); // normal
	for(int i = 0; i < MAX_LIGHTS; ++i)
	{
		// -> AMBIENT LIGHT
		finalColour += ambientLight*diffuseLight[i]*passColour.rgb;
		if(passColour.a == 0.0)
			finalColour += ambientLight*diffuseLight[i]*ambientMaterial;
		partialTex += ambientLight*diffuseLight[i]*texColour.rgb;

		// -> DIFFUSE LIGHT
		vec3 L = normalize(lightDir[i]);
		float lambertTerm = dot(N,L);
		lambertTerm = max(0.0, lambertTerm);
		finalColour += attenuation[i]*lambertTerm*diffuseLight[i]*diffuseMaterial; // / (MAX_LIGHTS/2);
		partialTex += attenuation[i]*lambertTerm*diffuseLight[i]*texColour.rgb; // / (MAX_LIGHTS/2);
		
		// -> SPECULAR REFLECTION (NOT USED)
		if(lambertTerm > 0.0)
		{	
			vec3 R = reflect(-L,N);
			vec3 E = normalize(cameraVector.xyz); // xzy
			float specular = pow( max(dot(R,E), 0.0), shininessMaterial);
			finalColour += attenuation[i]*specular*specularMaterial*specularLight; // / (MAX_LIGHTS/2);
			partialTex += attenuation[i]*specular*specularMaterial*specularLight; // / (MAX_LIGHTS/2);
		}
	}
    
	// FINAL RESULT
	outColour = vec4(finalColour.rgb, 1.0);
	// -> COLOUR
	if(texColour.a > 0.0)
		outColour = vec4(partialTex.rgb, texColour.a);
	// -> DEPTH
	outDepth = vec4(1.0 - min(1.0, depth/10.0), 1.0 - min(1.0, depth/10.0), 1.0 - min(1.0, depth/10.0), 1.0);
}
