
#pragma once

#include <glad/glad.h>
#include <vector>

#include "buffers/VAO.h"
#include "buffers/VBO.h"
#include "buffers/EBO.h"
#include "Vertex.h"
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
    std::vector<float> verticesCPU;

    // Dados novos: vector<Vertex>
    std::vector<Vertex> verticesEstruturadosCPU;

    // Índices
    std::vector<unsigned int> indicesCPU;

    GLsizei quantidadeVertices = 0;
    GLenum modo = GL_TRIANGLES;
    bool usaIndices = false;

    // Configuração do formato antigo
    static int computeStride(
        int componentes,
        const std::vector<int>& layout
    );

    static void setupAttributes(
        int componentes,
        const std::vector<int>& layout
    );

    // Configuração do formato Vertex
    static void setupVertexAttributes();

public:
    Mesh() = default;
    ~Mesh() { limpar(); }

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    // Adicionar textura à Mesh
    void adicionarTextura(Texture&& textura);

    // Consultar texturas da Mesh
    const std::vector<Texture>& getTextures() const;

    // Criar Mesh antiga, sem índices
    bool criar(
        const std::vector<float>& vertices,
        int componentes = 3,
        GLenum modoDesenho = GL_TRIANGLES,
        const std::vector<int>& layout = {}
    );

    // Criar Mesh antiga, com índices
    bool criarEbo(
        const std::vector<float>& vertices,
        const std::vector<unsigned int>& indices,
        int componentes = 3,
        GLenum modoDesenho = GL_TRIANGLES,
        const std::vector<int>& layout = {}
    );

    // Criar Mesh com Vertex, sem índices
    bool criar(
        const std::vector<Vertex>& vertices,
        GLenum modoDesenho = GL_TRIANGLES
    );

    // Criar Mesh com Vertex, com índices
    bool criarEbo(
        const std::vector<Vertex>& vertices,
        const std::vector<unsigned int>& indices,
        GLenum modoDesenho = GL_TRIANGLES
    );

    // Desenhar Mesh
    void desenhar() const;

    // Desenhar Mesh utilizando suas próprias texturas
    void desenhar(Shader& shader) const;

    // Liberar recursos
    void limpar();

    // Acesso aos dados antigos
    const std::vector<float>& getVerticesCPU() const;

    // Acesso aos dados estruturados
    const std::vector<Vertex>& getVerticesEstruturadosCPU() const;

    // Acesso aos índices
    const std::vector<unsigned int>& getIndicesCPU() const;
};
