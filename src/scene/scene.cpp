#include "scene.h"

#include <iostream>
#include <utility>
#include <algorithm>
#include <cmath>
#include <glm/gtx/norm.hpp>
#include <limits>

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
        selectedObject = nullptr;

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
                matrix
            );
        }
        else if (object.mesh)
        {
            shader.setMat4("model", matrix);
            object.mesh->draw(shader, &fallbackTexture);
        }
    }
}