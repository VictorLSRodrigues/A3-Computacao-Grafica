
#version 330 core

out vec4 FragColor;

// Dados interpolados do vertex shader.
in vec3 color;
in vec2 texCoord;
in vec3 Normal;
in vec3 crntPos;

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
const float ENVIRONMENT_STRENGTH = 1.0;
const float AMBIENT_STRENGTH = 0.12;

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

// Reflexão ambiental controlada pela incidência da luz.
vec3 calculateEnvironmentReflection(
    vec3 N,
    vec3 V,
    vec3 albedo,
    float metallic,
    float roughness,
    float lightInfluence)
{
    if (hasEnvironmentMap == 0 ||
        metallic <= 0.0 ||
        lightInfluence <= 0.0)
    {
        return vec3(0.0);
    }

    vec3 R = reflect(-V, N);

    vec3 environmentColor = texture(environmentMap, R).rgb;

    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    vec3 F = fresnelSchlick(
        max(dot(N, V), 0.0),
        F0
    );

    return environmentColor *
           F *
           ENVIRONMENT_STRENGTH *
           lightInfluence;
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
    vec3 radiance = lightColor.rgb *
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

    // Incidência efetiva da fonte sobre a superfície.
    float lightInfluence =
        max(dot(N, L), 0.0) *
        attenuation *
        max(lightIntensity, 0.0);

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

    // Fresnel para conservação de energia no ambiente.
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    vec3 ambientFresnel = fresnelSchlick(
        max(dot(N, V), 0.0),
        F0
    );

    // Ambiente difuso: metais não recebem difusão artificial.
    vec3 kD = (vec3(1.0) - ambientFresnel) * (1.0 - metallic);

    vec3 ambient = AMBIENT_STRENGTH * kD * albedo;

    // Reflexão ambiental dependente da incidência da luz.
    vec3 environmentReflection = calculateEnvironmentReflection(
        N,
        V,
        albedo,
        metallic,
        roughness,
        lightInfluence
    );

    // Composição final.
    vec3 finalColor =
        ambient +
        directLighting +
        environmentReflection;

    FragColor = vec4(finalColor, albedoSample.a);
}
