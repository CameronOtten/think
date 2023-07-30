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

const int MAX_POINT_LIGHTS = 8;
const float POINT_LIGHT_THRESHOLD = 0.001;
const int DIRECTIONAL = 0;
const int POINT = 1;

struct Light
{
	vec4 color;
	vec4 position;
	int lightType;
};

layout(std140, binding = 2)
uniform WorldDataBuffer
{
	vec4 AmbientLightTop;
	vec4 AmbientLightBottom;
	vec4 ViewPosition;
};

layout(std140, binding = 3)
uniform DirectionalLightBuffer
{
	Light DirectionalLight;
};

layout(std140, binding = 4)
uniform PointLightBuffer
{
	Light PointLights[MAX_POINT_LIGHTS];
};

layout(binding = 0) uniform sampler2DShadow ShadowDepthTestMap;
layout(binding = 1) uniform sampler2D ShadowMap;
uniform vec4 u_Color;

vec3 CalculateDiffuse()
{
	vec3 value;
	return value;
}

const float PI = 3.14159265359;

//Schlick fresnel
vec3 Fresnel(float cosTheta, vec3 zeroIncidence)
{
	return zeroIncidence + (1.0-zeroIncidence) * pow(clamp(1.0-cosTheta,0.0,1.0), 5.0);
}

//GGX
float DistributionGGX(float NdotH, float roughness)
{
	float a2 = roughness*roughness*roughness*roughness;
	float NdotH2 = NdotH*NdotH;

	float x = (NdotH2 * (a2-1.0) + 1.0);
	x = PI * x * x;

	return a2 / x;
}

float GeometryGGX(float cosTheta, float roughness)
{
	float r = roughness+1.0;
	float k = (r*r)/8.0;

	float x = cosTheta * (1.0-k) + k;
	return cosTheta / x;
}

//Smith
float Geometry(float NdotV, float NdotL, float roughness)
{
	return GeometryGGX(NdotV, roughness) * GeometryGGX(NdotL, roughness);
}

vec3 CalculateLight(float NdotV, vec3 lightDir, vec3 viewDir, vec3 normal, float attenuation, vec3 lightColor, float lightIntensity, vec3 albedo, float metallic, float roughness)
{
	vec3 halfway = normalize(lightDir+viewDir);
	vec3 radiance = lightColor * lightIntensity * attenuation;


	float NdotL = max(dot(normal, lightDir),0.0);
	float NdotH = max(dot(normal, halfway),0.0);

	vec3 zeroIncidence = mix(vec3(0.04),albedo, metallic);
	vec3 fresnelAmount = Fresnel(max(dot(halfway, viewDir),0.0), zeroIncidence);
	float NDF = DistributionGGX(NdotH, roughness);
	float geometryAmount = Geometry(NdotV, NdotL, roughness);

	vec3 numerator = NDF * geometryAmount * fresnelAmount;
	float denominator = 4.0 * NdotV * NdotL + 0.00001;
	vec3 specular = numerator/denominator;

	vec3 kD = vec3(1.0) - fresnelAmount;
	kD *= 1.0-metallic;

	return (kD*albedo / PI + specular) * radiance * NdotL;
}

float CalculateShadow(vec4 position)
{
	vec3 projCoords = position.xyz / position.w;
	projCoords = projCoords * 0.5 + 0.5;

	float bias = 0.005f;
	vec2 texelSize = 1.0f / textureSize(ShadowMap, 0);
	float shadow = 0;
	vec2 texel = vec2(-1.0);
	for (texel.x = -1.5; texel.x <= 1.5; texel.x+=1.0)
	for (texel.y = -1.5; texel.y <= 1.5; texel.y+=1.0)
	{
		//float sampledDepth = texture(ShadowMap, vec3(projCoords.xy + (texel*texelSize), projCoords.z)).r;
		//float sampledDepth = texture(ShadowMap, projCoords.xy + (texel*texelSize)).r;
		//shadow += projCoords.z > sampledDepth ? 0.0 : 1.0;
		shadow += texture(ShadowDepthTestMap, vec3(projCoords.xy + (texel*texelSize), projCoords.z)).r;//(projCoords.z-bias > sampledDepth ? 1.0 : 0.0);
	}

	float base = 0.05f;
	float amount = mix(0.0,1.0-base,shadow/16.0);
    return base + amount;
}

// PCSS Method from NVidia (https://developer.download.nvidia.com/whitepapers/2008/PCSS_Integration.pdf)
const vec2 POISSON_DISK[16] = {
  vec2( -0.94201624, -0.39906216 ),
  vec2( 0.94558609, -0.76890725 ),
  vec2( -0.094184101, -0.92938870 ),
  vec2( 0.34495938, 0.29387760 ),
  vec2( -0.91588581, 0.45771432 ),
  vec2( -0.81544232, -0.87912464 ),
  vec2( -0.38277543, 0.27676845 ),
  vec2( 0.97484398, 0.75648379 ),
  vec2( 0.44323325, -0.97511554 ),
  vec2( 0.53742981, -0.47373420 ),
  vec2( -0.26496911, -0.41893023 ),
  vec2( 0.79197514, 0.19090188 ),
  vec2( -0.24188840, 0.99706507 ),
  vec2( -0.81409955, 0.91437590 ),
  vec2( 0.19984126, 0.78641367 ),
  vec2( 0.14383161, -0.14100790 )
};
const int BLOCKER_SEARCH_NUM_SAMPLES = 16;
const int PCF_NUM_SAMPLES = 16;
const float NEAR_PLANE = 0.1;
const float LIGHT_WORLD_SIZE = 0.9;
const float LIGHT_FRUSTUM_WIDTH = 10.0;
const float LIGHT_SIZE_UV = (LIGHT_WORLD_SIZE / LIGHT_FRUSTUM_WIDTH);
const float BIAS = 0.0001;

