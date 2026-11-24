#include "scene.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <nlohmann/json.hpp>
#include <utility>
#include <vector>

namespace
{
    using json = nlohmann::json;

    // Converte glm::vec3 para uma lista JSON.
    json vectorToJson(const glm::vec3 &value)
    {
        return json::array({value.x, value.y, value.z});
    }

    // Recupera um glm::vec3 de uma lista JSON.
    bool jsonToVector(const json &value, glm::vec3 &result)
    {
        if (!value.is_array() || value.size() != 3)
            return false;

        for (const auto &component : value)
        {
            if (!component.is_number())
                return false;
        }

        const glm::vec3 parsed(
            value[0].get<float>(),
            value[1].get<float>(),
            value[2].get<float>());

        if (!std::isfinite(parsed.x) ||
            !std::isfinite(parsed.y) ||
            !std::isfinite(parsed.z))
            return false;

        result = parsed;
        return true;
    }

    json vec2ToJson(const glm::vec2 &value)
    {
        return json::array({value.x, value.y});
    }

    bool jsonToVec2(const json &value, glm::vec2 &result)
    {
        if (!value.is_array() || value.size() != 2 ||
            !value[0].is_number() || !value[1].is_number())
            return false;

        const glm::vec2 parsed(
            value[0].get<float>(),
            value[1].get<float>());

        if (!std::isfinite(parsed.x) || !std::isfinite(parsed.y))
            return false;

        result = parsed;
        return true;
    }

    json vec4ToJson(const glm::vec4 &value)
    {
        return json::array({value.x, value.y, value.z, value.w});
    }

    bool jsonToVec4(const json &value, glm::vec4 &result)
    {
        if (!value.is_array() || value.size() != 4)
            return false;

        for (const auto &component : value)
        {
            if (!component.is_number())
                return false;
        }

        const glm::vec4 parsed(
            value[0].get<float>(),
            value[1].get<float>(),
            value[2].get<float>(),
            value[3].get<float>());

        if (!std::isfinite(parsed.x) ||
            !std::isfinite(parsed.y) ||
            !std::isfinite(parsed.z) ||
            !std::isfinite(parsed.w))
            return false;

        result = parsed;
        return true;
    }

    // Converte e reconstrói todos os atributos de um Vertex.
    json vertexToJson(const Vertex &vertex)
    {
        return {
            {"position", vectorToJson(vertex.position)},
            {"color", vectorToJson(vertex.color)},
            {"texUV", vec2ToJson(vertex.texUV)},
            {"normal", vectorToJson(vertex.normal)},
            {"tangent", vec4ToJson(vertex.tangent)}};
    }

    bool jsonToVertex(const json &value, Vertex &result)
    {
        if (!value.is_object() ||
            !value.contains("position") ||
            !value.contains("color") ||
            !value.contains("texUV") ||
            !value.contains("normal") ||
            !value.contains("tangent"))
            return false;

        Vertex parsed{};

        if (!jsonToVector(value["position"], parsed.position) ||
            !jsonToVector(value["color"], parsed.color) ||
            !jsonToVec2(value["texUV"], parsed.texUV) ||
            !jsonToVector(value["normal"], parsed.normal) ||
            !jsonToVec4(value["tangent"], parsed.tangent))
            return false;

        result = parsed;
        return true;
    }

    bool jsonToSize(const json &value, std::size_t &result)
    {
        if (!value.is_number_integer() && !value.is_number_unsigned())
            return false;

        if (value.is_number_unsigned())
        {
            const std::uint64_t parsed = value.get<std::uint64_t>();
            if (parsed > static_cast<std::uint64_t>(
                             std::numeric_limits<std::size_t>::max()))
                return false;

            result = static_cast<std::size_t>(parsed);
            return true;
        }

        const std::int64_t parsed = value.get<std::int64_t>();
        if (parsed < 0 || static_cast<std::uint64_t>(parsed) >
                              static_cast<std::uint64_t>(
                                  std::numeric_limits<std::size_t>::max()))
            return false;

        result = static_cast<std::size_t>(parsed);
        return true;
    }

