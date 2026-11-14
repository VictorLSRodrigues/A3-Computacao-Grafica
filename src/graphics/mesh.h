
#pragma once

#include <glad/glad.h>
#include <vector>

#include "buffers/VAO.h"
#include "buffers/VBO.h"
#include "buffers/EBO.h"
#include "vertex.h"
#include "texture.h"
#include "bounding_box.h"
#include "face.h"

class Shader;

class Mesh
{
private:
    VAO vao;
    VBO vbo;
    EBO ebo;

    std::vector<Texture> textures;

    // Vértices em uma lista de números, com o formato descrito por layout.
    std::vector<float> vertexData;

    // Forma alternativa: cada vértice agrupa posição, cor, UV e normal.
    std::vector<Vertex> structuredVertices;

    std::vector<unsigned int> indexData;

    // Topologia editável da malha.
    std::vector<Face> faces;

    // Estado temporário da operação de extrusão.
    std::vector<Vertex> extrusionBaseVertices;
    std::vector<unsigned int> extrusionBaseIndices;
    std::vector<Face> extrusionBaseFaces;

    unsigned int extrusionFaceIndex = 0;
    bool extrusionInProgress = false;

    GLsizei vertexCount = 0;
    GLenum drawMode = GL_TRIANGLES;
    bool usesIndices = false;

    // Cor que multiplica a textura ao desenhar (vem do material do modelo).
    glm::vec4 baseColor{1.0f};

    // Raio aproximado da malha, calculado a partir dos vértices.
    float boundingRadius = 0.0f;

    BoundingBox boundingBox;

    // Calcula quantos números compõem cada vértice.
    static int computeStride(
        int componentCount,
        const std::vector<int> &layout);

    static void setupAttributes(
        int componentCount,
        const std::vector<int> &layout);

    // Configura como os campos de Vertex são enviados ao OpenGL.
    static void setupVertexAttributes();

    unsigned int extrusionResultFaceIndex = 0;

public:
    Mesh() = default;
    ~Mesh() { destroy(); }

    Mesh(const Mesh &) = delete;
    Mesh &operator=(const Mesh &) = delete;

    void addTexture(Texture &&texture);
    void setBaseColor(const glm::vec4 &color) { baseColor = color; }

    const std::vector<Texture> &getTextures() const;

    const std::vector<Face> &getFaces() const
    {
        return faces;
    }

    // Recupera as posições dos três vértices de uma face triangular.
    bool getFacePositions(
        unsigned int faceIndex,
        glm::vec3 &a,
        glm::vec3 &b,
        glm::vec3 &c) const;

    // Inicia uma operação de extrusão.
    bool beginExtrusion(unsigned int faceIndex);

    // Atualiza a distância da extrusão.
    bool updateExtrusion(float distance);

    // Finaliza e confirma a extrusão.
    void finishExtrusion();

    // Cancela e restaura a geometria original.
    void cancelExtrusion();

    // Extruda uma face na direção de sua normal.
    bool extrudeFace(
        unsigned int faceIndex,
        float distance);

    // Identifica a face atingida por um raio no espaço local da malha.
    bool intersectRay(
        const glm::vec3 &rayOrigin,
        const glm::vec3 &rayDirection,
        unsigned int &hitFaceIndex,
        float &hitDistance) const;

    // Em layout, cada número informa quantos valores formam um atributo do vértice.
    // Exemplo: {3, 3, 2, 3} representa posição, cor, coordenadas da textura e normal.
    // Cria uma malha a partir de números, sem reutilizar vértices por índice.
    bool create(
        const std::vector<float> &vertices,
        int componentCount = 3,
        GLenum drawMode = GL_TRIANGLES,
        const std::vector<int> &layout = {});

    // Cria uma malha a partir de números e índices de vértice.
    bool createIndexed(
        const std::vector<float> &vertices,
        const std::vector<unsigned int> &indices,
        int componentCount = 3,
        GLenum drawMode = GL_TRIANGLES,
        const std::vector<int> &layout = {});

    // Cria uma malha a partir de vértices estruturados, sem índices.
    bool create(
        const std::vector<Vertex> &vertices,
        GLenum drawMode = GL_TRIANGLES);

    // Cria uma malha a partir de vértices estruturados e índices.
    bool createIndexed(
        const std::vector<Vertex> &vertices,
        const std::vector<unsigned int> &indices,
        GLenum drawMode = GL_TRIANGLES);

    void draw() const;

    // Associa as texturas e desenha a malha; usa diffuseFallback se não houver textura difusa.
    void draw(Shader &shader, const Texture *diffuseFallback = nullptr) const;

    void destroy();

    const std::vector<float> &getVertexData() const;

    const std::vector<Vertex> &getStructuredVertices() const;

    const std::vector<unsigned int> &getIndexData() const;

    float getBoundingRadius() const { return boundingRadius; }

    const BoundingBox &getBoundingBox() const
    {
        return boundingBox;
    }

    unsigned int getExtrudedFaceIndex() const
{
    return extrusionResultFaceIndex;
}
};