float SearchWidth(float uvLightSize, float receiverDistance)
{
	float z = ViewPosition.z == 0.0 ? 0.00001 : ViewPosition.z;
	return uvLightSize * (receiverDistance - NEAR_PLANE) / 1.0;
}

float FindBlockerDistance(vec3 shadowCoords, float uvLightSize)
{
    int blockers = 0;
	float avgBlockerDistance = 0;
	float searchWidth = SearchWidth(uvLightSize, shadowCoords.z);
	for (int i = 0; i < BLOCKER_SEARCH_NUM_SAMPLES; i++)
	{
		//float z = texture(ShadowMap, shadowCoords.xy + POISSON_DISK[i] * searchWidth).r;
		float z = texture(ShadowMap, shadowCoords.xy + POISSON_DISK[i] * searchWidth).r;
		if (z < (shadowCoords.z - BIAS))
		{
			blockers++;
			avgBlockerDistance += z;
		}
	}
	if (blockers > 0)
		return avgBlockerDistance / blockers;
	else return -1;
}

float PCF(vec3 shadowCoords, float uvRadius)
{
	float bias = 0.005f;
	float sum = 0;
	for (int i = 0; i < PCF_NUM_SAMPLES; i++)
	{
		//float z = texture(ShadowMap, shadowCoords.xy + POISSON_DISK[i] * uvRadius).r;
		float z = texture(ShadowDepthTestMap, vec3(shadowCoords.xy + POISSON_DISK[i] * uvRadius, shadowCoords.z-bias)).r;
		//sum += (z < (shadowCoords.z - BIAS)) ? 1 : 0;
		sum += z;
	}

	//return sum / PCF_NUM_SAMPLES;

	float base = 0.05f;
	float amount = mix(0.0,1.0-base,sum/PCF_NUM_SAMPLES);
    return base + amount;
}

float PCSS (vec4 position)
{
	vec3 shadowCoords = position.xyz / position.w;
	shadowCoords = shadowCoords * 0.5 + 0.5;

	// blocker search
	float blockerDistance = FindBlockerDistance(shadowCoords, LIGHT_SIZE_UV);
	if (blockerDistance == -1)
		return 1;

	// penumbra estimation
	float penumbraWidth = (shadowCoords.z - blockerDistance) / blockerDistance;

	// percentage-close filtering
	float uvRadius = penumbraWidth * LIGHT_SIZE_UV * NEAR_PLANE / shadowCoords.z;
	return PCF(shadowCoords, uvRadius);

}

void main()
{
	vec3 normal = normalize(vData.worldNormal.xyz);
	vec3 viewDir = normalize(ViewPosition.xyz-vData.worldPosition);

	vec3 albedo = vData.color.rgb * u_Color.rgb;
	float roughness = 0.75f;
	float metallic = 0.0f;
	float ao = 1.0;

	vec3 finalLight = vec3(0.0);
	float NdotV = max(dot(normal, viewDir),0.0);

	Light light = DirectionalLight;
	vec3 lightDir = normalize(light.position.xyz);
	float atten = 1.0;
	//float shadowAtten = CalculateShadow(vData.lightSpacePosition);
	float shadowAtten = PCSS(vData.lightSpacePosition);
	//float shadowAtten = CalcShadowFactor(vData.lightSpacePosition);
	//float shadowAtten = CalculateShadowFourSamples(vData.lightSpacePosition);
	if (light.color.a > 0.0)
		finalLight += CalculateLight(NdotV, lightDir, viewDir, normal, atten, light.color.rgb, light.color.a, albedo, metallic, roughness) * shadowAtten;

	for (int i = 0; i < MAX_POINT_LIGHTS; i++)
	{
		light = PointLights[i];
		if (light.color.a <= 0) continue;

		lightDir = light.position.xyz-vData.worldPosition;
		float d2 = (lightDir.x*lightDir.x) + (lightDir.y*lightDir.y) + (lightDir.z*lightDir.z);
		atten = 1.0 / d2;

		if (atten < POINT_LIGHT_THRESHOLD) continue;

		lightDir = normalize(lightDir);

		finalLight += CalculateLight(NdotV, lightDir, viewDir, normal, atten, light.color.rgb, light.color.a, albedo, metallic, roughness);
	}

	vec4 ambient = mix(AmbientLightBottom, AmbientLightTop, max(0.0,dot(normal,vec3(0,1,0))));
	color.rgb = (ambient.rgb*albedo*ao)+finalLight;
	color.a = 1.0f;

};
