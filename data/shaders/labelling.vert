#version 150

in vec3 position;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

out float depth;

void main()
{
	vec4 pos = proj * view * model * vec4(position, 1.0);
	depth = pos.z;
    gl_Position = pos;
}
