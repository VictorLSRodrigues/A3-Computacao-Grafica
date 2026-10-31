#include "formas.h"

#include <cmath>

std::vector<float> formas::triangulo(float lado)
{
    float altura = lado * std::sqrt(3.0f) / 2.0f;

    // O centro do triângulo fica na origem: base em -altura/3, topo em 2*altura/3.
    return {
        -lado / 2.0f, -altura / 3.0f,        0.0f,
         lado / 2.0f, -altura / 3.0f,        0.0f,
         0.0f,         2.0f * altura / 3.0f, 0.0f};

         
}

std::vector<float> formas::quadrado()
{
    return {
        -0.15f, -0.15f,
         0.15f, -0.15f,
         0.15f,  0.15f,

        -0.15f, -0.15f,
         0.15f,  0.15f,
        -0.15f,  0.15f
    };
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

//3D

// PIRAMIDE
std::vector<float> formas::piramide()
{
    return {
        // Base quadrada
        -0.5f, -0.5f,  0.5f, // 0
        -0.5f, -0.5f, -0.5f, // 1
         0.5f, -0.5f, -0.5f, // 2
         0.5f, -0.5f,  0.5f, // 3

        // Ponta
         0.0f,  0.5f,  0.0f  // 4
    };
}

std::vector<unsigned int> formas::indicesPiramide()
{
    return {
        // Base
        0, 1, 2,
        0, 2, 3,

        // Faces laterais
        0, 1, 4,
        1, 2, 4,
        2, 3, 4,
        3, 0, 4
    };
}

// CUBO
std::vector<float> formas::cubo()
{
    return {
        // Frente
        -0.5f, -0.5f,  0.5f, // 0
         0.5f, -0.5f,  0.5f, // 1
         0.5f,  0.5f,  0.5f, // 2
        -0.5f,  0.5f,  0.5f, // 3

        // Fundo
        -0.5f, -0.5f, -0.5f, // 4
         0.5f, -0.5f, -0.5f, // 5
         0.5f,  0.5f, -0.5f, // 6
        -0.5f,  0.5f, -0.5f  // 7
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
        4, 5, 1, 4, 1, 0
    };
}