    std::string modelFileName(const std::string &path)
    {
        std::string portablePath = path;
        std::replace(portablePath.begin(), portablePath.end(), '\\', '/');
        return std::filesystem::path(portablePath).filename().string();
    }

    // Gera o registro JSON de uma geometria que já recebeu extrusão.
    bool appendExtrusionJson(
        json &extrusions,
        const Geometry &geometry,
        std::size_t meshIndex)
    {
        if (!geometry.hasExtrusion())
            return false;

        const auto &vertices = geometry.getStructuredVertices();
        const auto &indices = geometry.getIndexData();

        if (vertices.empty() || indices.empty() || indices.size() % 3 != 0)
            return false;

        json savedVertices = json::array();
        json savedIndices = json::array();

        for (const Vertex &vertex : vertices)
            savedVertices.push_back(vertexToJson(vertex));

        for (unsigned int index : indices)
        {
            if (index >= vertices.size())
                return false;
            savedIndices.push_back(index);
        }

        extrusions.push_back({{"mesh_index", meshIndex},
                              {"vertices", std::move(savedVertices)},
                              {"indices", std::move(savedIndices)}});

        return true;
    }

    // Lê e valida um registro de extrusão antes de alterar a malha.
    bool jsonToExtrusion(
        const json &value,
        std::size_t &meshIndex,
        std::vector<Vertex> &vertices,
        std::vector<unsigned int> &indices)
    {
        if (!value.is_object() ||
            !value.contains("mesh_index") ||
            !value.contains("vertices") ||
            !value.contains("indices") ||
            !value["vertices"].is_array() ||
            !value["indices"].is_array())
            return false;

        if (!jsonToSize(value["mesh_index"], meshIndex))
            return false;

        const json &savedVertices = value["vertices"];
        const json &savedIndices = value["indices"];

        if (savedVertices.empty() || savedIndices.empty() ||
            savedIndices.size() % 3 != 0)
            return false;

        std::vector<Vertex> parsedVertices;
        parsedVertices.reserve(savedVertices.size());

        for (const auto &savedVertex : savedVertices)
        {
            Vertex vertex{};
            if (!jsonToVertex(savedVertex, vertex))
                return false;
            parsedVertices.push_back(vertex);
        }

        std::vector<unsigned int> parsedIndices;
        parsedIndices.reserve(savedIndices.size());

        for (const auto &savedIndex : savedIndices)
        {
            std::size_t index = 0;
            if (!jsonToSize(savedIndex, index) ||
                index >= parsedVertices.size() ||
                index > std::numeric_limits<unsigned int>::max())
                return false;

            parsedIndices.push_back(static_cast<unsigned int>(index));
        }

        vertices = std::move(parsedVertices);
        indices = std::move(parsedIndices);
        return true;
    }
}

bool Scene::saveTransforms(const std::string &filePath) const
{
    try
    {
        const std::filesystem::path path(filePath);

        if (path.has_parent_path())
            std::filesystem::create_directories(path.parent_path());

        json data;
        data["version"] = 3;
        data["objects"] = json::array();

        for (std::size_t objectIndex = 0; objectIndex < objects.size(); ++objectIndex)
        {
            const Object &object = objects[objectIndex];

            if (object.name.empty())
                continue;

            json savedObject = {
                {"object_index", objectIndex},
                {"name", object.name},
                {"visible", object.visible},
                {"position", vectorToJson(object.transform.position())},
                {"rotation", vectorToJson(object.transform.rotation())},
                {"scale", vectorToJson(object.transform.scale())}};

            // A origem permite recriar modelos adicionados depois da inicializacao.
            if (object.model && !object.sourcePath.empty())
                savedObject["source_path"] = object.sourcePath;

            json extrusions = json::array();

            if (object.model)
            {
                for (std::size_t meshIndex = 0;
                     meshIndex < object.model->getMeshCount();
                     ++meshIndex)
                {
                    const Geometry *geometry =
                        object.model->getMeshGeometry(meshIndex);

                    if (geometry)
                        appendExtrusionJson(extrusions, *geometry, meshIndex);
                }
            }
            else if (object.mesh)
            {
                appendExtrusionJson(
                    extrusions,
                    object.mesh->getGeometry(),
                    0);
            }

            if (!extrusions.empty())
                savedObject["extrusions"] = std::move(extrusions);

            data["objects"].push_back(std::move(savedObject));
        }

        std::ofstream output(
            path,
            std::ios::out | std::ios::trunc);

        if (!output.is_open())
        {
            std::cerr << "Falha ao abrir arquivo de estado para salvar: "
                      << filePath << std::endl;
            return false;
        }

        output << std::setw(4) << data << '\n';
        return output.good();
    }
    catch (const std::exception &error)
    {
        std::cerr << "Erro ao salvar estado da cena: "
                  << error.what() << std::endl;
        return false;
    }
}

