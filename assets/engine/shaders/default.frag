
#version 330 core

out vec4 FragColor;

// Dados interpolados do vertex shader.
in vec3 color;
in vec2 texCoord;
in vec3 Normal;
in vec3 crntPos;
in vec4 FragPosLightSpace;

// Base TBN para Normal Mapping.
in vec3 Tangent;
in vec3 Bitangent;

// Texturas.
uniform sampler2D tex0;
uniform sampler2D specular0;
uniform sampler2D metallicRoughnessMap;
uniform sampler2D normalMap;

// Cubemap do ambiente.
uniform samplerCube environmentMap;
uniform sampler2D shadowMap;
uniform int shadowsEnabled;
uniform sampler2D ambientOcclusionMap;
uniform int ambientOcclusionEnabled;
uniform vec2 screenSize;
uniform float ambientOcclusionStrength;

// Controle dos mapas.
uniform int hasMetallicRoughnessMap;
uniform int hasNormalMap;
uniform int hasEnvironmentMap;

// Material.
uniform vec4 baseColor;
uniform float metallicFactor;
uniform float roughnessFactor;

// Iluminação.
uniform int lightType;
uniform vec4 lightColor;
uniform vec3 lightPos;
uniform vec3 lightDir;
uniform vec3 camPos;
uniform float lightIntensity;

const float PI = 3.14159265359;

vec3 srgbToLinear(vec3 color)
{
    vec3 low = color / 12.92;
    vec3 high = pow((color + 0.055) / 1.055, vec3(2.4));
    return mix(high, low, lessThanEqual(color, vec3(0.04045)));
}

vec3 linearToSrgb(vec3 color)
{
    color = max(color, vec3(0.0));
    vec3 low = color * 12.92;
    vec3 high = 1.055 * pow(color, vec3(1.0 / 2.4)) - 0.055;
    return mix(high, low, lessThanEqual(color, vec3(0.0031308)));
}
// ACES fitted tone mapper: compresses highlights while preserving midtone contrast.
vec3 acesFitted(vec3 color)
{
    color = max(color, vec3(0.0));
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((color * (a * color + b)) /
        (color * (c * color + d) + e), 0.0, 1.0);
}


uniform float environmentReflectionStrength;
uniform float ambientStrength;
uniform bool uFramebufferSRGB;
uniform int studioLightingEnabled;
uniform int environmentDiffuseEnabled;
uniform int toneMappingEnabled;

// Distribuição de microfacetas GGX.
float distributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;

    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denominator = NdotH2 * (a2 - 1.0) + 1.0;

    return a2 / max(PI * denominator * denominator, 0.000001);
}

// Geometria Schlick-GGX.
float geometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;

    return NdotV / max(NdotV * (1.0 - k) + k, 0.000001);
}

// Geometria Smith.
float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);

    float ggxV = geometrySchlickGGX(NdotV, roughness);
    float ggxL = geometrySchlickGGX(NdotL, roughness);

    return ggxV * ggxL;
}

// Fresnel Schlick.
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) *
        pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Iluminação PBR Cook-Torrance.
vec3 calculatePBR(
    vec3 N,
    vec3 V,
    vec3 L,
    vec3 radiance,
    vec3 albedo,
    float metallic,
    float roughness)
{
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);

    if (NdotL <= 0.0 || NdotV <= 0.0)
        return vec3(0.0);

    vec3 H = normalize(V + L);

    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    vec3 F = fresnelSchlick(
        max(dot(H, V), 0.0),
        F0
    );

    float D = distributionGGX(N, H, roughness);
    float G = geometrySmith(N, V, L, roughness);

    vec3 numerator = D * G * F;

    float denominator = max(
        4.0 * NdotV * NdotL,
        0.0001
    );

    vec3 specular = numerator / denominator;

    // Conservação de energia.
    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

    vec3 diffuse = kD * albedo / PI;

    return (diffuse + specular) * radiance * NdotL;
}

// Normal Mapping.
vec3 calculateNormal()
{
    vec3 N = normalize(Normal);

    if (hasNormalMap == 0)
        return N;

    vec3 T = normalize(Tangent - dot(Tangent, N) * N);
    vec3 B = normalize(cross(N, T));

    if (dot(B, Bitangent) < 0.0)
        B = -B;

    mat3 TBN = mat3(T, B, N);

    vec3 tangentNormal = texture(normalMap, texCoord).rgb;
    tangentNormal = normalize(tangentNormal * 2.0 - 1.0);

    return normalize(TBN * tangentNormal);
}

