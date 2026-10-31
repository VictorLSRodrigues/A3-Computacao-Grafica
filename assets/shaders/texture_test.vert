#version 330 core
layout (location = 0) in vec2 aPos; // o retângulo vai de 0 a 1

out vec2 vUV;

void main()
{
    gl_Position = vec4(aPos * 1.2 - 0.6, 0.0, 1.0); // centraliza na tela
    vUV = aPos;                                      // aqui 0..1 já serve de UV
}