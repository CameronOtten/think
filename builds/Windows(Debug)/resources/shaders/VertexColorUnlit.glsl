#version 430 core

layout(location = 0) out vec4 color;

in VertexData
{
	vec3 worldNormal;
	vec3 worldPosition;
	vec3 objectPosition;
	vec4 color;
	vec2 uv0;
	vec4 lightSpacePosition;
} vData;

uniform vec4 u_Color;

void main()
{
	color = u_Color * vData.color;	
	color.a = 1.0f;
};