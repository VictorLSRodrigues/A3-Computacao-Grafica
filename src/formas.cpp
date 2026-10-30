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
