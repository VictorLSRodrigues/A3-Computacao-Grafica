#include "shapes.h"

#include <cmath>

// 2D

std::vector<float> shapes::triangle(float side)
{
    float height = side * std::sqrt(3.0f) / 2.0f;

    // Posiciona a base e o topo para que o centro geométrico fique na origem.
    return {
        -side / 2.0f, -height / 3.0f, 0.0f,
        side / 2.0f, -height / 3.0f, 0.0f,
        0.0f, 2.0f * height / 3.0f, 0.0f};
}

std::vector<float> shapes::square()
{
    return {
        -0.15f, -0.15f,
        0.15f, -0.15f,
        0.15f, 0.15f,

        -0.15f, -0.15f,
        0.15f, 0.15f,
        -0.15f, 0.15f};
}

std::vector<float> shapes::rectangle()
{
    return {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f};
}

std::vector<unsigned int> shapes::rectangleIndices()
{
    return {0, 1, 2, 0, 2, 3};
}

// 3D

// Cada vértice da pirâmide tem 11 valores: posição (3), cor (3), UV (2) e normal (3).
// A base usa a imagem inteira. Cada face lateral usa um triângulo da imagem.

std::vector<float> shapes::pyramid()
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

std::vector<unsigned int> shapes::pyramidIndices()
{
    return {
        0, 1, 2, 0, 2, 3, // base
        4, 5, 6,          // esquerda
        7, 8, 9,          // trás
        10, 11, 12,       // direita
        13, 14, 15        // frente
    };
}

// Cubo
std::vector<float> shapes::cube()
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

std::vector<unsigned int> shapes::cubeIndices()
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
