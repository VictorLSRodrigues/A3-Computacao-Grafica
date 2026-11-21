#include "scene.h"

#include <iostream>
#include <utility>
#include <algorithm>
#include <cmath>
#include <glm/gtx/norm.hpp>
#include <limits>
#include <glm/gtc/matrix_inverse.hpp>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <nlohmann/json.hpp>

namespace
{
    using json = nlohmann::json;

    // Converte glm::vec3 para uma lista JSON.
    json vectorToJson(const glm::vec3& value)
    {
        return json::array({
            value.x,
            value.y,
            value.z
        });
    }

    // Recupera um glm::vec3 de uma lista JSON.
    bool jsonToVector(const json& value, glm::vec3& result)
    {
        if (!value.is_array() || value.size() != 3)
            return false;

        for (const auto& component : value)
        {
            if (!component.is_number())
                return false;
        }

        const glm::vec3 parsed(
            value[0].get<float>(),
            value[1].get<float>(),
            value[2].get<float>()
        );

        if (!std::isfinite(parsed.x) ||
            !std::isfinite(parsed.y) ||
            !std::isfinite(parsed.z))
        {
            return false;
        }

        result = parsed;
        return true;
    }
}

Scene::Object *Scene::addModel(const std::string &path, const Transform &transform)
{
    std::shared_ptr<Model> model;

    auto cached = modelCache.find(path);
    if (cached != modelCache.end())
    {
        model = cached->second;
    }
    else
    {
        try
        {
            model = std::make_shared<Model>(path.c_str());
        }
        catch (const std::exception &error)
        {
            std::cout << "Falha ao carregar o modelo " << path << ": "
                      << error.what() << std::endl;
            return nullptr;
        }

        modelCache[path] = model;
    }

    objects.push_back({path, model, nullptr, transform, {}});
    return &objects.back();
}

bool Scene::showOnly(const std::string &name)
{
    bool found = false;

    for (const Object &object : objects)
        found = found || object.name == name;

    if (!found)
        return false;

    for (Object &object : objects)
        object.visible = object.name == name;
    return true;
}

Scene::Object *Scene::addMesh(
    std::shared_ptr<Mesh> mesh,
    const std::string &name,
    const Transform &transform)
{
    if (!mesh)
        return nullptr;

    objects.push_back({name, nullptr, std::move(mesh), transform, {}});
    return &objects.back();
}

void Scene::update(float deltaTime)
{
    for (Object &object : objects)
    {
        if (object.update)
            object.update(object.transform, deltaTime);
    }
}

void Scene::showAll()
{
    for (Object &object : objects)
        object.visible = true;
}

// Verifica qual objeto é atingido pelo raio lançado pela câmera.
Scene::Object *Scene::pickObject(
    const glm::vec3 &rayOrigin,
    const glm::vec3 &rayDirection)
{
    Object *nearestObject = nullptr;

    if (glm::length2(rayDirection) < 0.000001f)
        return nullptr;

    const glm::vec3 direction = glm::normalize(rayDirection);

    float nearestDistance = std::numeric_limits<float>::max();

    for (Object &object : objects)
    {
        // Objetos ocultos não podem ser selecionados.
        if (!object.visible)
            continue;

        float localRadius = 0.0f;

        if (object.model)
            localRadius = object.model->getBoundingRadius();
        else if (object.mesh)
            localRadius = object.mesh->getGeometry().getBoundingRadius();

        if (localRadius <= 0.0f)
            continue;

        // Considera a maior escala aplicada ao objeto.
        const glm::vec3 scale = glm::abs(object.transform.scale());

        const float maxScale = std::max(
            scale.x,
            std::max(scale.y, scale.z));

        const float worldRadius = localRadius * maxScale;

        // Teste de interseção entre o raio e a esfera do objeto.
        const glm::vec3 offset =
            rayOrigin - object.transform.position();

        const float b = glm::dot(offset, direction);

        const float c = glm::dot(offset, offset) -
                        worldRadius * worldRadius;

        const float discriminant = b * b - c;

        if (discriminant < 0.0f)
            continue;

        const float sqrtDiscriminant = std::sqrt(discriminant);

        float distance = -b - sqrtDiscriminant;

        if (distance < 0.0f)
            distance = -b + sqrtDiscriminant;

        // Guarda apenas o objeto atingido mais próximo.
        if (distance >= 0.0f && distance < nearestDistance)
        {
            nearestDistance = distance;
            nearestObject = &object;
        }
    }

    return nearestObject;
}

