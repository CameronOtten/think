#version 430 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord0;
layout(location = 3) in vec4 color;

out VertexData
{
	vec3 worldNormal;
	vec3 worldPosition;
	vec3 objectPosition;
	vec4 color;
	vec2 uv0;
	vec4 lightSpacePosition;
} vData;

void main()
{
	gl_Position = vec4(position,1.0);
	vData.worldNormal = normal;
	vData.worldPosition = position;
	vData.objectPosition = position;
	vData.color = color;
	vData.uv0 = texCoord0;
	vData.lightSpacePosition = vec4(0.0);
};