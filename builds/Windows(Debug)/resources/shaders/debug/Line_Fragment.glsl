#version 430 core

layout(location = 0) out vec4 color;

in vec3 vertexColor;

void main()
{
	color.rgb = vertexColor;
	color.a = 1.0;
};