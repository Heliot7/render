#version 150

in vec3 position;
in vec4 colour;
in vec2 texcoord;
in vec3 normal;

uniform mat4 model;
uniform mat4 proj;

out vec4 passColour;
out vec2 passTexcoord;

void main()
{
	passColour = colour;
	passTexcoord = texcoord;
    gl_Position = proj * model * vec4(position, 1.0);
}