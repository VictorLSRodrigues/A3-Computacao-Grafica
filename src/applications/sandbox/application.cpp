#include "application.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif

#include "applications/sandbox/ui/environment_info.h"
#include "world.h"

#ifndef SANDBOX_DATA_DIR
#define SANDBOX_DATA_DIR "."
#endif
#ifndef SANDBOX_SCENE_DATA_DIR
#define SANDBOX_SCENE_DATA_DIR "src/applications/sandbox/data"
#endif

namespace
{
    constexpr const char *SCENE_STATE_PATH =
        SANDBOX_DATA_DIR "/scene_state.json";
    constexpr const char *SCENE_EXPORT_DIR = SANDBOX_SCENE_DATA_DIR;

    // Escala temporal local do Sandbox: 0.1 = simulacao a 10% da velocidade.
    // A camera, a interface e o restante da aplicacao continuam em tempo real.
    constexpr float SANDBOX_PHYSICS_TIME_SCALE = 0.1f;

    WindowConfig createWindowConfig()
    {
        WindowConfig config;
        config.title = "OpenGL - RotorGrafico";
        config.mode = WindowMode::Fullscreen;
        return config;
    }
}

SandboxApplication::SandboxApplication()
    : rotor::Application(createWindowConfig())
{
}

bool SandboxApplication::onInitialize()
{
    if (!renderer.init())
        return false;

    if (!ui.create())
    {
        std::cout << "Falha ao inicializar a UI!" << std::endl;
        return false;
    }

    if (!editor.create())
    {
        std::cout << "Falha ao inicializar os recursos do Editor!"
                  << std::endl;
        return false;
    }

    // A cena cria os objetos visuais. A participação de cada objeto na física
    // é configurada separadamente pela aplicação, depois de carregar o estado.
    if (!buildWorld(scene))
        return false;

    if (!addPhysicsDemoSphere(scene))
        return false;

    // Registra a cena original antes de carregar os dados locais.
    scene.captureDefaults();

    // Depois recupera as transformações salvas anteriormente.
    if (!scene.loadTransforms(SCENE_STATE_PATH, light))
    {
        std::cout << "Nenhum estado salvo foi restaurado. "
                  << "Utilizando as transformacoes iniciais."
                  << std::endl;
    }

    // A aplicação decide explicitamente quais objetos participam da física.
    // Todos os objetos da cena recebem corpos dinâmicos, exceto o grid, que
    // é representado pelo plano de chão do PhysicsWorld.
    if (!registerScenePhysics())
        return false;

    sceneReadyForPersistence = true;
    return true;
}

bool SandboxApplication::registerScenePhysics()
{
    // Recria os vínculos após alterações no Editor ou carregamento de cena,
    // atualizando centros, escalas e limites geométricos dos colisores.
    physics.clear();
    physicsBindings.clear();
    physics.setGravity(glm::vec3(0.0f, -9.81f, 0.0f));
    physics.setGroundPlane(-1.5f, 0.75f, 0.55f);

    std::size_t registeredBodies = 0;
    std::size_t expectedBodies = 0;

    for (Scene::Object &object : scene.getObjects())
    {
        // O grid visual é o único objeto excluído: sua superfície é
        // representada pelo plano de chão do PhysicsWorld.
        if (object.name == "grid_plane")
            continue;

        ++expectedBodies;
        const BoundingBox *bounds = object.boundingBox();
        if (!bounds || !bounds->valid)
        {
            std::cerr << "Objeto sem limites válidos para a física: "
                      << object.name << std::endl;
            return false;
        }

        const glm::vec3 localCenter =
            (bounds->min + bounds->max) * 0.5f;
        const glm::vec3 halfExtents =
            (bounds->max - bounds->min) * 0.5f;

        float localRadius = glm::length(halfExtents);

        // A mesa é muito larga e relativamente fina. Usar a meia diagonal
        // criaria uma esfera que começaria sobreposta a quase todos os itens
        // da bancada. Nesta primeira aproximação, sua esfera usa a meia-altura.
        if (object.name == "mesa")
            localRadius = halfExtents.y;

        // Malhas criadas pelo próprio Sandbox podem fornecer um raio mais
        // preciso que a meia diagonal da caixa envolvente.
        if (object.mesh && object.name != "mesa")
        {
            const Geometry &geometry = object.mesh->getGeometry();
            const auto &vertices = geometry.getStructuredVertices();
            if (!vertices.empty())
            {
                localRadius = 0.0f;
                for (const Vertex &vertex : vertices)
                {
                    localRadius = std::max(
                        localRadius, glm::length(vertex.position - localCenter));
                }
            }
        }

        if (!(localRadius > 1e-4f) || !std::isfinite(localRadius))
        {
            std::cerr << "Colisor inválido para o objeto: "
                      << object.name << std::endl;
            return false;
        }

        PhysicsBinding &binding = physicsBindings.emplace_back();
        binding.object = &object;
        binding.localCenter = localCenter;
        binding.bodyTransform = object.transform;
        binding.bodyTransform.move(object.visualCenter());

        const glm::vec3 absoluteScale = glm::abs(object.transform.scale());
        const float scaleFactor = std::max(
            absoluteScale.x, std::max(absoluteScale.y, absoluteScale.z));
        const float worldRadius = localRadius * scaleFactor;

        // Massa proporcional ao volume da esfera aproximada: um corpo com
        // raio de 0,5 m tem massa de referência de 1 kg.
        const float radiusRatio = worldRadius / 0.5f;
        const float mass = std::clamp(
            radiusRatio * radiusRatio * radiusRatio, 0.05f, 1000.0f);

        if (!physics.addBody(
                binding.bodyTransform,
                glm::vec3(0.0f),
                localRadius,
                0.12f,
                mass,
                0.75f,
                0.55f))
        {
            std::cerr << "Falha ao registrar objeto na física: "
                      << object.name << std::endl;
            physicsBindings.pop_back();
            return false;
        }

        ++registeredBodies;
    }

    if (registeredBodies == 0 || registeredBodies != expectedBodies)
    {
        std::cerr << "Registro físico incompleto: " << registeredBodies
                  << " de " << expectedBodies << " objetos." << std::endl;
        return false;
    }

    std::cout << "[SANDBOX] Corpos físicos registrados: "
              << registeredBodies << " (grid excluído)." << std::endl;
    return true;
}

