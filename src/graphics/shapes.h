#pragma once

#include <vector>

namespace shapes
{
    // Triângulo equilátero de lado 'lado', centrado na origem (x, y, z por vértice).
    std::vector<float> triangle(float side = 1.0f);

    // Quadrado unitário (0 a 1), 2 componentes por vértice (x, y), com índices.
    std::vector<float> rectangle();
    std::vector<unsigned int> rectangleIndices();

    // Cria um quadrado com largura e altura de 0.3.
    // Cada vértice possui 2 componentes: X e Y.
    std::vector<float> square();

    // Pirâmide com normais e UV: layout {3, 3, 2} (posição, normal, UV).
    // Cada face tem os seus próprios vértices (16 no total).
    std::vector<float> pyramid();
    std::vector<unsigned int> pyramidIndices();

    // Cubo
    std::vector<float> cube();
    std::vector<unsigned int> cubeIndices();

}