#pragma once

#include <vector>

namespace shapes
{
    // Triângulo equilátero centrado na origem. Retorna três posições XYZ.
    std::vector<float> triangle(float side = 1.0f);

    // Retângulo unitário de 0 a 1, com quatro posições XY e índices para dois triângulos.
    std::vector<float> rectangle();
    std::vector<unsigned int> rectangleIndices();

    // Quadrado de 0,3 por 0,3, centrado na origem e formado por seis posições XY.
    std::vector<float> square();

    // Pirâmide com posições, cores, coordenadas de textura e normais.
    // O formato de cada vértice é {3, 3, 2, 3}; faces laterais têm vértices próprios.
    std::vector<float> pyramid();
    std::vector<unsigned int> pyramidIndices();

    // Cubo com oito posições XYZ; índices definem as seis faces.
    std::vector<float> cube();
    std::vector<unsigned int> cubeIndices();

}