
#include "mesh.h"

#include <iostream>
#include <limits>
#include <cstddef>
#include <utility>
#include "shader.h"

// Calcula o stride dos vértices
int Mesh::computeStride(
    int componentes,
    const std::vector<int>& layout)
{
    if (layout.empty())
        return (componentes >= 1 && componentes <= 4)
            ? componentes : 0;

    int stride = 0;

    for (int c : layout)
    {
        if (c < 1 || c > 4)
            return 0;

        stride += c;
    }

    return stride;
}

// Configura os atributos do VAO que estiver ligado
void Mesh::setupAttributes(
    int componentes,
    const std::vector<int>& layout)
{
    const std::vector<int> atributos =
        layout.empty()
        ? std::vector<int>{componentes}
        : layout;

    int stride = 0;

    for (int c : atributos)
        stride += c;

    int offset = 0;

    for (size_t i = 0; i < atributos.size(); ++i)
    {
        glVertexAttribPointer(
            static_cast<GLuint>(i),
            atributos[i],
            GL_FLOAT,
            GL_FALSE,
            static_cast<GLsizei>(stride * sizeof(float)),
            reinterpret_cast<void*>(offset * sizeof(float))
        );

        glEnableVertexAttribArray(
            static_cast<GLuint>(i)
        );

        offset += atributos[i];
    }
}

// Criar Mesh sem índices
bool Mesh::criar(
    const std::vector<float>& vertices,
    int componentes,
    GLenum modoDesenho,
    const std::vector<int>& layout)
{
    const int stride = computeStride(componentes, layout);

    if (stride == 0)
    {
        std::cout << "Mesh: layout inválido." << std::endl;
        return false;
    }

    const size_t perVertex = static_cast<size_t>(stride);

    if (vertices.empty() || vertices.size() % perVertex != 0)
    {
        std::cout << "Mesh: vertices incompatíveis." << std::endl;
        return false;
    }

    const size_t totalVertices = vertices.size() / perVertex;

    if (totalVertices > static_cast<size_t>(
            std::numeric_limits<GLsizei>::max()))
    {
        std::cout << "Mesh: quantidade de vertices muito grande." << std::endl;
        return false;
    }

    // Salvar os dados na CPU
    verticesCPU = vertices;
    verticesEstruturadosCPU.clear();
    indicesCPU.clear();

    quantidadeVertices = static_cast<GLsizei>(totalVertices);
    modo = modoDesenho;
    usaIndices = false;

    // Configurar os objetos OpenGL
    vao.Bind();

    vbo.Upload(verticesCPU);

    // O Upload do VBO desvincula o GL_ARRAY_BUFFER.
    // Precisamos ligá-lo novamente para configurar os atributos.
    vbo.Bind();

    setupAttributes(componentes, layout);

    vbo.Unbind();

    vao.Unbind();

    return true;
}



// Criar Mesh com índices
bool Mesh::criarEbo(
    const std::vector<float>& vertices,
    const std::vector<unsigned int>& indices,
    int componentes,
    GLenum modoDesenho,
    const std::vector<int>& layout)
{
    const int stride = computeStride(componentes, layout);

    if (stride == 0)
    {
        std::cout << "Mesh: layout inválido." << std::endl;
        return false;
    }

    const size_t perVertex = static_cast<size_t>(stride);

    if (vertices.empty() || vertices.size() % perVertex != 0)
    {
        std::cout << "Mesh: vertices incompatíveis." << std::endl;
        return false;
    }

    if (indices.empty())
    {
        std::cout << "Mesh: lista de indices vazia." << std::endl;
        return false;
    }

    const size_t totalVertices = vertices.size() / perVertex;

    if (totalVertices > static_cast<size_t>(
            std::numeric_limits<GLsizei>::max()) ||
        indices.size() > static_cast<size_t>(
            std::numeric_limits<GLsizei>::max()))
    {
        std::cout << "Mesh: quantidade de dados muito grande." << std::endl;
        return false;
    }

    for (unsigned int indice : indices)
    {
        if (indice >= totalVertices)
        {
            std::cout << "Mesh: indice fora do intervalo." << std::endl;
            return false;
        }
    }

    // Salvar os dados na CPU
    verticesCPU = vertices;
    verticesEstruturadosCPU.clear();
    indicesCPU = indices;

    quantidadeVertices = static_cast<GLsizei>(totalVertices);
    modo = modoDesenho;
    usaIndices = true;

    // Configurar VAO, VBO e EBO
    vao.Bind();

    vbo.Upload(verticesCPU);

    // Associar o VBO aos atributos
    vbo.Bind();

    setupAttributes(componentes, layout);

    vbo.Unbind();

    // Associar os índices ao VAO atual
    ebo.Upload(indicesCPU);

    vao.Unbind();

    return true;
}


