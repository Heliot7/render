#version 150

in vec3 position;
in vec4 colour;
in vec2 texcoord;
in vec3 normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;
uniform vec3 lightPosition;
uniform vec3 lightAttenuation;

out vec4 passColour;
out vec2 passTexcoord;
out vec4 eyeVertexPosition;
out vec3 eyeNormal;
out vec3 lightDir;
out vec3 cameraVector;
out float attenuation;
out float depth;

void main()
{
    // Eye space normal:
    eyeNormal = (vec4(normal, 1.0)).xyz;
    // eyeNormal = normal;
    // Eye space vertex position:
    eyeVertexPosition = view * model * vec4(position, 1.0);

    // Calculate vertex light directions
	lightDir = vec3(lightPosition - (model*vec4(position, 1.0)).xyz);
    // Calculate Camera vector with pixel
    cameraVector = -eyeVertexPosition.xyz;

    // Attenuation
    float d = length(lightDir);
    attenuation = 1.0 / (lightAttenuation.x + lightAttenuation.y*d + lightAttenuation.z*d*d);

    // Pass texture coordinates
	passTexcoord = texcoord;
	
	// Pass vertex colour
	passColour = colour;

    // Projected vertex position used for the interpolation
	vec4 pos = proj * view * model * vec4(position, 1.0);
	depth = pos.z;
    gl_Position = pos;
}
