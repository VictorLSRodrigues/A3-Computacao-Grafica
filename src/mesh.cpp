#include "mesh.h"

#include <iostream>

// Soma os componentes de cada atributo. Layout vazio = formato antigo.
// Devolve 0 se algum atributo tiver menos de 1 ou mais de 4 componentes.
int Mesh::computeStride(int componentes, const std::vector<int> &layout)
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
void Mesh::setupAttributes(int componentes, const std::vector<int> &layout)
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
    return upload(vertices, {}, componentes, modoDesenho, layout);
}

bool Mesh::criarEbo(const std::vector<float> &vertices, const std::vector<unsigned int> &indices,
                    int componentes, GLenum modoDesenho, const std::vector<int> &layout)
{
    if (indices.empty())
    {
        std::cout << "Mesh: lista de indices vazia." << std::endl;
        return false;
    }
    return upload(vertices, indices, componentes, modoDesenho, layout);
}

bool Mesh::upload(const std::vector<float> &vertices, const std::vector<unsigned int> &indices,
                  int componentes, GLenum modoDesenho, const std::vector<int> &layout)
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
        std::cout << "Mesh: lista vazia ou tamanho incompatível com " << stride
                  << " floats por vértice." << std::endl;
        return false;
    }

    const size_t vertexCount = vertices.size() / perVertex;
    const bool indexed = !indices.empty();

    if (indexed)
    {
        for (unsigned int index : indices)
        {
            if (index >= vertexCount)
            {
                std::cout << "Mesh: indice fora do intervalo dos vertices." << std::endl;
                return false;
            }
        }
    }

    // 2. Apagar a malha anterior (só depois de validar, pra não perder uma malha boa)
    limpar();

    // 3. Guardar a configuração
    quantidadeVertices = static_cast<GLsizei>(vertexCount);
    quantidadeIndices = static_cast<GLsizei>(indices.size());
    modo = modoDesenho;
    usaIndices = indexed;

    // 4. Criar os buffers
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    if (indexed)
        glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    // 5. Enviar os vértices
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // 6. Explicar como ler cada atributo
    setupAttributes(componentes, layout);

    // 7. Enviar os índices (o EBO fica guardado dentro do VAO)
    if (indexed)
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    }

    // 8. Soltar os binds (o VAO primeiro, pra ele não perder o EBO)
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