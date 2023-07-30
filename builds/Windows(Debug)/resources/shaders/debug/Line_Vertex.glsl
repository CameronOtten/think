#version 430 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;

layout(std140, binding = 0)
uniform CameraBuffer
{
	mat4 CameraVP;
	mat4 MainLightVP;
};

out vec3 vertexColor;

void main()
{
	gl_Position = CameraVP * vec4(position,1.0);
    vertexColor = color;
};