// Persiste a fonte de luz junto com os objetos da cena. Mantem o formato
// principal de objetos e acrescenta um bloco independente chamado "light".
bool Scene::saveTransforms(const std::string &filePath, const Light &light) const
{
    if (!saveTransforms(filePath))
        return false;

    try
    {
        std::ifstream input(filePath);
        if (!input.is_open())
            return false;

        json data;
        input >> data;
        input.close();

        data["light"] = {
            {"type", static_cast<int>(light.type)},
            {"color", vectorToJson(light.color)},
            {"intensity", light.intensity},
            {"position", vectorToJson(light.position)},
            {"target", vectorToJson(light.target)}};

        const std::filesystem::path path(filePath);
        const std::filesystem::path temporaryPath = path.string() + ".tmp";
        std::ofstream output(temporaryPath, std::ios::out | std::ios::trunc);
        if (!output.is_open())
            return false;

        output << std::setw(4) << data << '\n';
        output.close();
        if (!output.good())
            return false;

        std::error_code error;
        std::filesystem::rename(temporaryPath, path, error);
        if (error)
        {
            // No Windows, rename nao substitui um destino existente.
            std::filesystem::remove(path, error);
            error.clear();
            std::filesystem::rename(temporaryPath, path, error);
        }
        if (error)
        {
            std::filesystem::remove(temporaryPath);
            std::cerr << "Falha ao finalizar arquivo de estado da luz: "
                      << error.message() << std::endl;
            return false;
        }
        return true;
    }
    catch (const std::exception &error)
    {
        std::cerr << "Erro ao salvar fonte de luz: " << error.what() << std::endl;
        return false;
    }
}

