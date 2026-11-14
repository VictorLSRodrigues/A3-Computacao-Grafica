#include "scene.h"

#include <iostream>
#include <utility>
#include <algorithm>
#include <cmath>
#include <glm/gtx/norm.hpp>
#include <limits>
#include <glm/gtc/matrix_inverse.hpp>

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

    // Limpa a seleção caso o objeto selecionado fique oculto.
    if (selectedObject && !selectedObject->visible)
    {
        selectedObject = nullptr;
        selectedFace.valid = false;
    }
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
    selectedObject = nullptr;
    selectedFace.valid = false;

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
            localRadius = object.mesh->getBoundingRadius();

        if (localRadius <= 0.0f)
            continue;

        // Considera a maior escala aplicada ao objeto.
        const glm::vec3 scale = glm::abs(object.transform.scale);

        const float maxScale = std::max(
            scale.x,
            std::max(scale.y, scale.z));

        const float worldRadius = localRadius * maxScale;

        // Teste de interseção entre o raio e a esfera do objeto.
        const glm::vec3 offset =
            rayOrigin - object.transform.position;

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
            selectedObject = &object;
        }
    }

    return selectedObject;
}

// Seleciona a face atingida pelo raio do mouse.
bool Scene::pickFace(
    const glm::vec3 &rayOrigin,
    const glm::vec3 &rayDirection)
{
    // Limpa a seleção anterior.
    selectedObject = nullptr;
    selectedFace.valid = false;

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

            if (!object.mesh->intersectRay(
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

    // Guarda o objeto e a face selecionados.
    selectedObject = nearestObject;
    selectedFace = nearestSelection;

    return true;
}

// Recupera os vértices globais da face selecionada.
bool Scene::getSelectedFacePositions(
    glm::vec3& a,
    glm::vec3& b,
    glm::vec3& c) const
{
    if (!selectedObject || !selectedFace.valid)
        return false;

    const glm::mat4 objectMatrix =
        selectedObject->transform.matrix();

    // Modelo importado.
    if (selectedObject->model)
    {
        return selectedObject->model->getFacePositions(
            selectedFace.meshIndex,
            selectedFace.faceIndex,
            objectMatrix,
            a, b, c
        );
    }

    // Malha criada diretamente na cena.
    if (selectedObject->mesh)
    {
        if (selectedFace.meshIndex != 0)
            return false;

        if (!selectedObject->mesh->getFacePositions(
                selectedFace.faceIndex,
                a, b, c))
            return false;

        a = glm::vec3(objectMatrix * glm::vec4(a, 1.0f));
        b = glm::vec3(objectMatrix * glm::vec4(b, 1.0f));
        c = glm::vec3(objectMatrix * glm::vec4(c, 1.0f));

        return true;
    }

    return false;
}


bool Scene::beginSelectedFaceExtrusion()
{
    if (!selectedObject || !selectedFace.valid)
        return false;

    // Modelo importado.
    if (selectedObject->model)
    {
        return selectedObject->model->beginExtrusion(
            selectedFace.meshIndex,
            selectedFace.faceIndex
        );
    }

    // Malha criada diretamente na cena.
    if (selectedObject->mesh)
    {
        if (selectedFace.meshIndex != 0)
            return false;

        return selectedObject->mesh->beginExtrusion(
            selectedFace.faceIndex
        );
    }

    return false;
}

bool Scene::updateSelectedFaceExtrusion(float distance)
{
    if (!selectedObject || !selectedFace.valid)
        return false;

    if (selectedObject->model)
    {
        return selectedObject->model->updateExtrusion(
            selectedFace.meshIndex,
            distance
        );
    }

    if (selectedObject->mesh)
    {
        return selectedObject->mesh->updateExtrusion(distance);
    }

    return false;
}

void Scene::finishSelectedFaceExtrusion()
{
    if (!selectedObject || !selectedFace.valid)
        return;

    // Obtém o índice da nova tampa antes de finalizar.
    if (selectedObject->model)
    {
        const unsigned int newFace =
            selectedObject->model->getExtrudedFaceIndex(
                selectedFace.meshIndex
            );

        selectedObject->model->finishExtrusion(
            selectedFace.meshIndex
        );

        selectedFace.faceIndex = newFace;
    }
    else if (selectedObject->mesh)
    {
        const unsigned int newFace =
            selectedObject->mesh->getExtrudedFaceIndex();

        selectedObject->mesh->finishExtrusion();

        selectedFace.faceIndex = newFace;
    }

    selectedFace.valid = true;
}

void Scene::cancelSelectedFaceExtrusion()
{
    if (!selectedObject || !selectedFace.valid)
        return;

    if (selectedObject->model)
    {
        selectedObject->model->cancelExtrusion(
            selectedFace.meshIndex
        );
    }
    else if (selectedObject->mesh)
    {
        selectedObject->mesh->cancelExtrusion();
    }
}


void Scene::draw(
    Shader &shader,
    Camera &camera,
    const Texture &fallbackTexture)
{
    for (Object &object : objects)
    {
        // Não desenha objetos ocultos.
        if (!object.visible)
            continue;

        const glm::mat4 matrix = object.transform.matrix();

        if (object.model)
        {
            // O modelo envia a matriz de cada parte dele,
            // já combinada com a transformação do objeto.
            object.model->draw(
                shader,
                camera,
                fallbackTexture,
                matrix);
        }
        else if (object.mesh)
        {
            shader.setMat4("model", matrix);
            object.mesh->draw(shader, &fallbackTexture);
        }
    }
}