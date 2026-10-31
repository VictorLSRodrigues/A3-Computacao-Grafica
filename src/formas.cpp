#include "formas.h"

#include <cmath>

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

        0.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f};
}

// 3D

// Pirâmide: 8 floats por vértice = posição (3) + normal (3) + UV (2).
// A base usa a imagem inteira. Cada face lateral usa um triângulo da imagem.
std::vector<float> formas::piramide()
{
    // Normais das faces laterais: o vetor (1, 0.5) normalizado.
    const float n = 0.894427f;
    const float m = 0.447214f;

    return {
        // Base (normal pra baixo)
        -0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f,  // 0
        -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, // 1
        0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f,  // 2
        0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,   // 3

        // Face esquerda
        -0.5f, -0.5f, -0.5f, -n, m, 0.0f, 0.0f, 0.0f, // 4
        -0.5f, -0.5f, 0.5f, -n, m, 0.0f, 1.0f, 0.0f,  // 5
        0.0f, 0.5f, 0.0f, -n, m, 0.0f, 0.5f, 1.0f,    // 6

        // Face de trás
        0.5f, -0.5f, -0.5f, 0.0f, m, -n, 0.0f, 0.0f,  // 7
        -0.5f, -0.5f, -0.5f, 0.0f, m, -n, 1.0f, 0.0f, // 8
        0.0f, 0.5f, 0.0f, 0.0f, m, -n, 0.5f, 1.0f,    // 9

        // Face direita
        0.5f, -0.5f, 0.5f, n, m, 0.0f, 0.0f, 0.0f,  // 10
        0.5f, -0.5f, -0.5f, n, m, 0.0f, 1.0f, 0.0f, // 11
        0.0f, 0.5f, 0.0f, n, m, 0.0f, 0.5f, 1.0f,   // 12

        // Face da frente
        -0.5f, -0.5f, 0.5f, 0.0f, m, n, 0.0f, 0.0f, // 13
        0.5f, -0.5f, 0.5f, 0.0f, m, n, 1.0f, 0.0f,  // 14
        0.0f, 0.5f, 0.0f, 0.0f, m, n, 0.5f, 1.0f    // 15
    };
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