// Approximate diffuse irradiance from the existing LDR sky cubemap.
// A small hemisphere sample set avoids a costly per-pixel convolution pass.
vec3 calculateEnvironmentDiffuse(vec3 N)
{
    if (hasEnvironmentMap == 0)
        return vec3(0.85);

    vec3 up = abs(N.y) < 0.98 ? vec3(0.0, 1.0, 0.0) : vec3(0.0, 0.0, 1.0);
    vec3 T = normalize(cross(up, N));
    vec3 B = normalize(cross(N, T));
    vec3 irradiance = texture(environmentMap, N).rgb * 0.40;
    irradiance += texture(environmentMap, normalize(N + T * 0.65)).rgb * 0.15;
    irradiance += texture(environmentMap, normalize(N - T * 0.65)).rgb * 0.15;
    irradiance += texture(environmentMap, normalize(N + B * 0.65)).rgb * 0.15;
    irradiance += texture(environmentMap, normalize(N - B * 0.65)).rgb * 0.15;
    return irradiance;
}

// Studio fill/rim lights are deliberately unshadowed and supplement the key light.
vec3 calculateStudioLighting(vec3 N, vec3 V, vec3 albedo, float metallic, float roughness)
{
    if (studioLightingEnabled == 0)
        return vec3(0.0);

    vec3 fillL = normalize(vec3(0.58, 0.42, 0.70));
    vec3 rimL = normalize(vec3(-0.48, 0.32, -0.82));
    vec3 fillRadiance = vec3(0.38, 0.55, 0.78);
    vec3 rimRadiance = vec3(0.72, 0.36, 0.16);
    vec3 fill = calculatePBR(N, V, fillL, fillRadiance, albedo, metallic, roughness);
    vec3 rim = calculatePBR(N, V, rimL, rimRadiance, albedo, metallic, roughness);
    return fill + rim;
}

// Reflexão ambiental independente da incidência da luz direta.
vec3 calculateEnvironmentReflection(
    vec3 N,
    vec3 V,
    vec3 albedo,
    float metallic,
    float roughness)
{
    if (hasEnvironmentMap == 0 || metallic <= 0.0)
    {
        return vec3(0.0);
    }

    vec3 R = reflect(-V, N);

    // Mipmaps mais altos representam versões progressivamente mais borradas do ambiente.
    float maxLod = floor(log2(float(textureSize(environmentMap, 0).x)));
    float lod = clamp(roughness, 0.0, 1.0) * maxLod;

    vec3 environmentColor = textureLod(environmentMap, R, lod).rgb;

    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    vec3 F = fresnelSchlick(
        max(dot(N, V), 0.0),
        F0
    );

    return environmentColor *
           F *
           environmentReflectionStrength;
}

float calculateDirectionalShadow(vec3 normal, vec3 lightDirection)
{
    if (shadowsEnabled == 0)
        return 0.0;

    vec3 projected = FragPosLightSpace.xyz / max(FragPosLightSpace.w, 0.00001);
    projected = projected * 0.5 + 0.5;

    // Fora do volume ortográfico da luz não há sombra projetada por este mapa.
    if (projected.z > 1.0 || projected.z < 0.0 ||
        projected.x < 0.0 || projected.x > 1.0 ||
        projected.y < 0.0 || projected.y > 1.0)
        return 0.0;

    float currentDepth = projected.z;
    float bias = max(0.00055 * (1.0 - max(dot(normal, lightDirection), 0.0)), 0.00012);
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));
    float shadow = 0.0;

    // PCF 3x3: reduz serrilhado e suaviza a transição entre regiões iluminadas.
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float closestDepth = texture(
                shadowMap,
                projected.xy + vec2(x, y) * texelSize).r;
            shadow += (currentDepth - bias > closestDepth) ? 1.0 : 0.0;
        }
    }

    return shadow / 9.0;
}

