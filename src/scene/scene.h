#pragma once

#include <deque>
#include <map>
#include <memory>
#include <string>

#include "camera.h"
#include "graphics/mesh.h"
#include "graphics/model.h"
#include "graphics/shader.h"
#include "graphics/texture.h"
#include "transform.h"

// Lista de objetos que aparecem na tela. Quem carrega e desenha.
class Scene
{
public:
    struct Object
    {
        std::string name;
        std::shared_ptr<Model> model; // um modelo carregado de arquivo...
        std::shared_ptr<Mesh> mesh;   // ...ou uma malha criada no código (só um dos dois é usado)
        Transform transform;
    };

    // Carrega o modelo (ou reaproveita, se o mesmo arquivo já foi carregado) e põe na cena.
    // Devolve o objeto criado pra você ajustar a posição, ou nullptr se o carregamento falhar.
    Object* addModel(const std::string& path, const Transform& transform = Transform{});

    // Põe na cena uma malha já pronta (com as texturas já adicionadas a ela).
    Object* addMesh(std::shared_ptr<Mesh> mesh, const std::string& name = "",
                    const Transform& transform = Transform{});

    // Desenha todos os objetos. O shader precisa estar em uso e com a câmera já configurada.
    void draw(Shader& shader, Camera& camera, const Texture& fallbackTexture);

private:
    std::map<std::string, std::shared_ptr<Model>> modelCache;
    std::deque<Object> objects; // deque: o ponteiro devolvido continua válido ao acrescentar
};