bool Scene::loadTransforms(const std::string &filePath)
{
    try
    {
        std::ifstream input(filePath);

        // Na primeira execução, ainda não existe arquivo salvo.
        if (!input.is_open())
            return false;

        json data;
        input >> data;

        if (!data.is_object() ||
            !data.contains("version") ||
            !data["version"].is_number_integer() ||
            !data.contains("objects") ||
            !data["objects"].is_array())
        {
            std::cerr << "Arquivo de estado da cena invalido." << std::endl;
            return false;
        }

        const int version = data["version"].get<int>();
        if (version != 1 && version != 2 && version != 3)
        {
            std::cerr << "Versao do arquivo de estado nao suportada: "
                      << version << std::endl;
            return false;
        }

        std::size_t restoredCount = 0;
        std::size_t restoredExtrusionCount = 0;

        // Impede que dois registros do JSON sejam associados
        // à mesma instância da cena.
        std::vector<bool> matchedObjects(objects.size(), false);

        for (const auto &savedObject : data["objects"])
        {
            if (!savedObject.is_object() ||
                !savedObject.contains("name") ||
                !savedObject["name"].is_string() ||
                !savedObject.contains("position") ||
                !savedObject.contains("rotation") ||
                !savedObject.contains("scale"))
                continue;

            const std::string name = savedObject["name"].get<std::string>();

            glm::vec3 position;
            glm::vec3 rotation;
            glm::vec3 scale;

            if (!jsonToVector(savedObject["position"], position) ||
                !jsonToVector(savedObject["rotation"], rotation) ||
                !jsonToVector(savedObject["scale"], scale))
            {
                std::cerr << "Transformacao invalida para: "
                          << name << std::endl;
                continue;
            }

            // O indice identifica instancias repetidas; source_path permite
            // reconstruir modelos que nao existiam quando a cena foi iniciada.
            Object *targetObject = nullptr;
            std::size_t targetObjectIndex = objects.size();
            std::size_t objectIndex = objects.size();

            if (savedObject.contains("object_index"))
                jsonToSize(savedObject["object_index"], objectIndex);

            const std::string sourcePath =
                (version >= 3 && savedObject.contains("source_path") &&
                 savedObject["source_path"].is_string())
                    ? savedObject["source_path"].get<std::string>()
                    : std::string{};

            // Arquivos antigos nao possuem source_path: mantem o fallback por nome.
            // Quando source_path existe, tambem confirma o arquivo de origem.
            const auto matchesSavedObject =
                [&name, &sourcePath](const Object &candidate) -> bool
            {
                if (candidate.name != name)
                    return false;

                if (sourcePath.empty())
                    return true;

                if (!candidate.model || candidate.sourcePath.empty())
                    return false;

                return modelFileName(candidate.sourcePath) ==
                       modelFileName(sourcePath);
            };

            // Primeiro tenta o indice salvo, desde que ele ainda identifique
            // a mesma instancia e ainda nao tenha sido utilizado neste arquivo.
            if (objectIndex < objects.size() &&
                objectIndex < matchedObjects.size() &&
                !matchedObjects[objectIndex] &&
                matchesSavedObject(objects[objectIndex]))
            {
                targetObject = &objects[objectIndex];
                targetObjectIndex = objectIndex;
            }

            // Se o indice mudou, procura uma instancia existente pelo nome
            // e pelo arquivo de origem, sem reutilizar uma instancia ja associada.
            if (!targetObject)
            {
                for (std::size_t i = 0; i < objects.size(); ++i)
                {
                    if (i < matchedObjects.size() && matchedObjects[i])
                        continue;

                    if (!matchesSavedObject(objects[i]))
                        continue;

                    targetObject = &objects[i];
                    targetObjectIndex = i;
                    break;
                }
            }

            // Instancia dinamica de modelo: carrega o recurso e preserva o nome
            // personalizado da instancia gravada no arquivo. Caminhos absolutos
            // de outros computadores sao resolvidos contra os assets do projeto.
            if (!targetObject && !sourcePath.empty())
            {
                std::string resolvedSourcePath = sourcePath;
                std::error_code fileError;

                if (!std::filesystem::is_regular_file(
                        resolvedSourcePath, fileError) ||
                    fileError)
                {
                    fileError.clear();
                    const std::filesystem::path assetPath =
                        std::filesystem::path(
                            std::string(ASSETS_DIR) + "sandbox/models") /
                        modelFileName(sourcePath);

                    if (std::filesystem::is_regular_file(
                            assetPath, fileError) &&
                        !fileError)
                    {
                        resolvedSourcePath = assetPath.string();
                    }
                }

                targetObject = addModel(resolvedSourcePath);

                if (targetObject)
                {
                    targetObject->name = name;
                    targetObjectIndex = objects.size() - 1;
                }
            }

            if (!targetObject)
                continue;

            // Cada registro do JSON deve ser aplicado a uma instancia diferente.
            if (matchedObjects.size() < objects.size())
                matchedObjects.resize(objects.size(), false);

            matchedObjects[targetObjectIndex] = true;

            if (version >= 3 && savedObject.contains("visible") &&
                savedObject["visible"].is_boolean())
                targetObject->visible = savedObject["visible"].get<bool>();

            targetObject->transform.move(position);
            targetObject->transform.setRotation(rotation);
            targetObject->transform.setScale(scale);
            ++restoredCount;

            if (version < 2 ||
                !savedObject.contains("extrusions") ||
                !savedObject["extrusions"].is_array())
                continue;

            bool objectRestoredExtrusion = false;
            std::shared_ptr<Model> workingModel;
            std::shared_ptr<Mesh> workingMesh;
            bool meshCopyAttempted = false;

            for (const auto &savedExtrusion : savedObject["extrusions"])
            {
                std::size_t meshIndex = 0;
                std::vector<Vertex> vertices;
                std::vector<unsigned int> indices;

                if (!jsonToExtrusion(
                        savedExtrusion,
                        meshIndex,
                        vertices,
                        indices))
                    continue;

                if (targetObject->model)
                {
                    if (!workingModel)
                    {
                        workingModel = targetObject->model;

                        // Se ainda for o modelo do cache, separa esta instância.
                        bool usesCachedModel = false;
                        for (const auto &cached : modelCache)
                        {
                            if (cached.second == targetObject->model)
                            {
                                usesCachedModel = true;
                                break;
                            }
                        }

                        if (usesCachedModel)
                        {
                            workingModel = targetObject->model->clone();
                            if (!workingModel)
                            {
                                std::cerr << "Falha ao copiar modelo para restaurar extrusao: "
                                          << name << std::endl;
                                continue;
                            }
                        }
                    }

                    if (workingModel->restoreExtrudedGeometry(
                            meshIndex,
                            vertices,
                            indices))
                    {
                        targetObject->model = workingModel;
                        objectRestoredExtrusion = true;
                    }
                }
                else if (targetObject->mesh && meshIndex == 0)
                {
                    if (!workingMesh && !meshCopyAttempted)
                    {
                        meshCopyAttempted = true;
                        workingMesh = std::make_shared<Mesh>();

                        if (!workingMesh->copyFrom(*targetObject->mesh))
                        {
                            std::cerr << "Falha ao copiar malha para restaurar extrusao: "
                                      << name << std::endl;
                            workingMesh.reset();
                        }
                    }

                    if (workingMesh &&
                        workingMesh->restoreExtrudedGeometry(vertices, indices))
                    {
                        targetObject->mesh = workingMesh;
                        objectRestoredExtrusion = true;
                    }
                }
            }

            if (objectRestoredExtrusion)
                ++restoredExtrusionCount;
        }

        std::cout << "Transformacoes restauradas: "
                  << restoredCount << std::endl;
        std::cout << "Objetos com geometria extrudada restaurada: "
                  << restoredExtrusionCount << std::endl;

        return restoredCount > 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "Erro ao carregar estado da cena: "
                  << error.what() << std::endl;
        return false;
    }
}

