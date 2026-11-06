#include "formas.h"

#include <cmath>

// 2D

std::vector<float> formas::triangulo(float lado)
{
    float altura = lado * std::sqrt(3.0f) / 2.0f;

    // O centro do triângulo fica na origem: base em -altura/3, topo em 2*altura/3.
    return {
        -lado / 2.0f, -altura / 3.0f, 0.0f,
        lado / 2.0f, -altura / 3.0f, 0.0f,
        0.0f, 2.0f * altura / 3.0f, 0.0f};
}

std::vector<float> formas::quadrado()
{
    return {
        -0.15f, -0.15f,
        0.15f, -0.15f,
        0.15f, 0.15f,

        -0.15f, -0.15f,
        0.15f, 0.15f,
        -0.15f, 0.15f};
}

std::vector<float> formas::retangulo()
{
    return {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f};
}

std::vector<unsigned int> formas::indicesRetangulo()
{
    return {0, 1, 2, 0, 2, 3};
}

// 3D

// Pirâmide: 8 floats por vértice = posição (3) + normal (3) + UV (2).
// A base usa a imagem inteira. Cada face lateral usa um triângulo da imagem.

std::vector<float> formas::piramide()
{
    const float n = 0.894427f;
    const float m = 0.447214f;

    return {
        // Base
        -0.5f, -0.5f, 0.5f, 0.83f, 0.70f, 0.44f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f,
        -0.5f, -0.5f, -0.5f, 0.83f, 0.70f, 0.44f, 0.0f, 1.0f, 0.0f, -1.0f, 0.0f,
        0.5f, -0.5f, -0.5f, 0.83f, 0.70f, 0.44f, 1.0f, 1.0f, 0.0f, -1.0f, 0.0f,
        0.5f, -0.5f, 0.5f, 0.83f, 0.70f, 0.44f, 1.0f, 0.0f, 0.0f, -1.0f, 0.0f,

        // Face esquerda
        -0.5f, -0.5f, -0.5f, 0.83f, 0.70f, 0.44f, 0.0f, 0.0f, -n, m, 0.0f,
        -0.5f, -0.5f, 0.5f, 0.83f, 0.70f, 0.44f, 1.0f, 0.0f, -n, m, 0.0f,
        0.0f, 0.5f, 0.0f, 0.92f, 0.86f, 0.76f, 0.5f, 1.0f, -n, m, 0.0f,

        // Face de trás
        0.5f, -0.5f, -0.5f, 0.83f, 0.70f, 0.44f, 0.0f, 0.0f, 0.0f, m, -n,
        -0.5f, -0.5f, -0.5f, 0.83f, 0.70f, 0.44f, 1.0f, 0.0f, 0.0f, m, -n,
        0.0f, 0.5f, 0.0f, 0.92f, 0.86f, 0.76f, 0.5f, 1.0f, 0.0f, m, -n,

        // Face direita
        0.5f, -0.5f, 0.5f, 0.83f, 0.70f, 0.44f, 0.0f, 0.0f, n, m, 0.0f,
        0.5f, -0.5f, -0.5f, 0.83f, 0.70f, 0.44f, 1.0f, 0.0f, n, m, 0.0f,
        0.0f, 0.5f, 0.0f, 0.92f, 0.86f, 0.76f, 0.5f, 1.0f, n, m, 0.0f,

        // Face da frente
        -0.5f, -0.5f, 0.5f, 0.83f, 0.70f, 0.44f, 0.0f, 0.0f, 0.0f, m, n,
        0.5f, -0.5f, 0.5f, 0.83f, 0.70f, 0.44f, 1.0f, 0.0f, 0.0f, m, n,
        0.0f, 0.5f, 0.0f, 0.92f, 0.86f, 0.76f, 0.5f, 1.0f, 0.0f, m, n};
}

std::vector<unsigned int> formas::indicesPiramide()
{
    return {
        0, 1, 2, 0, 2, 3, // base
        4, 5, 6,          // esquerda
        7, 8, 9,          // trás
        10, 11, 12,       // direita
        13, 14, 15        // frente
    };
}

// CUBO
std::vector<float> formas::cubo()
{
    return {
        // Frente
        -0.5f, -0.5f, 0.5f, // 0
        0.5f, -0.5f, 0.5f,  // 1
        0.5f, 0.5f, 0.5f,   // 2
        -0.5f, 0.5f, 0.5f,  // 3

        // Fundo
        -0.5f, -0.5f, -0.5f, // 4
        0.5f, -0.5f, -0.5f,  // 5
        0.5f, 0.5f, -0.5f,   // 6
        -0.5f, 0.5f, -0.5f   // 7
    };
}

std::vector<unsigned int> formas::indicesCubo()
{
    return {
        // Frente
        0, 1, 2, 0, 2, 3,

        // Fundo
        5, 4, 7, 5, 7, 6,

        // Direita
        1, 5, 6, 1, 6, 2,

        // Esquerda
        4, 0, 3, 4, 3, 7,

        // Cima
        3, 2, 6, 3, 6, 7,

        // Baixo
        4, 5, 1, 4, 1, 0};
}
