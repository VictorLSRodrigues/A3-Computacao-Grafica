#version 330 core
layout (location = 0) in vec2 aPos;

uniform vec2 uPos;      // canto superior esquerdo do retângulo, em pixels
uniform vec2 uTamanho;  // largura e altura do retângulo, em pixels
uniform vec2 uTela;     // largura e altura da janela, em pixels

void main()
{
    // aPos vai de 0 a 1: escala pelo tamanho e desloca pela posição
    vec2 pixel = uPos + aPos * uTamanho;

    // pixels -> coordenadas da tela do OpenGL (-1 a +1, com y invertido)
    vec2 ndc = vec2(pixel.x / uTela.x * 2.0 - 1.0,
                    1.0 - pixel.y / uTela.y * 2.0);

    gl_Position = vec4(ndc, 0.0, 1.0);
}