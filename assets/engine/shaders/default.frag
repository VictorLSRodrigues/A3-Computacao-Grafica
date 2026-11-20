
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

const float PI = 3.14159265359;

// Intensidade das reflexões ambientais.
const float ENVIRONMENT_STRENGTH = 1.0;

// Distribuição de microfacetas GGX.
float distributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;

    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denominator =
        NdotH2 * (a2 - 1.0) + 1.0;

    return a2 /
        max(PI * denominator * denominator, 0.000001);
}

// Geometria Schlick-GGX.
float geometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;

    return NdotV /
        max(NdotV * (1.0 - k) + k, 0.000001);
}

// Geometria Smith.
float geometrySmith(
    vec3 N,
    vec3 V,
    vec3 L,
    float roughness)
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

// Iluminação PBR.
vec3 calculatePBR(
    vec3 N,
    vec3 V,
    vec3 L,
    vec3 radiance,
    vec3 albedo,
    float metallic,
    float roughness)
{
    vec3 H = normalize(V + L);

    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);

    if (NdotL <= 0.0 || NdotV <= 0.0)
        return vec3(0.0);

    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    vec3 F = fresnelSchlick(
        max(dot(H, V), 0.0),
        F0
    );

    float D = distributionGGX(N, H, roughness);
    float G = geometrySmith(N, V, L, roughness);

    vec3 numerator = D * G * F;

    float denominator =
        4.0 * NdotV * NdotL + 0.0001;

    vec3 specular = numerator / denominator;

    // Conservação de energia.
    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

    vec3 diffuse = kD * albedo / PI;

    return (diffuse + specular) * radiance * NdotL;
}

// Calcula a normal utilizando o Normal Map.
vec3 calculateNormal()
{
    vec3 N = normalize(Normal);

    // Sem mapa normal, utiliza a normal original.
    if (hasNormalMap == 0)
        return N;

    // Reconstrói uma base ortonormal.
    vec3 T = normalize(Tangent - dot(Tangent, N) * N);

    vec3 B = normalize(cross(N, T));

    // Preserva a orientação da bitangente.
    if (dot(B, Bitangent) < 0.0)
        B = -B;

    mat3 TBN = mat3(T, B, N);

    // Converte os valores da textura de [0,1] para [-1,1].
    vec3 tangentNormal = texture(normalMap, texCoord).rgb;
    tangentNormal = normalize(tangentNormal * 2.0 - 1.0);

    // Transforma a normal para o espaço do mundo.
    return normalize(TBN * tangentNormal);
}

// Calcula a reflexão do ambiente.
vec3 calculateEnvironmentReflection(
    vec3 N,
    vec3 V,
    vec3 albedo,
    float metallic)
{
    if (hasEnvironmentMap == 0 || metallic <= 0.0)
        return vec3(0.0);

    // Direção da reflexão.
    vec3 R = reflect(-V, N);

    // Cor refletida pelo ambiente.
    vec3 environmentColor = texture(environmentMap, R).rgb;

    // Fresnel do material metálico.
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    vec3 F = fresnelSchlick(
        max(dot(N, V), 0.0),
        F0
    );

    return environmentColor * F * metallic * ENVIRONMENT_STRENGTH;
}

void main()
{
    // Cor-base e transparência.
    vec4 albedoSample = texture(tex0, texCoord) * baseColor;
    vec3 albedo = max(albedoSample.rgb, vec3(0.0));

    // Fatores do material.
    float metallic = clamp(metallicFactor, 0.0, 1.0);
    float roughness = clamp(roughnessFactor, 0.04, 1.0);

    // No glTF: G = roughness e B = metallic.
    if (hasMetallicRoughnessMap == 1)
    {
        vec4 mr = texture(metallicRoughnessMap, texCoord);

        roughness *= mr.g;
        metallic *= mr.b;

        roughness = clamp(roughness, 0.04, 1.0);
        metallic = clamp(metallic, 0.0, 1.0);
    }

    // Normal final, com suporte a Normal Map.
    vec3 N = calculateNormal();

    vec3 V = normalize(camPos - crntPos);

    vec3 L;
    vec3 radiance = lightColor.rgb;
    float attenuation = 1.0;

    // Luz pontual.
    if (lightType == 0)
    {
        vec3 lightVector = lightPos - crntPos;
        float distanceToLight = length(lightVector);

        L = normalize(lightVector);

        float a = 3.0;
        float b = 0.7;

        attenuation = 1.0 /
            (a * distanceToLight * distanceToLight +
             b * distanceToLight + 1.0);
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
        float distanceToLight = length(lightVector);

        L = normalize(lightVector);

        float outerCone = 0.90;
        float innerCone = 0.95;

        float angle = dot(normalize(lightDir), -L);

        float coneIntensity = clamp(
            (angle - outerCone) /
            (innerCone - outerCone),
            0.0,
            1.0
        );

        float a = 3.0;
        float b = 0.7;

        attenuation =
            coneIntensity /
            (a * distanceToLight * distanceToLight +
             b * distanceToLight + 1.0);
    }

    // Iluminação direta PBR.
    vec3 directLighting = calculatePBR(
        N,
        V,
        L,
        radiance * attenuation,
        albedo,
        metallic,
        roughness
    );

    // Ambiente difuso simplificado.
    vec3 ambient = 0.12 * albedo;

    // Reflexões ambientais metálicas.
    vec3 environmentReflection = calculateEnvironmentReflection(
        N,
        V,
        albedo,
        metallic
    );

    // Resultado final.
    vec3 finalColor =
        ambient +
        directLighting +
        environmentReflection;

    FragColor = vec4(finalColor, albedoSample.a);
}
