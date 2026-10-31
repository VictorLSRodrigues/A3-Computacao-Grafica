#pragma once

#include <vector>

namespace formas
{
    // Triângulo equilátero de lado 'lado', centrado na origem (x, y, z por vértice).
    std::vector<float> triangulo(float lado = 1.0f);

    // Quadrado unitário (0 a 1), 2 componentes por vértice (x, y).
    // A posição e o tamanho reais vêm dos uniforms do shader de UI.
    std::vector<float> retangulo();

    // Cria um quadrado com largura e altura de 0.3.
    // Cada vértice possui 2 componentes: X e Y.
    std::vector<float> quadrado();

    // Piramide
    std::vector<float> piramide();
    std::vector<unsigned int> indicesPiramide();

    // Cubo
    std::vector<float> cubo();
    std::vector<unsigned int> indicesCubo();
}