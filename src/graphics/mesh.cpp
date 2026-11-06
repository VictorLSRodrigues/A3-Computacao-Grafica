
#include "mesh.h"

#include <iostream>
#include <limits>
#include <cstddef>
#include <utility>
#include "shader.h"

// Calcula o stride dos vértices
int Mesh::computeStride(
    int componentCount,
    const std::vector<int>& layout)
{
    if (layout.empty())
        return (componentCount >= 1 && componentCount <= 4)
            ? componentCount : 0;

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
    int componentCount,
    const std::vector<int>& layout)
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
            reinterpret_cast<void*>(offset * sizeof(float))
        );

        glEnableVertexAttribArray(
            static_cast<GLuint>(i)
        );

        offset += attributes[i];
    }
}

// Criar Mesh sem índices
bool Mesh::create(
    const std::vector<float>& vertices,
    int componentCount,
    GLenum drawMode,
    const std::vector<int>& layout)
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

    // Salvar os dados na CPU
    vertexData = vertices;
    structuredVertices.clear();
    indexData.clear();

    vertexCount = static_cast<GLsizei>(totalVertices);
    this->drawMode = drawMode;
    usesIndices = false;

    // Configurar os objetos OpenGL
    vao.Bind();

    vbo.Upload(vertexData);

    // O Upload do VBO desvincula o GL_ARRAY_BUFFER.
    // Precisamos ligá-lo novamente para configurar os atributos.
    vbo.Bind();

    setupAttributes(componentCount, layout);

    vbo.Unbind();

    vao.Unbind();

    return true;
}



// Criar Mesh com índices
bool Mesh::createIndexed(
    const std::vector<float>& vertices,
    const std::vector<unsigned int>& indices,
    int componentCount,
    GLenum drawMode,
    const std::vector<int>& layout)
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

    // Salvar os dados na CPU
    vertexData = vertices;
    structuredVertices.clear();
    indexData = indices;

    vertexCount = static_cast<GLsizei>(totalVertices);
    this->drawMode = drawMode;
    usesIndices = true;

    // Configurar VAO, VBO e EBO
    vao.Bind();

    vbo.Upload(vertexData);

    // Associar o VBO aos atributos
    vbo.Bind();

    setupAttributes(componentCount, layout);

    vbo.Unbind();

    // Associar os índices ao VAO atual
    ebo.Upload(indexData);

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
bool Mesh::create(
    const std::vector<Vertex>& vertices,
    GLenum drawMode)
{
    if (vertices.empty() ||
        vertices.size() > static_cast<size_t>(
            std::numeric_limits<GLsizei>::max()))
    {
        std::cout << "Mesh: vertices inválidos." << std::endl;
        return false;
    }

    // Salvar os dados na CPU
    structuredVertices = vertices;
    vertexData.clear();
    indexData.clear();

    vertexCount = static_cast<GLsizei>(vertices.size());
    this->drawMode = drawMode;
    usesIndices = false;

    // Configurar os objetos OpenGL
    vao.Bind();

    vbo.Upload(structuredVertices);

    // Associar o VBO aos atributos
    vbo.Bind();

    setupVertexAttributes();

    vbo.Unbind();

    vao.Unbind();

    return true;
}

// Criar Mesh com Vertex e índices
bool Mesh::createIndexed(
    const std::vector<Vertex>& vertices,
    const std::vector<unsigned int>& indices,
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

    // Salvar os dados na CPU
    structuredVertices = vertices;
    vertexData.clear();
    indexData = indices;

    vertexCount = static_cast<GLsizei>(vertices.size());
    this->drawMode = drawMode;
    usesIndices = true;

    // Configurar VAO, VBO e EBO
    vao.Bind();

    vbo.Upload(structuredVertices);

    // Associar o VBO aos atributos
    vbo.Bind();

    setupVertexAttributes();

    vbo.Unbind();

    // Associar os índices ao VAO atual
    ebo.Upload(indexData);

    vao.Unbind();

    return true;
}

// Desenhar Mesh
void Mesh::draw() const
{
    vao.Bind();

    if (usesIndices)
    {
        glDrawElements(
            drawMode,
            ebo.GetCount(),
            GL_UNSIGNED_INT,
            nullptr
        );
    }
    else
    {
        glDrawArrays(
            drawMode,
            0,
            vertexCount
        );
    }

    vao.Unbind();
}
// Desenhar Mesh utilizando suas próprias texturas
void Mesh::draw(Shader& shader, const Texture* diffuseFallback) const
{
    bool hasDiffuse = false;
    bool hasSpecular = false;

    // Percorrer as texturas armazenadas na Mesh
    for (const Texture& texture : textures)
    {
        // Identificar o tipo da textura
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

    // Desenhar a geometria
    draw();

    // Restaurar a unidade de textura padrão
    glActiveTexture(GL_TEXTURE0);
}

// Liberar recursos
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
    usesIndices = false;
}

// Acessar os dados mantidos na CPU
const std::vector<float>& Mesh::getVertexData() const
{
    return vertexData;
}

// Acessar os vértices estruturados mantidos na CPU
const std::vector<Vertex>& Mesh::getStructuredVertices() const
{
    return structuredVertices;
}

// Acessar os índices mantidos na CPU
const std::vector<unsigned int>& Mesh::getIndexData() const
{
    return indexData;
}

// Adicionar textura à Mesh
void Mesh::addTexture(Texture&& texture)
{
    textures.push_back(std::move(texture));
}

// Acessar as texturas mantidas na Mesh
const std::vector<Texture>& Mesh::getTextures() const
{
    return textures;
}