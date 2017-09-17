#version 150

in float depth;

out vec4 outColour;

void main()
{
	// Right now depth is not used 
	outColour = vec4(1.0, 1.0, 1.0, 1.0);
}
