#version 330

in float depth;

uniform vec3 labelColour;
uniform float labelAlpha;

layout (location = 0) out vec4 outColour;
layout (location = 1) out vec4 outDepth;

void main()
{
	outColour = vec4(labelColour, 1.0);
	outDepth = vec4(depth/10.0, depth/10.0, depth/10.0, 1.0);
}
