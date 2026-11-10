#include "scene.h"

#include <iostream>
#include <utility>

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
            std::cout << "Falha ao carregar o modelo " << path << ": " << error.what() << std::endl;
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

Scene::Object *Scene::addMesh(std::shared_ptr<Mesh> mesh, const std::string &name,
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
    for (Object& object : objects)
        object.visible = true;
}

void Scene::draw(Shader &shader, Camera &camera, const Texture &fallbackTexture)
{
    for (Object &object : objects)
    {
        if (!object.visible)
            continue;

        const glm::mat4 matrix = object.transform.matrix();

        if (object.model)
        {
            // O modelo envia a matriz de cada parte dele, já combinada com a do objeto.
            object.model->draw(shader, camera, fallbackTexture, matrix);
        }
        else if (object.mesh)
        {
            shader.setMat4("model", matrix);
            object.mesh->draw(shader, &fallbackTexture);
        }
    }
}