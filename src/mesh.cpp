#include "mesh.h"

#include <iostream>

// Soma os componentes de cada atributo. Layout vazio = formato antigo.
// Devolve 0 se algum atributo tiver menos de 1 ou mais de 4 componentes.
int Mesh::computeStride(int componentes, const std::vector<int>& layout)
{
    if (layout.empty())
        return (componentes >= 1 && componentes <= 4) ? componentes : 0;

    int stride = 0;
    for (int c : layout)
    {
        if (c < 1 || c > 4)
            return 0;
        stride += c;
    }
    return stride;
}

// Explica à placa de vídeo como ler cada atributo do vértice (posição, normal, UV...).
void Mesh::setupAttributes(int componentes, const std::vector<int>& layout)
{
    const std::vector<int> atributos = layout.empty() ? std::vector<int>{componentes} : layout;

    int stride = 0;
    for (int c : atributos)
        stride += c;

    int offset = 0;
    for (size_t i = 0; i < atributos.size(); ++i)
    {
        glVertexAttribPointer(static_cast<GLuint>(i), atributos[i], GL_FLOAT, GL_FALSE,
                              static_cast<GLsizei>(stride * sizeof(float)),
                              (void *)(offset * sizeof(float)));
        glEnableVertexAttribArray(static_cast<GLuint>(i));
        offset += atributos[i];
    }
}

bool Mesh::criar(const std::vector<float> &vertices, int componentes, GLenum modoDesenho,
                 const std::vector<int> &layout)
{
    // 0. checar se os dados fazem sentido antes de mexer na placa de vídeo
    const int stride = computeStride(componentes, layout);
    if (stride == 0)
    {
        std::cout << "Mesh: componentes deve ser de 1 a 4 (em cada atributo)." << std::endl;
        return false;
    }
    const size_t perVertex = static_cast<size_t>(stride);
    if (vertices.empty() || vertices.size() % perVertex != 0)
    {
        std::cout << "Mesh: lista vazia ou tamanho incompatível com " << stride
                  << " floats por vértice." << std::endl;
        return false;
    }

    limpar(); // se já havia uma malha aqui, apaga antes de criar a nova

    // 1. guardar a configuração
    quantidadeVertices = static_cast<GLsizei>(vertices.size() / perVertex);
    modo = modoDesenho;

    // 2. criar o VAO e o VBO
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    // 3. selecionar os dois (começa a gravar no VAO)
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    // 4. enviar os números para a placa de vídeo
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // 5. explicar como ler cada atributo
    setupAttributes(componentes, layout);

    // 6. soltar os binds
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return true;
}

bool Mesh::criarEbo(
    const std::vector<float> &vertices,
    const std::vector<unsigned int> &indices,
    int componentes,
    GLenum modoDesenho,
    const std::vector<int> &layout)
{
    // 1. Verificar os dados
    const int stride = computeStride(componentes, layout);
    if (stride == 0)
    {
        std::cout << "Mesh: componentes deve ser de 1 a 4 (em cada atributo)." << std::endl;
        return false;
    }
    const size_t perVertex = static_cast<size_t>(stride);

    if (vertices.empty() || vertices.size() % perVertex != 0)
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
        if (indice >= vertices.size() / perVertex)
        {
            std::cout << "Mesh: indice fora do intervalo dos vertices." << std::endl;
            return false;
        }
    }

    // 2. Limpar malha anterior
    limpar();

    // 3. Guardar configuracoes
    quantidadeVertices = static_cast<GLsizei>(vertices.size() / perVertex);
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
        GL_STATIC_DRAW);

    // 7. Configurar atributos
    setupAttributes(componentes, layout);

    // 8. Enviar indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        indices.size() * sizeof(unsigned int),
        indices.data(),
        GL_STATIC_DRAW);

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
            0);
    }
    else
    {
        glDrawArrays(
            modo,
            0,
            quantidadeVertices);
    }

    glBindVertexArray(0);
}

void Mesh::limpar()
{
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO); 
    VAO = 0;
    VBO = 0;
    EBO = 0;
    quantidadeVertices = 0;
    quantidadeIndices = 0;
    usaIndices = false;
}