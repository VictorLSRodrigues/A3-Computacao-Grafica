#version 330 core

in vec2 texCoord;
layout (location = 0) out float ambientOcclusion;

uniform sampler2D normalTexture;
uniform sampler2D depthTexture;
uniform sampler2D noiseTexture;
uniform mat4 projection;
uniform mat4 inverseProjection;
uniform vec2 noiseScale;
uniform float radius;
uniform float bias;
uniform float strength;

const int SAMPLE_COUNT = 16;
const vec3 kernel[SAMPLE_COUNT] = vec3[SAMPLE_COUNT](
    vec3( 0.035,  0.012, 0.095), vec3(-0.052,  0.041, 0.121),
    vec3( 0.071, -0.038, 0.158), vec3(-0.091, -0.024, 0.188),
    vec3( 0.018,  0.112, 0.226), vec3(-0.146,  0.078, 0.255),
    vec3( 0.163,  0.052, 0.302), vec3(-0.058, -0.174, 0.335),
    vec3( 0.225, -0.118, 0.382), vec3(-0.248, -0.092, 0.421),
    vec3( 0.103,  0.267, 0.466), vec3(-0.315,  0.172, 0.512),
    vec3( 0.354,  0.148, 0.563), vec3(-0.174, -0.369, 0.612),
    vec3( 0.428, -0.277, 0.681), vec3(-0.442, -0.311, 0.742)
);

vec3 reconstructViewPosition(vec2 uv, float depth)
{
    vec4 clip = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 view = inverseProjection * clip;
    return view.xyz / max(abs(view.w), 0.000001) * sign(view.w);
}

void main()
{
    float centerDepth = texture(depthTexture, texCoord).r;
    if (centerDepth >= 0.999999)
    {
        ambientOcclusion = 1.0;
        return;
    }

    vec3 normalSample = texture(normalTexture, texCoord).xyz;
    float normalLengthSquared = dot(normalSample, normalSample);
    if (normalLengthSquared < 0.5)
    {
        ambientOcclusion = 1.0;
        return;
    }
    vec3 normal = normalSample * inversesqrt(normalLengthSquared);

    vec3 fragmentPosition = reconstructViewPosition(texCoord, centerDepth);
    vec3 randomVector = normalize(texture(noiseTexture, texCoord * noiseScale).xyz);
    vec3 tangent = randomVector - normal * dot(randomVector, normal);
    if (dot(tangent, tangent) < 0.0001)
        tangent = cross(normal, vec3(0.0, 1.0, 0.0));
    if (dot(tangent, tangent) < 0.0001)
        tangent = cross(normal, vec3(1.0, 0.0, 0.0));
    tangent = normalize(tangent);
    vec3 bitangent = normalize(cross(normal, tangent));
    mat3 tbn = mat3(tangent, bitangent, normal);

    float occlusion = 0.0;
    for (int i = 0; i < SAMPLE_COUNT; ++i)
    {
        vec3 samplePosition = fragmentPosition +
            tbn * kernel[i] * radius;
        vec4 offset = projection * vec4(samplePosition, 1.0);
        if (offset.w <= 0.000001)
            continue;

        vec2 sampleUv = offset.xy / offset.w * 0.5 + 0.5;
        if (sampleUv.x < 0.0 || sampleUv.x > 1.0 ||
            sampleUv.y < 0.0 || sampleUv.y > 1.0)
            continue;

        float sampleDepthRaw = texture(depthTexture, sampleUv).r;
        if (sampleDepthRaw >= 0.999999)
            continue;

        float sampleDepth = reconstructViewPosition(sampleUv, sampleDepthRaw).z;
        float rangeWeight = smoothstep(
            0.0, 1.0,
            radius / max(abs(fragmentPosition.z - sampleDepth), 0.0001));
        occlusion += (sampleDepth >= samplePosition.z + bias ? 1.0 : 0.0) *
            rangeWeight;
    }

    ambientOcclusion = clamp(
        1.0 - (occlusion / float(SAMPLE_COUNT)) * strength,
        0.0, 1.0);
}
