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

layout(std140, binding = 0)
uniform CameraBuffer
{
	mat4 CameraVP;
	mat4 MainLightVP;
};

layout(std140, binding = 1)
uniform ObjectBuffer
{
	mat4 ModelMatrix;
	mat4 InverseModelMatrix;
};

void main()
{
	vec4 worldPosition = ModelMatrix * vec4(position,1.0f);

	gl_Position = CameraVP * worldPosition;

	vData.objectPosition = position;
	vData.worldPosition = worldPosition.xyz;
	vData.uv0 = texCoord0;
	vData.worldNormal = (InverseModelMatrix * vec4(normal,0.0f)).xyz;
	vData.color = color;
	vData.lightSpacePosition = MainLightVP * worldPosition;
};

