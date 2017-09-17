#version 330

in vec4 passColour;
in vec2 passTexcoord;
in float depth;

// out vec4 outColour;
layout (location = 0) out vec4 outColour;
layout (location = 1) out vec4 outDepth;

uniform sampler2D tex;

void main()
{
	vec4 texColour = texture(tex, passTexcoord);
	vec3 mixColour = (passColour.rgb + texColour.a*texColour.rgb) / (1.0 + texColour.a);
	outColour = vec4(mixColour, 1.0);
	outDepth = vec4(1.0 - min(1.0, depth/10.0), 1.0 - min(1.0, depth/10.0), 1.0 - min(1.0, depth/10.0), 0.0);
}
