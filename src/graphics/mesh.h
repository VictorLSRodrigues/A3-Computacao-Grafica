
#pragma once

#include <glad/glad.h>
#include <vector>

#include "buffers/VAO.h"
#include "buffers/VBO.h"
#include "buffers/EBO.h"
#include "vertex.h"
#include "texture.h"

class Shader;

class Mesh
{
private:
    // Gerenciadores OpenGL
    VAO vao;
    VBO vbo;
    EBO ebo;

    // Texturas pertencentes à Mesh
    std::vector<Texture> textures;

    // Dados antigos: vector<float>
    std::vector<float> vertexData;

    // Dados novos: vector<Vertex>
    std::vector<Vertex> structuredVertices;

    // Índices
    std::vector<unsigned int> indexData;

    GLsizei vertexCount = 0;
    GLenum drawMode = GL_TRIANGLES;
    bool usesIndices = false;

    // Configuração do formato antigo
    static int computeStride(
        int componentCount,
        const std::vector<int>& layout
    );

    static void setupAttributes(
        int componentCount,
        const std::vector<int>& layout
    );

    // Configuração do formato Vertex
    static void setupVertexAttributes();

public:
    Mesh() = default;
    ~Mesh() { destroy(); }

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    // Adicionar textura à Mesh
    void addTexture(Texture&& texture);

    // Consultar texturas da Mesh
    const std::vector<Texture>& getTextures() const;

    // Criar Mesh antiga, sem índices
    bool create(
        const std::vector<float>& vertices,
        int componentCount = 3,
        GLenum drawMode = GL_TRIANGLES,
        const std::vector<int>& layout = {}
    );

    // Criar Mesh antiga, com índices
    bool createIndexed(
        const std::vector<float>& vertices,
        const std::vector<unsigned int>& indices,
        int componentCount = 3,
        GLenum drawMode = GL_TRIANGLES,
        const std::vector<int>& layout = {}
    );

    // Criar Mesh com Vertex, sem índices
    bool create(
        const std::vector<Vertex>& vertices,
        GLenum drawMode = GL_TRIANGLES
    );

    // Criar Mesh com Vertex, com índices
    bool createIndexed(
        const std::vector<Vertex>& vertices,
        const std::vector<unsigned int>& indices,
        GLenum drawMode = GL_TRIANGLES
    );

    // Desenhar Mesh
    void draw() const;

    // Desenhar Mesh utilizando suas próprias texturas
    void draw(Shader& shader, const Texture* diffuseFallback = nullptr) const;

    // Liberar recursos
    void destroy();

    // Acesso aos dados antigos
    const std::vector<float>& getVertexData() const;

    // Acesso aos dados estruturados
    const std::vector<Vertex>& getStructuredVertices() const;

    // Acesso aos índices
    const std::vector<unsigned int>& getIndexData() const;
};