// Procura a face atingida pelo raio do mouse.
bool Scene::pickFace(
    const glm::vec3 &rayOrigin,
    const glm::vec3 &rayDirection,
    Object *&hitObject,
    FaceSelection &hitFace)
{
    // Sem acerto, o resultado é "nenhum objeto, face inválida".
    hitObject = nullptr;
    hitFace.valid = false;

    const float directionLength = glm::length(rayDirection);

    if (directionLength < 0.000001f)
        return false;

    const glm::vec3 worldDirection =
        rayDirection / directionLength;

    float nearestDistance =
        std::numeric_limits<float>::max();

    Object *nearestObject = nullptr;
    FaceSelection nearestSelection;

    // Percorre todos os objetos da cena.
    for (Object &object : objects)
    {
        if (!object.visible)
            continue;

        const glm::mat4 objectMatrix =
            object.transform.matrix();

        // Caso 1: objeto importado como Model.
        if (object.model)
        {
            std::size_t meshIndex = 0;
            unsigned int faceIndex = 0;
            float distance = 0.0f;

            if (!object.model->intersectRay(
                    rayOrigin,
                    worldDirection,
                    objectMatrix,
                    meshIndex,
                    faceIndex,
                    distance))
            {
                continue;
            }

            if (distance < nearestDistance)
            {
                nearestDistance = distance;
                nearestObject = &object;

                nearestSelection.meshIndex = meshIndex;
                nearestSelection.faceIndex = faceIndex;
                nearestSelection.valid = true;
            }
        }

        // Caso 2: objeto criado diretamente como Mesh.
        else if (object.mesh)
        {
            const float determinant =
                glm::determinant(objectMatrix);

            if (std::abs(determinant) < 0.000001f)
                continue;

            const glm::mat4 inverseMatrix =
                glm::inverse(objectMatrix);

            // Transforma o raio do espaço global para o espaço local.
            const glm::vec3 localOrigin = glm::vec3(
                inverseMatrix * glm::vec4(rayOrigin, 1.0f));

            glm::vec3 localDirection = glm::vec3(
                inverseMatrix * glm::vec4(worldDirection, 0.0f));

            const float localDirectionLength =
                glm::length(localDirection);

            if (localDirectionLength < 0.000001f)
                continue;

            localDirection /= localDirectionLength;

            unsigned int faceIndex = 0;
            float localDistance = 0.0f;

            if (!object.mesh->getGeometry().intersectRay(
                    localOrigin,
                    localDirection,
                    faceIndex,
                    localDistance))
            {
                continue;
            }

            // Recupera o ponto atingido no espaço global.
            const glm::vec3 localHitPosition =
                localOrigin + localDirection * localDistance;

            const glm::vec3 worldHitPosition = glm::vec3(
                objectMatrix * glm::vec4(localHitPosition, 1.0f));

            const float worldDistance = glm::dot(
                worldHitPosition - rayOrigin,
                worldDirection);

            if (worldDistance < 0.0f)
                continue;

            if (worldDistance < nearestDistance)
            {
                nearestDistance = worldDistance;
                nearestObject = &object;

                nearestSelection.meshIndex = 0;
                nearestSelection.faceIndex = faceIndex;
                nearestSelection.valid = true;
            }
        }
    }

    // Nenhuma face foi atingida.
    if (!nearestObject)
        return false;

    hitObject = nearestObject;
    hitFace = nearestSelection;

    return true;
}

// Recupera os vértices globais de uma face do objeto.
bool Scene::getFacePositions(
    const Object *object,
    const FaceSelection &face,
    glm::vec3& a,
    glm::vec3& b,
    glm::vec3& c) const
{
    if (!object || !face.valid)
        return false;

    const glm::mat4 objectMatrix =
        object->transform.matrix();

    // Modelo importado.
    if (object->model)
    {
        return object->model->getFacePositions(
            face.meshIndex,
            face.faceIndex,
            objectMatrix,
            a, b, c
        );
    }

    // Malha criada diretamente na cena.
    if (object->mesh)
    {
        if (face.meshIndex != 0)
            return false;

        if (!object->mesh->getGeometry().getFacePositions(
                face.faceIndex,
                a, b, c))
            return false;

        a = glm::vec3(objectMatrix * glm::vec4(a, 1.0f));
        b = glm::vec3(objectMatrix * glm::vec4(b, 1.0f));
        c = glm::vec3(objectMatrix * glm::vec4(c, 1.0f));

        return true;
    }

    return false;
}


