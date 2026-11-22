#pragma once

#include <algorithm>
#include <utility>

#include "engine/platform/window.h"
#include <GLFW/glfw3.h>

// Base de uma aplicacao Rotor: gerencia a janela e o ciclo de vida comum.
// Recursos e comportamento especificos continuam pertencendo a cada aplicacao.
namespace rotor
{
class Application
{
public:
    explicit Application(WindowConfig windowConfig)
        : windowConfig(std::move(windowConfig))
    {
    }

    virtual ~Application() = default;

    Application(const Application &) = delete;
    Application &operator=(const Application &) = delete;

    int run()
    {
        if (!window.create(windowConfig))
            return -1;

        if (!onInitialize())
        {
            onShutdown();
            return -1;
        }

        double previousFrame = glfwGetTime();

        while (!window.shouldClose())
        {
            const double currentFrame = glfwGetTime();
            const float deltaTime = static_cast<float>(
                std::min(currentFrame - previousFrame, 0.1));
            previousFrame = currentFrame;

            if (window.keyPressed(GLFW_KEY_ESCAPE))
                onEscapePressed();

            onUpdate(deltaTime);
            onRender();

            window.swapBuffers();
            window.pollEvents();
        }

        onShutdown();
        return 0;
    }

protected:
    Window window;

    // O Escape fecha a aplicacao por padrao, mas cada app pode definir
    // outro comportamento (por exemplo, fechar um painel ou cancelar uma acao).
    virtual void onEscapePressed() { window.close(); }

    virtual bool onInitialize() = 0;
    // Atualizacao e opcional: apps sem logica por quadro nao precisam sobrescrever.
    virtual void onUpdate(float) {}
    virtual void onRender() = 0;
    virtual void onShutdown() {}

private:
    WindowConfig windowConfig;
};
} // namespace rotor