void SandboxApplication::syncSceneFromPhysics()
{
    for (PhysicsBinding &binding : physicsBindings)
    {
        if (!binding.object)
            continue;

        Transform &objectTransform = binding.object->transform;
        const glm::quat orientation = binding.bodyTransform.orientation();
        const glm::vec3 scale = binding.bodyTransform.scale();
        const glm::vec3 scaledLocalCenter = scale * binding.localCenter;
        const glm::vec3 worldCenterOffset = orientation * scaledLocalCenter;

        objectTransform.setScale(scale);
        objectTransform.setOrientation(orientation);
        objectTransform.move(
            binding.bodyTransform.position() - worldCenterOffset);
    }
}

void SandboxApplication::resetPhysicsFromScene()
{
    if (!registerScenePhysics())
    {
        std::cerr << "Falha ao reconstruir os corpos físicos da cena."
                  << std::endl;
        return;
    }

    physics.reset();
    syncSceneFromPhysics();
}

void SandboxApplication::onUpdate(float deltaTime)
{
    if (doomActive)
    {
        if (window.keyPressed(GLFW_KEY_F10))
        {
            doomGame.suspend();
            doomActive = false;
            window.setCursorMode(Window::CursorMode::Normal);
        }
        else
        {
            doomGame.update(window);
        }
        return;
    }

    hud.update(window, deltaTime);
    menu.update(window);

    const bool wasEditing = editor.isEditing();

    // A câmera só recebe controles quando o menu está fechado.
    if (menu.isOpen())
    {
        window.setCursorMode(Window::CursorMode::Normal);
    }
    else
    {
        editor.handleModeToggle();

        camera.processInput(window, deltaTime);

        if (!editor.isEditing())
            handleModelShortcuts(window, scene, selectedModelName);
    }

    // Mantém a câmera com o tamanho atual da janela.
    camera.setViewportSize(window.getWidth(), window.getHeight());

    // O editor só trata o mouse quando o menu está fechado.
    editor.update(!menu.isOpen());

    // Atualiza as animações e os objetos da cena.
    scene.update(deltaTime);

    // A física fica pausada durante a edição e enquanto o menu está aberto.
    // Ao sair do Editor, a transformação editada vira o novo estado inicial.
    if (wasEditing && !editor.isEditing())
        resetPhysicsFromScene();

    if (!editor.isEditing() && !menu.isOpen())
    {
        physics.update(deltaTime * SANDBOX_PHYSICS_TIME_SCALE);
        syncSceneFromPhysics();
    }
}

void SandboxApplication::onRender()
{
    if (doomActive)
    {
        doomGame.render(window.getWidth(), window.getHeight());
        return;
    }

    renderer.render(scene, light, camera);
    renderUI();
}

void SandboxApplication::onShutdown()
{
    if (!sceneReadyForPersistence)
        return;

    // A trajetória física é estado de execução, não uma alteração permanente
    // da cena. Se o usuário ainda estiver editando, preserva a transformação
    // ajustada manualmente; caso contrário, restaura a posição inicial do corpo.
    if (editor.isEditing())
        resetPhysicsFromScene();
    else
    {
        physics.reset();
        syncSceneFromPhysics();
    }

    // Salva as transformações quando a aplicação encerra.
    if (!scene.saveTransforms(SCENE_STATE_PATH, light))
    {
        std::cerr << "Falha ao salvar as transformacoes da cena!"
                  << std::endl;
    }
}

