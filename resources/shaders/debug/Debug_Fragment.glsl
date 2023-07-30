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

layout(binding = 1) uniform sampler2D DisplayTexture;

void main()
{
	color = vec4(texture2D(DisplayTexture, vData.uv0).r);
	color.a = 1.0;
};