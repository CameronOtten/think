#version 430 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord0;
layout(location = 3) in vec4 color;

uniform mat4 LightVP;

layout(std140, binding = 1)
uniform ObjectBuffer
{
	mat4 ModelMatrix;
	mat4 InverseModelMatrix;
};

void main()
{
	gl_Position = LightVP * ModelMatrix * vec4(position,1.0);
};