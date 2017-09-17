#version 150

in vec4 passColour;
in vec2 passTexcoord;

out vec4 outColour;

uniform sampler2D tex;

void main()
{
	vec4 texColour = texture(tex, passTexcoord);
	vec3 mixColour = (passColour.rgb + texColour.a*texColour.rgb) / (1.0 + texColour.a);
	outColour = texColour;
}
