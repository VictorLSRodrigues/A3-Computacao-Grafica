#pragma once

#include <cstddef>
#include <vector>

#include <glm/glm.hpp>

#include "bounding_box.h"
#include "face.h"
#include "vertex.h"

// Como os vértices se ligam para formar as primitivas.
enum class PrimitiveTopology
{
    Points,
    Lines,
    LineLoop,
    LineStrip,
    Triangles,
    TriangleStrip,
    TriangleFan
};

// Como os vértices estão guardados.
enum class VertexFormat
{
    None,      // Ainda sem dados.
    Raw,       // Lista de números, com atributos descritos por getAttributeSizes().
    Structured // Estrutura Vertex: posição, cor, UV, normal e tangente.
};

// Dados e informações espaciais de uma malha: vértices, índices, faces, limites e extrusão.
// Não depende de OpenGL: a Mesh é quem envia estes dados à GPU.
class Geometry
{
public:
    // Em layout, cada número informa quantos valores formam um atributo do vértice.
    // Exemplo: {3, 3, 2, 3} representa posição, cor, coordenadas da textura e normal.
    // Guarda os vértices como números, sem reutilizá-los por índice.
    bool create(
        const std::vector<float> &vertices,
        int componentCount = 3,
        PrimitiveTopology topology = PrimitiveTopology::Triangles,
        const std::vector<int> &layout = {});

    // Guarda os vértices como números e os triângulos como índices de vértice.
    bool createIndexed(
        const std::vector<float> &vertices,
        const std::vector<unsigned int> &indices,
        int componentCount = 3,
        PrimitiveTopology topology = PrimitiveTopology::Triangles,
        const std::vector<int> &layout = {});

    // Guarda vértices estruturados, sem índices.
    bool create(
        const std::vector<Vertex> &vertices,
        PrimitiveTopology topology = PrimitiveTopology::Triangles);

    // Guarda vértices estruturados e índices; reconstrói as faces e os limites.
    bool createIndexed(
        const std::vector<Vertex> &vertices,
        const std::vector<unsigned int> &indices,
        PrimitiveTopology topology = PrimitiveTopology::Triangles);

    // Descarta todos os dados.
    void clear();

    PrimitiveTopology getTopology() const { return topology; }
    VertexFormat getVertexFormat() const { return vertexFormat; }
    bool isIndexed() const { return !indexData.empty(); }

    // Quantidade de vértices, qualquer que seja o formato.
    std::size_t getVertexCount() const;

    // Quantos valores formam cada atributo, no formato Raw.
    const std::vector<int> &getAttributeSizes() const { return attributeSizes; }

    const std::vector<float> &getVertexData() const { return vertexData; }
    const std::vector<Vertex> &getStructuredVertices() const { return structuredVertices; }
    const std::vector<unsigned int> &getIndexData() const { return indexData; }

    // Topologia editável da geometria.
    const std::vector<Face> &getFaces() const { return faces; }

    // Raio aproximado, calculado a partir dos vértices.
    float getBoundingRadius() const { return boundingRadius; }

    const BoundingBox &getBoundingBox() const { return boundingBox; }

    // Recupera as posições dos três vértices de uma face triangular.
    bool getFacePositions(
        unsigned int faceIndex,
        glm::vec3 &a,
        glm::vec3 &b,
        glm::vec3 &c) const;

    // Identifica a face atingida por um raio no espaço local da geometria.
    bool intersectRay(
        const glm::vec3 &rayOrigin,
        const glm::vec3 &rayDirection,
        unsigned int &hitFaceIndex,
        float &hitDistance) const;

    // Extrusão: altera os dados da geometria; quem os usa na GPU precisa reenviá-los
    // depois de begin/update/cancel que retornarem true.

    // Inicia uma operação de extrusão.
    bool beginExtrusion(unsigned int faceIndex);

    // Atualiza a distância da extrusão.
    bool updateExtrusion(float distance);

    // Finaliza e confirma a extrusão.
    void finishExtrusion();

    // Cancela e restaura a geometria original; retorna true se restaurou.
    bool cancelExtrusion();

    unsigned int getExtrudedFaceIndex() const { return extrusionResultFaceIndex; }

    // Informa se a geometria possui extrusão confirmada.
    bool hasExtrusion() const
    {
        return extrusionApplied;
    }

    // Restaura uma geometria extrudada a partir dos dados salvos.
    bool restoreExtrudedGeometry(
        const std::vector<Vertex> &vertices,
        const std::vector<unsigned int> &indices);

private:
    PrimitiveTopology topology = PrimitiveTopology::Triangles;
    VertexFormat vertexFormat = VertexFormat::None;

    // Vértices em uma lista de números, com o formato descrito por attributeSizes.
    std::vector<float> vertexData;
    std::vector<int> attributeSizes;

    // Forma alternativa: cada vértice agrupa posição, cor, UV, normal e tangente.
    std::vector<Vertex> structuredVertices;

    std::vector<unsigned int> indexData;

    // Topologia editável.
    std::vector<Face> faces;

    // Raio aproximado e caixa envolvente, calculados a partir dos vértices.
    float boundingRadius = 0.0f;
    BoundingBox boundingBox;

    // Estado temporário da operação de extrusão.
    std::vector<Vertex> extrusionBaseVertices;
    std::vector<unsigned int> extrusionBaseIndices;
    std::vector<Face> extrusionBaseFaces;

    unsigned int extrusionFaceIndex = 0;
    bool extrusionInProgress = false;
    unsigned int extrusionResultFaceIndex = 0;

    bool extrusionApplied = false;
    bool extrusionBaseHadExtrusion = false;

    // Calcula quantos números compõem cada vértice.
    static int computeStride(
        int componentCount,
        const std::vector<int> &layout);
};
