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


layout(binding = 2) uniform sampler2D GlyphAtlas;

float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

const float SCREEN_PX_MULT = 500.0/32.0*2.0;

void main()
{
	vec3 msd = texture(GlyphAtlas, vData.uv0).rgb;
    float sd = median(msd.r, msd.g, msd.b);
    float screenPxDistance = SCREEN_PX_MULT*(sd - 0.5);
    float opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
    color = vData.color;
	color.a *= opacity;
};
