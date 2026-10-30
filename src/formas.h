#pragma once

#include <vector>

namespace formas
{
    // Triângulo equilátero de lado 'lado', centrado na origem (x, y, z por vértice).
    std::vector<float> triangulo(float lado = 1.0f);

    // Quadrado unitário (0 a 1), 2 componentes por vértice (x, y).
    // A posição e o tamanho reais vêm dos uniforms do shader de UI.
    std::vector<float> retangulo();
}