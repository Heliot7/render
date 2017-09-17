#version 150

in vec3 position;
in vec4 colour;
in vec2 texcoord;
in vec3 normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

out vec4 passColour;
out vec2 passTexcoord;
out float depth;

void main()
{
	passColour = colour;
	passTexcoord = texcoord;
	
	vec4 pos = proj * view * model * vec4(position, 1.0);
	depth = pos.z;
    gl_Position = pos;
}
