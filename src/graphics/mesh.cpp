
#include "mesh.h"

#include <iostream>
#include <limits>
#include <cstddef>
#include <utility>
#include <algorithm>
#include "shader.h"

namespace
{
    // Calcula o maior afastamento dos vértices em relação à origem.
    float calculateBoundingRadius(
        const std::vector<float>& vertices,
        int stride)
    {
        if (stride < 3)
            return 0.0f;

        float radius = 0.0f;

        for (size_t i = 0; i < vertices.size(); i += stride)
        {
            glm::vec3 position(
                vertices[i],
                vertices[i + 1],
                vertices[i + 2]
            );

            radius = std::max(radius, glm::length(position));
        }

        return radius;
    }

    float calculateBoundingRadius(
        const std::vector<Vertex>& vertices)
    {
        float radius = 0.0f;

        for (const Vertex& vertex : vertices)
        {
            radius = std::max(
                radius,
                glm::length(vertex.position)
            );
        }

        return radius;
    }
}

// Calcula o número de valores por vértice, conforme o layout informado.
int Mesh::computeStride(
    int componentCount,
    const std::vector<int> &layout)
{
    if (layout.empty())
        return (componentCount >= 1 && componentCount <= 4)
                   ? componentCount
                   : 0;

    int stride = 0;

    for (int c : layout)
    {
        if (c < 1 || c > 4)
            return 0;

        stride += c;
    }

    return stride;
}

// Informa ao VAO como separar os atributos dentro de cada vértice.
void Mesh::setupAttributes(
    int componentCount,
    const std::vector<int> &layout)
{
    const std::vector<int> attributes =
        layout.empty()
            ? std::vector<int>{componentCount}
            : layout;

    int stride = 0;

    for (int c : attributes)
        stride += c;

    int offset = 0;

    for (size_t i = 0; i < attributes.size(); ++i)
    {
        glVertexAttribPointer(
            static_cast<GLuint>(i),
            attributes[i],
            GL_FLOAT,
            GL_FALSE,
            static_cast<GLsizei>(stride * sizeof(float)),
            reinterpret_cast<void *>(offset * sizeof(float)));

        glEnableVertexAttribArray(
            static_cast<GLuint>(i));

        offset += attributes[i];
    }
}

// Cria uma malha sem índices.
bool Mesh::create(
    const std::vector<float> &vertices,
    int componentCount,
    GLenum drawMode,
    const std::vector<int> &layout)
{
    const int stride = computeStride(componentCount, layout);

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

    // Mantém uma cópia dos dados para consulta e desenho.
    vertexData = vertices;
    boundingRadius = calculateBoundingRadius(vertices, stride);
    structuredVertices.clear();
    indexData.clear();

    vertexCount = static_cast<GLsizei>(totalVertices);
    this->drawMode = drawMode;
    usesIndices = false;

    // Envia os vértices e configura seus atributos no VAO.
    vao.Bind();

    vbo.Upload(vertexData);

    // Upload desvincula GL_ARRAY_BUFFER; associe-o novamente antes de configurar os atributos.
    vbo.Bind();

    setupAttributes(componentCount, layout);

    vbo.Unbind();

    vao.Unbind();

    return true;
}

// Cria uma malha cujos triângulos referenciam vértices por índice.
bool Mesh::createIndexed(
    const std::vector<float> &vertices,
    const std::vector<unsigned int> &indices,
    int componentCount,
    GLenum drawMode,
    const std::vector<int> &layout)
{
    const int stride = computeStride(componentCount, layout);

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

    for (unsigned int index : indices)
    {
        if (index >= totalVertices)
        {
            std::cout << "Mesh: indice fora do intervalo." << std::endl;
            return false;
        }
    }

    // Mantém cópias dos vértices e índices para consulta.
    vertexData = vertices;
    boundingRadius = calculateBoundingRadius(vertices, stride);
    structuredVertices.clear();
    indexData = indices;

    vertexCount = static_cast<GLsizei>(totalVertices);
    this->drawMode = drawMode;
    usesIndices = true;

    // Configura os vértices no VBO e os índices no EBO associado ao VAO.
    vao.Bind();

    vbo.Upload(vertexData);

    vbo.Bind();

    setupAttributes(componentCount, layout);

    vbo.Unbind();

    // O EBO fica associado ao VAO que está ativo.
    ebo.Upload(indexData);

    vao.Unbind();

    return true;
}

