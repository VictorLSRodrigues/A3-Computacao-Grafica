#include "mesh.h"

#include <iostream>

// Guarda os vértices de uma forma dentro da placa de vídeo.
// Recebe uma lista de números: x, y, z, x, y, z, ...
bool Mesh::criar(const std::vector<float>& vertices)
{
    // 1. descobrir quantos vértices tem (cada vértice usa 3 números)
    quantidadeVertices = vertices.size() / 3;

    // 2. criar o VAO (a "etiqueta") e o VBO (a "caixa" dos dados)
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    // 3. selecionar os dois (começa a gravar no VAO)
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    // 4. enviar os números para a placa de vídeo
    // (tamanho em bytes = quantidade de floats * tamanho de cada um)
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // 5. explicar como ler os dados (atributo 0, 3 floats por vértice)
    // O "0" é o mesmo location = 0 do aPos no vertex shader.
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // 6. soltar os binds (deixa tudo "desligado" para não mexer sem querer)
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return true;
}

// Desenha a forma. O shader certo deve estar em uso antes (shader.usar()).
void Mesh::desenhar() const
{
    glBindVertexArray(VAO);                               // escolhe os dados
    glDrawArrays(GL_TRIANGLES, 0, quantidadeVertices);    // desenha de 3 em 3 vértices
    glBindVertexArray(0);
}

// Apaga os dados da placa de vídeo (chamar antes de fechar a janela).
void Mesh::limpar()
{
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}