void SandboxApplication::renderUI()
{
    // Inicia a interface gráfica.
    ui.begin(window);

    // Desenha o HUD e as informações da cena.
    hud.draw(
        ui,
        window,
        makeEnvironmentInfo(scene, camera, light, editor, selectedModelName));

    // Desenha o menu.
    menu.draw(
        window,
        ui,
        renderer,
        camera,
        light,

        // Inicialização do DOOM.
        [this]()
        {
            startDoom();
        },

        // Salvar uma cena com nome próprio.
        [this](const std::string &name)
        {
            const std::filesystem::path sceneDirectory =
                std::filesystem::path(SCENE_EXPORT_DIR);

            const std::filesystem::path sceneFile =
                sceneDirectory / (name + ".json");

            return scene.saveTransforms(sceneFile.string(), light);
        },
        // Carregar a cena selecionada.
        [this](const std::string &filePath)
        {
            if (!scene.resetToDefaults())
            {
                std::cerr << "Falha ao restaurar a cena antes do carregamento."
                          << std::endl;
                return false;
            }

            editor.resetForSceneDefaults();
            light.resetProperties();

            if (!scene.loadTransforms(filePath, light))
            {
                resetPhysicsFromScene();
                std::cerr << "Falha ao carregar a cena: "
                          << filePath << std::endl;
                return false;
            }

            resetPhysicsFromScene();
            selectedModelName = "Todos";
            return true;
        },
        // Restauração completa da cena.
        [this]()
        {
            if (!scene.resetToDefaults())
            {
                std::cerr << "Falha ao restaurar os objetos da cena."
                          << std::endl;
                return;
            }

            editor.resetForSceneDefaults();
            light.resetProperties();
            resetPhysicsFromScene();

            selectedModelName = "Todos";

            if (!scene.saveTransforms(SCENE_STATE_PATH, light))
            {
                std::cerr << "Falha ao salvar o estado restaurado."
                          << std::endl;
            }
        });

    // Exibe as ferramentas somente no modo de edição
    // e quando o menu estiver fechado.
    if (editor.isEditing() && !menu.isOpen())
    {
        editor.drawGizmos();

        if (editor.isLightSelected())
        {
            const Light previousLight = light;
            lightPanel.draw(window, ui, light);
            editor.recordLightChange(previousLight);
        }

        // Desenha a barra de ferramentas.
        editor.drawToolbar();
    }

    // Finaliza a interface gráfica.
    ui.end();
}

void SandboxApplication::startDoom()
{
#ifdef _WIN32
    std::vector<wchar_t> applicationPath(32768, L'\0');
    const DWORD pathLength = GetModuleFileNameW(
        nullptr, applicationPath.data(), static_cast<DWORD>(applicationPath.size()));
    if (pathLength == 0 || pathLength >= applicationPath.size())
    {
        MessageBoxW(nullptr, L"Não foi possível localizar a pasta do aplicativo.",
                    L"DOOM", MB_OK | MB_ICONERROR);
        return;
    }

    const std::filesystem::path applicationDirectory =
        std::filesystem::path(std::wstring(applicationPath.data(), pathLength)).parent_path();
    const std::filesystem::path doomWad = applicationDirectory / L"doom1.wad";

    std::error_code fileError;
    if (!std::filesystem::is_regular_file(doomWad, fileError) || fileError)
    {
        const std::wstring message =
            L"Coloque um arquivo doom1.wad obtido legalmente ao lado de OpenGLProjeto.exe:\n\n" +
            applicationDirectory.wstring();
        MessageBoxW(nullptr, message.c_str(), L"DOOM - WAD necessário",
                    MB_OK | MB_ICONINFORMATION);
        return;
    }

    std::string error;
    if (!doomGame.start(doomWad, error))
    {
        std::cerr << error << std::endl;
        MessageBoxW(nullptr,
                    L"Não foi possível iniciar o DOOM dentro da engine. Consulte a saída do aplicativo.",
                    L"DOOM", MB_OK | MB_ICONERROR);
        return;
    }

    doomActive = true;
    window.setCursorMode(Window::CursorMode::Hidden);
#else
    std::error_code fileError;
    const std::filesystem::path doomWad =
        std::filesystem::current_path() / "build" / "doom1.wad";
    if (!std::filesystem::is_regular_file(doomWad, fileError) || fileError)
    {
        std::cerr << "Arquivo doom1.wad não encontrado: " << doomWad << std::endl;
        return;
    }

    std::string error;
    if (!doomGame.start(doomWad, error))
    {
        std::cerr << error << std::endl;
        return;
    }

    doomActive = true;
#endif
}