void main()
{
    // Cor-base.
    vec4 albedoSample = texture(tex0, texCoord) * baseColor;
    vec3 albedo = max(albedoSample.rgb, vec3(0.0));

    // Propriedades do material.
    float metallic = clamp(metallicFactor, 0.0, 1.0);
    float roughness = clamp(roughnessFactor, 0.04, 1.0);

    // Metallic-Roughness Map: G = Roughness, B = Metallic.
    if (hasMetallicRoughnessMap == 1)
    {
        vec4 mr = texture(metallicRoughnessMap, texCoord);

        roughness *= mr.g;
        metallic *= mr.b;

        roughness = clamp(roughness, 0.04, 1.0);
        metallic = clamp(metallic, 0.0, 1.0);
    }

    // Normais e direção da câmera.
    vec3 N = calculateNormal();
    vec3 V = normalize(camPos - crntPos);

    vec3 L = vec3(0.0);
    vec3 radiance = srgbToLinear(lightColor.rgb) *
                    max(lightIntensity, 0.0);

    float attenuation = 1.0;

    // Luz pontual.
    if (lightType == 0)
    {
        vec3 lightVector = lightPos - crntPos;
        float distanceToLight = max(length(lightVector), 0.0001);

        L = lightVector / distanceToLight;

        float a = 3.0;
        float b = 0.7;

        attenuation = 1.0 / (
            a * distanceToLight * distanceToLight +
            b * distanceToLight + 1.0
        );
    }
    // Luz direcional.
    else if (lightType == 1)
    {
        L = normalize(-lightDir);
    }
    // Holofote.
    else
    {
        vec3 lightVector = lightPos - crntPos;
        float distanceToLight = max(length(lightVector), 0.0001);

        L = lightVector / distanceToLight;

        float outerCone = 0.90;
        float innerCone = 0.95;

        float angle = dot(normalize(lightDir), -L);

        float coneIntensity = clamp(
            (angle - outerCone) / (innerCone - outerCone),
            0.0,
            1.0
        );

        float a = 3.0;
        float b = 0.7;

        attenuation = coneIntensity / (
            a * distanceToLight * distanceToLight +
            b * distanceToLight + 1.0
        );
    }

    // Iluminação direta Cook-Torrance.
    vec3 directLighting = calculatePBR(
        N,
        V,
        L,
        radiance * attenuation,
        albedo,
        metallic,
        roughness
    );

    if (lightType == 1 && shadowsEnabled == 1)
    {
        float shadow = calculateDirectionalShadow(N, L);
        directLighting *= (1.0 - shadow);
    }

    // Fresnel para conservação de energia no ambiente.
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    vec3 ambientFresnel = fresnelSchlick(
        max(dot(N, V), 0.0),
        F0
    );

    // Ambiente difuso: metais não recebem difusão artificial.
    vec3 kD = (vec3(1.0) - ambientFresnel) * (1.0 - metallic);

    float ambientOcclusion = 1.0;
    if (ambientOcclusionEnabled != 0)
    {
        vec2 aoUv = gl_FragCoord.xy / max(screenSize, vec2(1.0));
        ambientOcclusion = clamp(texture(ambientOcclusionMap, aoUv).r, 0.0, 1.0);
        ambientOcclusion = clamp(
            mix(1.0, ambientOcclusion, max(ambientOcclusionStrength, 0.0)),
            0.0, 1.0);
    }

    vec3 environmentDiffuse = environmentDiffuseEnabled != 0
        ? calculateEnvironmentDiffuse(N)
        : vec3(1.0);
    vec3 ambient = ambientStrength * kD * albedo *
                   environmentDiffuse * ambientOcclusion;

    // Reflexão ambiental: não depende da iluminação direta.
    vec3 environmentReflection = calculateEnvironmentReflection(
        N,
        V,
        albedo,
        metallic,
        roughness
    );

    // Composição final.
    // A oclusao ambiente tambem atenua levemente reflexoes indiretas,
    // sem apagar a resposta especular dos materiais metalicos.
    environmentReflection *= mix(1.0, ambientOcclusion, 0.25);

    vec3 studioLighting = calculateStudioLighting(
        N, V, albedo, metallic, roughness);

    vec3 finalColor =
        ambient +
        directLighting +
        environmentReflection +
        studioLighting;

    if (toneMappingEnabled != 0)
        finalColor = acesFitted(finalColor);

    vec3 outputColor = uFramebufferSRGB ? finalColor : linearToSrgb(finalColor);
    FragColor = vec4(outputColor, albedoSample.a);
}
