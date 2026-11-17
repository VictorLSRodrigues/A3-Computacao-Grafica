#version 330 core
layout (location = 0) in vec2 aPos; // retângulo unitário (0 a 1)

uniform vec2 uScreen;  // tamanho da janela, em pixels
uniform vec2 uPos;     // canto superior esquerdo da letra, em pixels
uniform vec2 uSize;    // largura e altura da letra, em pixels
uniform vec2 uUVPos;   // canto superior esquerdo da letra no atlas
uniform vec2 uUVSize;  // tamanho da letra no atlas

out vec2 vUV;

void main()
{
    vec2 pixel = uPos + aPos * uSize;
    vec2 ndc = vec2(pixel.x / uScreen.x * 2.0 - 1.0,
                    1.0 - pixel.y / uScreen.y * 2.0);
    gl_Position = vec4(ndc, 0.0, 1.0);

    vUV = uUVPos + aPos * uUVSize;
}