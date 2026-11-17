
#version 330 core

layout (location = 0) in vec2 aPos;

uniform vec2 uPos;
uniform vec2 uTamanho;
uniform vec2 uTela;
uniform float uAngle;

void main()
{
    // Calcula a posição do vértice dentro do retângulo.
    vec2 local = aPos * uTamanho;

    // Move a origem para o centro do retângulo.
    vec2 center = uTamanho * 0.5;
    local -= center;

    // Rotaciona o retângulo em torno do seu centro.
    float c = cos(uAngle);
    float s = sin(uAngle);

    vec2 rotated;

    rotated.x = local.x * c - local.y * s;
    rotated.y = local.x * s + local.y * c;

    // Reposiciona o retângulo na tela.
    vec2 pixel = uPos + center + rotated;

    // Converte pixels para coordenadas NDC.
    vec2 ndc = vec2(
        pixel.x / uTela.x * 2.0 - 1.0,
        1.0 - pixel.y / uTela.y * 2.0
    );

    gl_Position = vec4(ndc, 0.0, 1.0);
}
