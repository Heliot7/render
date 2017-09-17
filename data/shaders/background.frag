#version 150

in vec4 passColour;
in vec2 passTexcoord;

out vec4 outColour;

uniform sampler2D tex;

void main()
{
	vec4 texColour = texture(tex, passTexcoord);
	outColour = texColour;
}
