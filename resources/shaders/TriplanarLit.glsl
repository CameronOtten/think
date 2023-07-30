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


layout(binding = 0) uniform sampler2D AlbedoTex;

void main()
{
	color = texture2D(AlbedoTex,  vData.uv0);
};