// NOVO FORMATO: VERTEX
// Configura os atributos da estrutura Vertex
void Mesh::setupVertexAttributes()
{
    const GLsizei stride = static_cast<GLsizei>(sizeof(Vertex));

    // Location 0: posição
    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE,
        stride,
        reinterpret_cast<const void*>(offsetof(Vertex, position))
    );
    glEnableVertexAttribArray(0);

    // Location 1: cor
    glVertexAttribPointer(
        1, 3, GL_FLOAT, GL_FALSE,
        stride,
        reinterpret_cast<const void*>(offsetof(Vertex, color))
    );
    glEnableVertexAttribArray(1);

    // Location 2: coordenadas UV
    glVertexAttribPointer(
        2, 2, GL_FLOAT, GL_FALSE,
        stride,
        reinterpret_cast<const void*>(offsetof(Vertex, texUV))
    );
    glEnableVertexAttribArray(2);

    // Location 3: normal
    glVertexAttribPointer(
        3, 3, GL_FLOAT, GL_FALSE,
        stride,
        reinterpret_cast<const void*>(offsetof(Vertex, normal))
    );
    glEnableVertexAttribArray(3);
}

// Criar Mesh com Vertex sem índices
bool Mesh::criar(
    const std::vector<Vertex>& vertices,
    GLenum modoDesenho)
{
    if (vertices.empty() ||
        vertices.size() > static_cast<size_t>(
            std::numeric_limits<GLsizei>::max()))
    {
        std::cout << "Mesh: vertices inválidos." << std::endl;
        return false;
    }

    // Salvar os dados na CPU
    verticesEstruturadosCPU = vertices;
    verticesCPU.clear();
    indicesCPU.clear();

    quantidadeVertices = static_cast<GLsizei>(vertices.size());
    modo = modoDesenho;
    usaIndices = false;

    // Configurar os objetos OpenGL
    vao.Bind();

    vbo.Upload(verticesEstruturadosCPU);

    // Associar o VBO aos atributos
    vbo.Bind();

    setupVertexAttributes();

    vbo.Unbind();

    vao.Unbind();

    return true;
}

// Criar Mesh com Vertex e índices
bool Mesh::criarEbo(
    const std::vector<Vertex>& vertices,
    const std::vector<unsigned int>& indices,
    GLenum modoDesenho)
{
    if (vertices.empty() || indices.empty())
    {
        std::cout << "Mesh: vertices ou indices vazios." << std::endl;
        return false;
    }

    if (vertices.size() > static_cast<size_t>(
            std::numeric_limits<GLsizei>::max()) ||
        indices.size() > static_cast<size_t>(
            std::numeric_limits<GLsizei>::max()))
    {
        std::cout << "Mesh: quantidade de dados muito grande." << std::endl;
        return false;
    }

    for (unsigned int indice : indices)
    {
        if (indice >= vertices.size())
        {
            std::cout << "Mesh: indice fora do intervalo." << std::endl;
            return false;
        }
    }

    // Salvar os dados na CPU
    verticesEstruturadosCPU = vertices;
    verticesCPU.clear();
    indicesCPU = indices;

    quantidadeVertices = static_cast<GLsizei>(vertices.size());
    modo = modoDesenho;
    usaIndices = true;

    // Configurar VAO, VBO e EBO
    vao.Bind();

    vbo.Upload(verticesEstruturadosCPU);

    // Associar o VBO aos atributos
    vbo.Bind();

    setupVertexAttributes();

    vbo.Unbind();

    // Associar os índices ao VAO atual
    ebo.Upload(indicesCPU);

    vao.Unbind();

    return true;
}

// Desenhar Mesh
void Mesh::desenhar() const
{
    vao.Bind();

    if (usaIndices)
    {
        glDrawElements(
            modo,
            ebo.GetCount(),
            GL_UNSIGNED_INT,
            nullptr
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

    vao.Unbind();
}
// Desenhar Mesh utilizando suas próprias texturas
void Mesh::desenhar(Shader& shader) const
{
    // Percorrer as texturas armazenadas na Mesh
    for (const Texture& textura : textures)
    {
        // Identificar o tipo da textura
        if (textura.getType() == "diffuse")
        {
            textura.bind(0);
            shader.setInt("tex0", 0);
        }
        else if (textura.getType() == "specular")
        {
            textura.bind(1);
            shader.setInt("specular0", 1);
        }
    }

    // Desenhar a geometria
    desenhar();

    // Restaurar a unidade de textura padrão
    glActiveTexture(GL_TEXTURE0);
}

// Liberar recursos
void Mesh::limpar()
{
    vao.Delete();
    vbo.Delete();
    ebo.Delete();

    verticesCPU.clear();
    verticesEstruturadosCPU.clear();
    indicesCPU.clear();
    textures.clear();

    quantidadeVertices = 0;
    usaIndices = false;
}

// Acessar os dados mantidos na CPU
const std::vector<float>& Mesh::getVerticesCPU() const
{
    return verticesCPU;
}

// Acessar os vértices estruturados mantidos na CPU
const std::vector<Vertex>& Mesh::getVerticesEstruturadosCPU() const
{
    return verticesEstruturadosCPU;
}

// Acessar os índices mantidos na CPU
const std::vector<unsigned int>& Mesh::getIndicesCPU() const
{
    return indicesCPU;
}

// Adicionar textura à Mesh
void Mesh::adicionarTextura(Texture&& textura)
{
    textures.push_back(std::move(textura));
}

// Acessar as texturas mantidas na Mesh
const std::vector<Texture>& Mesh::getTextures() const
{
    return textures;
}