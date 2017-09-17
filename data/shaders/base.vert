#version 150

in vec2 position;
in vec3 colour;
in vec2 texcoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 ortho;

out vec4 passColour;
out vec2 passTexcoord;

void main()
{
	passColour = vec4(colour, 1.0);
	passTexcoord = texcoord;
    gl_Position = ortho * view * model * vec4(position, 0.0, 1.0);
}
