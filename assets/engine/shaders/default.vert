
#version 330 core

// Atributos do vértice.
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aTex;
layout (location = 3) in vec3 aNormal;
layout (location = 4) in vec4 aTangent;

// Dados enviados ao Fragment Shader.
out vec3 color;
out vec2 texCoord;
out vec3 Normal;
out vec3 crntPos;

// Base para o cálculo do Normal Map.
out vec3 Tangent;
out vec3 Bitangent;

// Matrizes.
uniform mat4 camMatrix;
uniform mat4 model;

void main()
{
    // Posição no espaço do mundo.
    vec4 worldPosition = model * vec4(aPos, 1.0);
    crntPos = worldPosition.xyz;

    // Projeção.
    gl_Position = camMatrix * worldPosition;

    // Matriz normal para corrigir escalas e rotações.
    mat3 normalMatrix = mat3(transpose(inverse(model)));

    vec3 N = normalize(normalMatrix * aNormal);
    vec3 T = normalize(mat3(model) * aTangent.xyz);

    // Remove possíveis componentes não ortogonais.
    T = normalize(T - dot(T, N) * N);

    // Calcula a bitangente.
    vec3 B = normalize(cross(N, T)) * aTangent.w;

    Normal = N;
    Tangent = T;
    Bitangent = B;

    color = aColor;
    texCoord = aTex;
}
