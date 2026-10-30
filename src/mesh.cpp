#include "mesh.h"

#include <iostream>

bool Mesh::criar(const std::vector<float> &vertices, int componentes, GLenum modoDesenho)
{
    // 0. checar se os dados fazem sentido antes de mexer na placa de vídeo
    if (componentes < 1 || componentes > 4)
    {
        std::cout << "Mesh: componentes deve ser de 1 a 4." << std::endl;
        return false;
    }
    if (vertices.empty() || vertices.size() % componentes != 0)
    {
        std::cout << "Mesh: lista vazia ou tamanho incompatível com " << componentes
                  << " componentes por vértice." << std::endl;
        return false;
    }

    limpar(); // se já havia uma malha aqui, apaga antes de criar a nova
    
    // 1. guardar a configuração
    quantidadeVertices = static_cast<GLsizei>(vertices.size() / componentes);
    modo = modoDesenho;

    // 2. criar o VAO e o VBO
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    // 3. selecionar os dois (começa a gravar no VAO)
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    // 4. enviar os números para a placa de vídeo
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // 5. explicar como ler (atributo 0, 'componentes' floats por vértice)
    glVertexAttribPointer(0, componentes, GL_FLOAT, GL_FALSE, componentes * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // 6. soltar os binds
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return true;
}

void Mesh::desenhar() const
{
    glBindVertexArray(VAO);
    glDrawArrays(modo, 0, quantidadeVertices);
    glBindVertexArray(0);
}

void Mesh::limpar()
{
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    VAO = 0;
    VBO = 0;
    quantidadeVertices = 0;
}