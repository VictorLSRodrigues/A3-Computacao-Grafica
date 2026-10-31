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


bool Mesh::criarEbo(
    const std::vector<float>& vertices,
    const std::vector<unsigned int>& indices,
    int componentes,
    GLenum modoDesenho)
{
    // 1. Verificar os dados
    if (componentes < 1 || componentes > 4)
    {
        std::cout << "Mesh: componentes deve ser de 1 a 4." << std::endl;
        return false;
    }

    if (vertices.empty() || vertices.size() % componentes != 0)
    {
        std::cout << "Mesh: vertices invalidos." << std::endl;
        return false;
    }

    if (indices.empty())
    {
        std::cout << "Mesh: lista de indices vazia." << std::endl;
        return false;
    }

    if (indices.size() > static_cast<size_t>(-1) / sizeof(unsigned int))
    {
        std::cout << "Mesh: quantidade de indices muito grande." << std::endl;
        return false;
    }

    for (unsigned int indice : indices)
    {
        if (indice >= vertices.size() / componentes)
        {
            std::cout << "Mesh: indice fora do intervalo dos vertices." << std::endl;
            return false;
        }
    }

    // 2. Limpar malha anterior
    limpar();

    // 3. Guardar configuracoes
    quantidadeVertices = static_cast<GLsizei>(vertices.size() / componentes);
    quantidadeIndices = static_cast<GLsizei>(indices.size());
    modo = modoDesenho;
    usaIndices = true;

    // 4. Criar VAO, VBO e EBO
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    // 5. Selecionar VAO
    glBindVertexArray(VAO);

    // 6. Enviar vertices
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * sizeof(float),
        vertices.data(),
        GL_STATIC_DRAW
    );

    // 7. Configurar atributos
    glVertexAttribPointer(
        0,
        componentes,
        GL_FLOAT,
        GL_FALSE,
        componentes * sizeof(float),
        (void*)0
    );

    glEnableVertexAttribArray(0);

    // 8. Enviar indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        indices.size() * sizeof(unsigned int),
        indices.data(),
        GL_STATIC_DRAW
    );

    // 9. Desvincular VAO
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return true;
}

void Mesh::desenhar() const
{
    glBindVertexArray(VAO);

    if (usaIndices)
    {
        glDrawElements(
            modo,
            quantidadeIndices,
            GL_UNSIGNED_INT,
            0
        );
    }
    else
    {
        glDrawArrays(
            modo,
            0,
            quantidadeVertices
        );
    }

    glBindVertexArray(0);
}


void Mesh::limpar()
{
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    VAO = 0;
    VBO = 0;
    EBO = 0;
    quantidadeVertices = 0;
    quantidadeIndices = 0;
    usaIndices = false;
}