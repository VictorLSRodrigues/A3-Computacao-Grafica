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

    objects.push_back({path, model, nullptr, transform, {}, true, path});
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

    objects.push_back({name, nullptr, std::move(mesh), transform, {}, true, {}});
    return &objects.back();
}

void Scene::captureDefaults()
{
    defaultStates.clear();
    defaultStates.reserve(objects.size());

    for (const Object& object : objects)
    {
        defaultStates.push_back({
            object.model,
            object.mesh,
            object.transform,
            object.visible
        });
    }

    std::cout << "Estado original da cena capturado: "
              << defaultStates.size()
              << " objetos." << std::endl;
}

bool Scene::resetToDefaults()
{
    if (defaultStates.size() != objects.size())
    {
        std::cerr
            << "Nao foi possivel restaurar a cena: "
            << "quantidade de objetos diferente do estado original."
            << std::endl;
        return false;
    }

    std::size_t index = 0;

    for (Object& object : objects)
    {
        const ObjectDefaultState& original =
            defaultStates[index++];

        // Recupera os recursos originais, sem extrusões.
        object.model = original.model;
        object.mesh = original.mesh;

        // Recupera as transformações iniciais.
        object.transform = original.transform;

        // Recupera a visibilidade original.
        object.visible = original.visible;
    }

    std::cout << "Cena restaurada para os padroes originais."
              << std::endl;

    return true;
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

        const BoundingBox *bounds = object.boundingBox();
        if (!bounds || !bounds->valid)
            continue;

        const glm::vec3 localCenter =
            (bounds->min + bounds->max) * 0.5f;

        // Calcula o raio a partir do centro real da geometria, não da origem
        // arbitrária que veio do Blender.
        float localRadius = 0.0f;
        for (int x = 0; x < 2; ++x)
            for (int y = 0; y < 2; ++y)
                for (int z = 0; z < 2; ++z)
                {
                    const glm::vec3 corner(
                        x ? bounds->max.x : bounds->min.x,
                        y ? bounds->max.y : bounds->min.y,
                        z ? bounds->max.z : bounds->min.z);
                    localRadius = std::max(
                        localRadius, glm::length(corner - localCenter));
                }

        const glm::vec3 scale = glm::abs(object.transform.scale());
        const float maxScale = std::max(
            scale.x, std::max(scale.y, scale.z));
        const float worldRadius = localRadius * maxScale;

        const glm::vec3 worldCenter = object.visualCenter();
        const glm::vec3 offset = rayOrigin - worldCenter;

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


bool Scene::beginFaceExtrusion(
    Object *object,
    const FaceSelection &face)
{
    if (!object || !face.valid)
        return false;

    // Modelo importado.
    if (object->model)
    {
        if (face.meshIndex >= object->model->getMeshCount())
            return false;

        // Verifica se o modelo ainda pertence ao cache compartilhado.
        bool usesCachedModel = false;

        for (const auto& cached : modelCache)
        {
            if (cached.second == object->model)
            {
                usesCachedModel = true;
                break;
            }
        }

        // Se for compartilhado, cria uma cópia independente.
        if (usesCachedModel)
        {
            std::shared_ptr<Model> independentModel =
                object->model->clone();

            if (!independentModel)
            {
                std::cerr << "Falha ao criar copia independente do modelo."
                          << std::endl;
                return false;
            }

            object->model = std::move(independentModel);
        }

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

        // Se estiver compartilhada, cria uma cópia independente.
        if (object->mesh.use_count() > 1)
        {
            std::shared_ptr<Mesh> independentMesh =
                std::make_shared<Mesh>();

            if (!independentMesh->copyFrom(*object->mesh))
            {
                std::cerr << "Falha ao criar copia independente da malha."
                          << std::endl;
                return false;
            }

            object->mesh = std::move(independentMesh);
        }

        return object->mesh->beginExtrusion(face.faceIndex);
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