// Carrega a luz somente quando o bloco existe e todos os campos sao validos.
// Arquivos antigos sem "light" continuam funcionando e preservam a luz padrao.
bool Scene::loadTransforms(const std::string &filePath, Light &light)
{
    try
    {
        std::ifstream input(filePath);
        if (!input.is_open())
            return false;

        json data;
        input >> data;
        input.close();

        if (!loadTransforms(filePath))
            return false;

        if (!data.is_object() || !data.contains("light") ||
            !data["light"].is_object())
            return true;

        const json &savedLight = data["light"];
        if (!savedLight.contains("type") ||
            !savedLight["type"].is_number_integer() ||
            !savedLight.contains("color") ||
            !savedLight.contains("intensity") ||
            !savedLight["intensity"].is_number() ||
            !savedLight.contains("position") ||
            !savedLight.contains("target"))
            return true;

        const int type = savedLight["type"].get<int>();
        glm::vec3 color, position, target;
        const float intensity = savedLight["intensity"].get<float>();
        if (type < static_cast<int>(LightType::Point) ||
            type > static_cast<int>(LightType::Spot) ||
            !std::isfinite(intensity) || intensity < 0.0f ||
            !jsonToVector(savedLight["color"], color) ||
            !jsonToVector(savedLight["position"], position) ||
            !jsonToVector(savedLight["target"], target))
        {
            std::cerr << "Dados da fonte de luz invalidos; mantendo valores atuais."
                      << std::endl;
            return true;
        }

        if (glm::any(glm::lessThan(color, glm::vec3(0.0f))) ||
            glm::any(glm::greaterThan(color, glm::vec3(1.0f))))
            return true;

        light.type = static_cast<LightType>(type);
        light.color = color;
        light.intensity = intensity;
        light.position = position;
        light.target = target;
        return true;
    }
    catch (const std::exception &error)
    {
        std::cerr << "Erro ao carregar fonte de luz: " << error.what() << std::endl;
        return false;
    }
}