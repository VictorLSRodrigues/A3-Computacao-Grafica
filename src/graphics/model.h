#ifndef MODEL_CLASS_H
#define MODEL_CLASS_H

#include <deque>
#include <json/json.h>

#include "buffers/VAO.h"
#include "camera.h"
#include "mesh.h"
#include "texture.h"

using json = nlohmann::json;

class Model
{
public:
    // Carrega um modelo glTF (.gltf) ou GLB (.glb) e prepara suas partes para desenho.
    Model(const char *path);

    // objectMatrix é a transformação do objeto na cena; ela é combinada com a de cada parte do modelo.
    void draw(Shader &shader, Camera &camera, const Texture &diffuseFallback,
              const glm::mat4 &objectMatrix = glm::mat4(1.0f));

private:
    // Dados do arquivo e descrição de sua estrutura.
    const char *file;
    std::vector<unsigned char> data;
    json JSON;

    // Geometria encontrada no arquivo e transformações de cada parte.
    std::deque<Mesh> meshes;
    std::vector<glm::vec3> translationsMeshes;
    std::vector<glm::quat> rotationsMeshes;
    std::vector<glm::vec3> scalesMeshes;
    std::vector<glm::mat4> matricesMeshes;

    // Lê uma malha descrita no arquivo e prepara seus dados para desenho.
    void loadMesh(unsigned int meshIndex);

    // Percorre a hierarquia de partes do modelo e acumula suas transformações.
    void traverseNode(unsigned int nodeIndex, glm::mat4 parentTransform = glm::mat4(1.0f));

    // Lê os dados binários associados à descrição do modelo.
    std::vector<unsigned char> getData();
    // Converte referências do arquivo em dados de vértices, índices e imagens.
    std::vector<float> getFloats(json accessor);
    std::vector<GLuint> getIndices(json accessor);
    std::vector<Texture> getTextures();

    // Combina os atributos separados do arquivo em vértices completos.
    std::vector<Vertex> assembleVertices(
        std::vector<glm::vec3> positions,
        std::vector<glm::vec3> normals,
        std::vector<glm::vec2> texUVs);

    // Agrupa uma lista de números em vetores com 2, 3 ou 4 componentes.
    std::vector<glm::vec2> groupFloatsVec2(std::vector<float> values);
    std::vector<glm::vec3> groupFloatsVec3(std::vector<float> values);
    std::vector<glm::vec4> groupFloatsVec4(std::vector<float> values);
};
#endif