// Configura o layout fixo da estrutura Vertex.
void Mesh::setupVertexAttributes()
{
    const GLsizei stride = static_cast<GLsizei>(sizeof(Vertex));

    // Localização 0: posição.
    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE,
        stride,
        reinterpret_cast<const void *>(offsetof(Vertex, position)));
    glEnableVertexAttribArray(0);

    // Localização 1: cor.
    glVertexAttribPointer(
        1, 3, GL_FLOAT, GL_FALSE,
        stride,
        reinterpret_cast<const void *>(offsetof(Vertex, color)));
    glEnableVertexAttribArray(1);

    // Localização 2: coordenadas da textura.
    glVertexAttribPointer(
        2, 2, GL_FLOAT, GL_FALSE,
        stride,
        reinterpret_cast<const void *>(offsetof(Vertex, texUV)));
    glEnableVertexAttribArray(2);

    // Localização 3: normal.
    glVertexAttribPointer(
        3, 3, GL_FLOAT, GL_FALSE,
        stride,
        reinterpret_cast<const void *>(offsetof(Vertex, normal)));
    glEnableVertexAttribArray(3);
}

// Cria uma malha de vértices estruturados sem índices.
bool Mesh::create(
    const std::vector<Vertex> &vertices,
    GLenum drawMode)
{
    if (vertices.empty() ||
        vertices.size() > static_cast<size_t>(
                              std::numeric_limits<GLsizei>::max()))
    {
        std::cout << "Mesh: vertices inválidos." << std::endl;
        return false;
    }

    // Mantém uma cópia dos vértices para consulta.
    structuredVertices = vertices;
    boundingRadius = calculateBoundingRadius(vertices);
    vertexData.clear();
    indexData.clear();

    vertexCount = static_cast<GLsizei>(vertices.size());
    this->drawMode = drawMode;
    usesIndices = false;

    // Envia os vértices e configura seus atributos no VAO.
    vao.Bind();

    vbo.Upload(structuredVertices);

    vbo.Bind();

    setupVertexAttributes();

    vbo.Unbind();

    vao.Unbind();

    return true;
}

// Cria uma malha de vértices estruturados com índices.
bool Mesh::createIndexed(
    const std::vector<Vertex> &vertices,
    const std::vector<unsigned int> &indices,
    GLenum drawMode)
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

    for (unsigned int index : indices)
    {
        if (index >= vertices.size())
        {
            std::cout << "Mesh: indice fora do intervalo." << std::endl;
            return false;
        }
    }

    // Mantém cópias dos vértices e índices para consulta.
    structuredVertices = vertices;
    boundingRadius = calculateBoundingRadius(vertices);
    vertexData.clear();
    indexData = indices;

    vertexCount = static_cast<GLsizei>(vertices.size());
    this->drawMode = drawMode;
    usesIndices = true;

    // Configura os vértices no VBO e os índices no EBO associado ao VAO.
    vao.Bind();

    vbo.Upload(structuredVertices);

    vbo.Bind();

    setupVertexAttributes();

    vbo.Unbind();

    // O EBO fica associado ao VAO que está ativo.
    ebo.Upload(indexData);

    vao.Unbind();

    return true;
}

void Mesh::draw() const
{
    vao.Bind();

    if (usesIndices)
    {
        glDrawElements(
            drawMode,
            ebo.GetCount(),
            GL_UNSIGNED_INT,
            nullptr);
    }
    else
    {
        glDrawArrays(
            drawMode,
            0,
            vertexCount);
    }

    vao.Unbind();
}
void Mesh::draw(Shader &shader, const Texture *diffuseFallback) const
{
    bool hasDiffuse = false;
    bool hasSpecular = false;

    // Associa cada textura à unidade usada pelo shader.
    for (const Texture &texture : textures)
    {
        if (texture.getType() == "diffuse")
        {
            hasDiffuse = true;
            texture.bind(0);
            shader.setInt("tex0", 0);
        }
        else if (texture.getType() == "specular")
        {
            hasSpecular = true;
            texture.bind(1);
            shader.setInt("specular0", 1);
        }
    }

    if (!hasDiffuse && diffuseFallback)
    {
        diffuseFallback->bind(0);
        shader.setInt("tex0", 0);
    }

    if (!hasSpecular)
    {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);
        shader.setInt("specular0", 1);
    }
    shader.setVec4("baseColor", baseColor.r, baseColor.g, baseColor.b, baseColor.a);
    draw();

    // Restaurar a unidade de textura padrão
    glActiveTexture(GL_TEXTURE0);
}

void Mesh::destroy()
{
    vao.Delete();
    vbo.Delete();
    ebo.Delete();

    vertexData.clear();
    structuredVertices.clear();
    indexData.clear();
    textures.clear();

    vertexCount = 0;
    boundingRadius = 0.0f;
    usesIndices = false;
}

const std::vector<float> &Mesh::getVertexData() const
{
    return vertexData;
}

const std::vector<Vertex> &Mesh::getStructuredVertices() const
{
    return structuredVertices;
}

const std::vector<unsigned int> &Mesh::getIndexData() const
{
    return indexData;
}

void Mesh::addTexture(Texture &&texture)
{
    textures.push_back(std::move(texture));
}

const std::vector<Texture> &Mesh::getTextures() const
{
    return textures;
}
