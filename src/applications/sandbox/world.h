#pragma once

#include <string>

#include "engine/platform/window.h"
#include "engine/scene/scene.h"

// Monta os objetos da cena: a pirâmide animada e os modelos carregados de arquivo.
// Retorna false se algum recurso obrigatório não puder ser carregado.
bool buildWorld(Scene& scene);

// Adiciona a esfera procedural usada como corpo de demonstração física.
Scene::Object *addPhysicsDemoSphere(Scene &scene);

// Atalhos de teclado do sandbox para escolher qual modelo da cena é exibido
// (1 = macaco, 2 = multi, 3 = pyramid, 0 = todos). viewedModel recebe o nome exibido.
void handleModelShortcuts(const Window& window, Scene& scene, std::string& viewedModel);
