#version 150
#define MAX_LIGHTS 8

in vec3 position;
in vec4 colour;
in vec2 texcoord;
in vec3 normal;

// scene transformations:
uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

// lighting:
uniform vec3 lightPosition[MAX_LIGHTS];
uniform vec3 lightAttenuation;

out vec4 passColour;
out vec2 passTexcoord;
out vec4 eyeVertexPosition;
out vec3 eyeNormal;
out vec3 lightDir[MAX_LIGHTS];
out vec3 cameraVector;
out float attenuation[MAX_LIGHTS];
out float depth;

void main()
{
    // Eye space normal:
	mat3 normalMatrix = transpose(inverse(mat3(model)));
    eyeNormal = normalMatrix*(vec4(normal, 1.0)).xyz; // eyeNormal = normal;
    // Eye space vertex position:
    eyeVertexPosition = view * model * vec4(position, 1.0);
	// Calculate Camera vector with pixel
    cameraVector = -eyeVertexPosition.xyz;
    
	for(int i = 0; i < MAX_LIGHTS; ++i)
	{
		// CALCULATE VERTEX<->LIGHT DIRECTIONS
		lightDir[i] = vec3(lightPosition[i] - (model*vec4(position, 1.0)).xyz);
		// LIGHT ATTENUATION
		float d = length(lightDir[i]);
		attenuation[i] = 1.0 / (lightAttenuation.x + lightAttenuation.y*d + lightAttenuation.z*d*d);
	}
	
    // Pass texture coordinates
	passTexcoord = texcoord;
	// Pass vertex colour
	passColour = colour;

    // Projected vertex position used for the interpolation
	vec4 pos = proj * view * model * vec4(position, 1.0);
	depth = pos.z;
    gl_Position = pos;
}
