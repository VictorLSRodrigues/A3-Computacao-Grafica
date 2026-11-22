#pragma once

#include <vector>

#include "vertex.h"

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

    // Gera um plano no eixo XZ, centrado na origem e com normais voltadas para +Y.
    // Cada subdivisão forma uma célula quadrilateral composta por dois triângulos.
    // Retorna false para dimensões ou subdivisões inválidas.
    bool gridPlane(
        float width,
        float depth,
        unsigned int segmentsX,
        unsigned int segmentsZ,
        std::vector<Vertex> &vertices,
        std::vector<unsigned int> &indices);

    // Gera uma caixa centrada na origem, com UVs, normais e tangentes por face.
    // Cada face possui seus próprios vértices para preservar os atributos das arestas.
    // Retorna false para dimensões inválidas.
    bool box(
        float width,
        float height,
        float depth,
        std::vector<Vertex> &vertices,
        std::vector<unsigned int> &indices);

    // Gera uma esfera UV centrada na origem, com normais suaves, UVs e tangentes.
    // sectors controla os segmentos ao redor do eixo Y; stacks, os segmentos verticais.
    // Retorna false para raio ou subdivisões inválidas.
    bool sphere(
        float radius,
        unsigned int sectors,
        unsigned int stacks,
        std::vector<Vertex> &vertices,
        std::vector<unsigned int> &indices);

    // Cubo com oito posições XYZ; índices definem as seis faces.
    std::vector<float> cube();
    std::vector<unsigned int> cubeIndices();

}