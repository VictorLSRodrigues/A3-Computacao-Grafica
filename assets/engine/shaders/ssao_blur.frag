#version 330 core

in vec2 texCoord;
layout (location = 0) out float blurredOcclusion;

uniform sampler2D inputOcclusion;
uniform sampler2D depthTexture;
uniform sampler2D normalTexture;
uniform mat4 inverseProjection;
uniform vec2 texelSize;

vec3 reconstructViewPosition(vec2 uv, float depth)
{
    vec4 clip = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 view = inverseProjection * clip;
    return view.xyz / max(abs(view.w), 0.000001) * sign(view.w);
}

void main()
{
    float centerDepth = texture(depthTexture, texCoord).r;
    float centerAO = texture(inputOcclusion, texCoord).r;
    if (centerDepth >= 0.999999)
    {
        blurredOcclusion = centerAO;
        return;
    }

    vec3 centerNormalSample = texture(normalTexture, texCoord).xyz;
    float centerNormalLengthSquared = dot(centerNormalSample, centerNormalSample);
    if (centerNormalLengthSquared < 0.5)
    {
        blurredOcclusion = centerAO;
        return;
    }
    vec3 centerNormal = centerNormalSample * inversesqrt(centerNormalLengthSquared);
    vec3 centerPosition = reconstructViewPosition(texCoord, centerDepth);

    float total = centerAO * 4.0;
    float weightSum = 4.0;
    const vec2 offsets[8] = vec2[8](
        vec2(-1.0,  0.0), vec2(1.0,  0.0),
        vec2( 0.0, -1.0), vec2(0.0,  1.0),
        vec2(-1.0, -1.0), vec2(1.0, -1.0),
        vec2(-1.0,  1.0), vec2(1.0,  1.0)
    );
    const float spatialWeights[8] = float[8](
        2.0, 2.0, 2.0, 2.0, 1.0, 1.0, 1.0, 1.0
    );

    for (int i = 0; i < 8; ++i)
    {
        vec2 sampleUv = texCoord + offsets[i] * texelSize;
        if (sampleUv.x < 0.0 || sampleUv.x > 1.0 ||
            sampleUv.y < 0.0 || sampleUv.y > 1.0)
            continue;

        float sampleDepth = texture(depthTexture, sampleUv).r;
        if (sampleDepth >= 0.999999)
            continue;

        vec3 sampleNormalSample = texture(normalTexture, sampleUv).xyz;
        float sampleNormalLengthSquared = dot(sampleNormalSample, sampleNormalSample);
        if (sampleNormalLengthSquared < 0.5)
            continue;
        vec3 sampleNormal = sampleNormalSample * inversesqrt(sampleNormalLengthSquared);
        vec3 samplePosition = reconstructViewPosition(sampleUv, sampleDepth);
        float depthWeight = exp(-abs(centerPosition.z - samplePosition.z) * 18.0);
        float normalWeight = pow(max(dot(centerNormal, sampleNormal), 0.0), 12.0);
        float weight = spatialWeights[i] * depthWeight * normalWeight;
        total += texture(inputOcclusion, sampleUv).r * weight;
        weightSum += weight;
    }

    blurredOcclusion = clamp(total / max(weightSum, 0.0001), 0.0, 1.0);
}
