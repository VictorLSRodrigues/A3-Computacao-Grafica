#pragma once

#include <cstddef>
#include <string>

#include <glm/glm.hpp>

#include "engine/ui/UI.h"
#include "engine/platform/window.h"

// Valores que o HUD exibe. É só uma coleção de dados: quem preenche decide de onde eles vêm,
// e o HUD não depende de Scene, Light nem Camera.
struct EnvironmentInfo
{
    // Modo de uso.
    bool editing = false;
    std::string tool;        // ferramenta ativa; só é exibida no modo de edição
    std::string viewedModel; // modelo exibido ("Todos" ou o nome do modelo)

    // Câmera.
    glm::vec3 cameraPosition{0.0f};

    // Luz.
    std::string lightType;
    glm::vec3 lightPosition{0.0f};
    glm::vec3 lightColor{1.0f}; // RGB de 0 a 1
    bool lightSelected = false;

    // Cena; as faces contadas são as dos objetos visíveis.
    int objectCount = 0;
    int visibleObjectCount = 0;
    std::size_t faceCount = 0;

    // Objeto selecionado (hasSelection é falso quando a luz está selecionada).
    bool hasSelection = false;
    std::string selectedName;
    glm::vec3 selectedPosition{0.0f};
    glm::vec3 selectedRotation{0.0f}; // em graus
    glm::vec3 selectedScale{1.0f};
    bool hasFace = false;
    unsigned int selectedFace = 0;
};

// Exibe FPS e informações do ambiente no canto da tela.
// F3 mostra ou oculta o painel.
class HUD
{
public:
    void update(const Window &window, float deltaTime);
    void draw(UI &ui, const Window &window, const EnvironmentInfo &info);

private:
    bool visible = true;
    float fps = 0.0f;
    float frameTimeMs = 0.0f;
    float timeAccumulator = 0.0f;
    int frameCount = 0;

    void drawPanel(UI &ui, const Window &window, const EnvironmentInfo &info) const;
};
