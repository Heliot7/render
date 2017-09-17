#version 150

in vec3 position;
in vec4 colour;

uniform mat4 model;

out vec4 passColour;

void main()
{
	passColour = colour;
    gl_Position = model * vec4(position, 1.0);
}
