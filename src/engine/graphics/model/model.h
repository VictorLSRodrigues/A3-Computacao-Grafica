#ifndef MODEL_CLASS_H
#define MODEL_CLASS_H

#include <cstddef>
#include <deque>
#include <vector>
#include <memory>

#include <glm/glm.hpp>

#include "engine/graphics/geometry/bounding_box.h"
#include "engine/graphics/mesh/mesh.h"

class Shader;
class Texture;

class Model
{
public:
    // Construtor para criar uma cópia independente.
    Model() = default;

    // Cria uma cópia independente do modelo.
    std::shared_ptr<Model> clone() const;

    // Carrega um modelo glTF (.gltf) ou GLB (.glb) e prepara suas partes para desenho.
    Model(const char *path);

    // objectMatrix é a transformação do objeto na cena; ela é combinada com a de cada parte do modelo.
    void draw(Shader &shader, const Texture &diffuseFallback,
              const glm::mat4 &objectMatrix = glm::mat4(1.0f));

    // Detecta a face atingida pelo raio em qualquer malha do modelo.
    bool intersectRay(
        const glm::vec3 &rayOrigin,
        const glm::vec3 &rayDirection,
        const glm::mat4 &objectMatrix,
        std::size_t &hitMeshIndex,
        unsigned int &hitFaceIndex,
        float &hitDistance) const;

    // Recupera os vértices de uma face no espaço global.
    bool getFacePositions(
        std::size_t meshIndex,
        unsigned int faceIndex,
        const glm::mat4 &objectMatrix,
        glm::vec3 &a,
        glm::vec3 &b,
        glm::vec3 &c) const;

    // Inicia a extrusão de uma face.
    bool beginExtrusion(
        std::size_t meshIndex,
        unsigned int faceIndex);

    // Atualiza a distância da extrusão.
    bool updateExtrusion(
        std::size_t meshIndex,
        float distance);

    // Confirma a extrusão.
    void finishExtrusion(std::size_t meshIndex);

    // Cancela a extrusão.
    void cancelExtrusion(std::size_t meshIndex);

    // Retorna o índice da nova face extrudada.
    unsigned int getExtrudedFaceIndex(
        std::size_t meshIndex) const;

    // Retorna a quantidade de malhas do modelo.
    std::size_t getMeshCount() const
    {
        return meshes.size();
    }

    // Permite consultar a geometria de uma malha específica.
    const Geometry *getMeshGeometry(std::size_t meshIndex) const
    {
        if (meshIndex >= meshes.size())
            return nullptr;

        return &meshes[meshIndex].getGeometry();
    }

    // Restaura a geometria extrudada de uma malha específica.
    bool restoreExtrudedGeometry(
        std::size_t meshIndex,
        const std::vector<Vertex> &vertices,
        const std::vector<unsigned int> &indices);

    // Total de faces de todas as partes do modelo.
    std::size_t getFaceCount() const
    {
        std::size_t total = 0;
        for (const Mesh &mesh : meshes)
            total += mesh.getGeometry().getFaces().size();
        return total;
    }

    // Retorna o raio aproximado utilizado na seleção do modelo.
    float getBoundingRadius() const { return boundingRadius; }

    const BoundingBox &getBoundingBox() const
    {
        return boundingBox;
    }

private:
    // Malhas e transformações internas prontas para renderização.
    std::deque<Mesh> meshes;
    std::vector<glm::mat4> matricesMeshes;
    float boundingRadius = 0.0f;
    BoundingBox boundingBox;

    // Recalcula os limites espaciais após modificar uma malha.
    void recalculateBounds();
};

#endif