bool Scene::beginFaceExtrusion(Object *object, const FaceSelection &face)
{
    if (!object || !face.valid)
        return false;

    // Modelo importado.
    if (object->model)
    {
        return object->model->beginExtrusion(
            face.meshIndex,
            face.faceIndex
        );
    }

    // Malha criada diretamente na cena.
    if (object->mesh)
    {
        if (face.meshIndex != 0)
            return false;

        return object->mesh->beginExtrusion(
            face.faceIndex
        );
    }

    return false;
}

bool Scene::updateFaceExtrusion(Object *object, const FaceSelection &face, float distance)
{
    if (!object || !face.valid)
        return false;

    if (object->model)
    {
        return object->model->updateExtrusion(
            face.meshIndex,
            distance
        );
    }

    if (object->mesh)
    {
        return object->mesh->updateExtrusion(distance);
    }

    return false;
}

void Scene::finishFaceExtrusion(Object *object, FaceSelection &face)
{
    if (!object || !face.valid)
        return;

    // Obtém o índice da nova tampa antes de finalizar.
    if (object->model)
    {
        const unsigned int newFace =
            object->model->getExtrudedFaceIndex(
                face.meshIndex
            );

        object->model->finishExtrusion(
            face.meshIndex
        );

        face.faceIndex = newFace;
    }
    else if (object->mesh)
    {
        const unsigned int newFace =
            object->mesh->getGeometry().getExtrudedFaceIndex();

        object->mesh->finishExtrusion();

        face.faceIndex = newFace;
    }

    face.valid = true;
}

void Scene::cancelFaceExtrusion(Object *object, const FaceSelection &face)
{
    if (!object || !face.valid)
        return;

    if (object->model)
    {
        object->model->cancelExtrusion(
            face.meshIndex
        );
    }
    else if (object->mesh)
    {
        object->mesh->cancelExtrusion();
    }
}

bool Scene::saveTransforms(const std::string& filePath) const
{
    try
    {
        const std::filesystem::path path(filePath);

        if (path.has_parent_path())
            std::filesystem::create_directories(path.parent_path());

        json data;

        data["version"] = 1;
        data["objects"] = json::array();

        for (const Object& object : objects)
        {
            if (object.name.empty())
                continue;

            data["objects"].push_back({
                {"name", object.name},
                {"position", vectorToJson(object.transform.position())},
                {"rotation", vectorToJson(object.transform.rotation())},
                {"scale", vectorToJson(object.transform.scale())}
            });
        }

        std::ofstream output(
            path,
            std::ios::out | std::ios::trunc
        );

        if (!output.is_open())
        {
            std::cerr << "Falha ao abrir arquivo de estado para salvar: "
                      << filePath << std::endl;
            return false;
        }

        output << std::setw(4) << data << '\n';

        return output.good();
    }
    catch (const std::exception& error)
    {
        std::cerr << "Erro ao salvar transformacoes: "
                  << error.what() << std::endl;
        return false;
    }
}

bool Scene::loadTransforms(const std::string& filePath)
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
            data["version"].get<int>() != 1 ||
            !data.contains("objects") ||
            !data["objects"].is_array())
        {
            std::cerr << "Arquivo de transformacoes invalido."
                      << std::endl;
            return false;
        }

        std::size_t restoredCount = 0;

        for (const auto& savedObject : data["objects"])
        {
            if (!savedObject.is_object() ||
                !savedObject.contains("name") ||
                !savedObject["name"].is_string() ||
                !savedObject.contains("position") ||
                !savedObject.contains("rotation") ||
                !savedObject.contains("scale"))
            {
                continue;
            }

            const std::string name =
                savedObject["name"].get<std::string>();

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

            // Procura a instância correspondente na cena atual.
            auto object = std::find_if(
                objects.begin(),
                objects.end(),
                [&name](const Object& candidate)
                {
                    return candidate.name == name;
                }
            );

            if (object == objects.end())
                continue;

            // Restaura o estado através da interface do Transform.
            object->transform.move(position);
            object->transform.setRotation(rotation);
            object->transform.setScale(scale);

            ++restoredCount;
        }

        std::cout << "Transformacoes restauradas: "
                  << restoredCount << std::endl;

        return restoredCount > 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Erro ao carregar transformacoes: "
                  << error.what() << std::endl;
        return false;
    }
}
