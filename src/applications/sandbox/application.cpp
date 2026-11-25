#include "application.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>
#include <memory>
#include <utility>
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

    // Mesa.glb tem um tampo com aproximadamente 0,21 unidade no eixo local Y.
    // O colisor representa apenas essa placa; pernas continuam sendo visuais.
    constexpr float SANDBOX_TABLETOP_COLLIDER_THICKNESS = 0.21f;

    bool appendGeometryTriangles(
        const Geometry& geometry,
        const glm::mat4& localTransform,
        std::vector<glm::vec3>& positions,
        std::vector<unsigned int>& triangleIndices)
    {
        if (geometry.getTopology() != PrimitiveTopology::Triangles)
            return false;

        const auto& vertices = geometry.getStructuredVertices();
        if (vertices.empty())
            return false;

        if (positions.size() + vertices.size() >
            static_cast<std::size_t>(std::numeric_limits<unsigned int>::max()))
        {
            return false;
        }

        const unsigned int baseIndex = static_cast<unsigned int>(positions.size());
        positions.reserve(positions.size() + vertices.size());
        for (const Vertex& vertex : vertices)
        {
            const glm::vec4 transformed =
                localTransform * glm::vec4(vertex.position, 1.0f);
            if (!std::isfinite(transformed.x) || !std::isfinite(transformed.y) ||
                !std::isfinite(transformed.z) || std::fabs(transformed.w) <= 1e-8f)
            {
                return false;
            }
            positions.emplace_back(glm::vec3(transformed) / transformed.w);
        }

        const auto& indices = geometry.getIndexData();
        if (!indices.empty())
        {
            if (indices.size() % 3 != 0)
                return false;
            triangleIndices.reserve(triangleIndices.size() + indices.size());
            for (unsigned int index : indices)
            {
                if (index >= vertices.size())
                    return false;
                triangleIndices.push_back(baseIndex + index);
            }
            return true;
        }

        // Geometrias não indexadas mantêm faces reconstruídas pelo Geometry.
        for (const Face& face : geometry.getFaces())
        {
            if (face.vertexIndices.size() < 3)
                continue;
            for (std::size_t i = 1; i + 1 < face.vertexIndices.size(); ++i)
            {
                const unsigned int a = face.vertexIndices[0];
                const unsigned int b = face.vertexIndices[i];
                const unsigned int c = face.vertexIndices[i + 1];
                if (a >= vertices.size() || b >= vertices.size() || c >= vertices.size())
                    return false;
                triangleIndices.push_back(baseIndex + a);
                triangleIndices.push_back(baseIndex + b);
                triangleIndices.push_back(baseIndex + c);
            }
        }

        return triangleIndices.size() >= 3;
    }

    std::shared_ptr<const rotor::physics::TriangleMeshColliderData>
    buildObjectTriangleMeshCollider(
        const Scene::Object& object,
        glm::vec3& localCenterOfMass)
    {
        std::vector<glm::vec3> positions;
        std::vector<unsigned int> triangleIndices;

        if (object.model)
        {
            for (std::size_t meshIndex = 0;
                 meshIndex < object.model->getMeshCount(); ++meshIndex)
            {
                const Geometry* geometry = object.model->getMeshGeometry(meshIndex);
                const glm::mat4* meshTransform = object.model->getMeshTransform(meshIndex);
                if (!geometry || !meshTransform ||
                    !appendGeometryTriangles(
                        *geometry, *meshTransform, positions, triangleIndices))
                {
                    return nullptr;
                }
            }
        }
        else if (object.mesh)
        {
            if (!appendGeometryTriangles(
                    object.mesh->getGeometry(), glm::mat4(1.0f),
                    positions, triangleIndices))
            {
                return nullptr;
            }
        }
        else
        {
            return nullptr;
        }

        auto data = std::make_shared<rotor::physics::TriangleMeshColliderData>();
        if (!rotor::physics::buildTriangleMeshColliderData(
                positions, triangleIndices, *data))
        {
            return nullptr;
        }

        localCenterOfMass = data->localCenterOfMass;
        return data;
    }

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

bool SandboxApplication::registerScenePhysics(bool stabilizeOverlaps)
{
    // Recria os vínculos após alterações no Editor ou carregamento de cena,
    // atualizando centros, escalas e limites geométricos dos colisores.
    physics.clear();
    physicsBindings.clear();
    physics.setGravity(glm::vec3(0.0f, -9.81f, 0.0f));
    physics.setGroundPlane(-1.5f, 0.75f, 0.55f);

    std::size_t registeredBodies = 0;
    std::size_t expectedBodies = 0;
    bool tableColliderRegistered = false;

    for (Scene::Object &object : scene.getObjects())
    {
        // O grid visual é representado pelo plano de chão do PhysicsWorld.
        if (object.name == "grid_plane")
            continue;

        const BoundingBox *bounds = object.boundingBox();
        if (!bounds || !bounds->valid)
        {
            std::cerr << "Objeto sem limites válidos para a física: "
                      << object.name << std::endl;
            return false;
        }

        const glm::vec3 boundsCenter =
            (bounds->min + bounds->max) * 0.5f;
        const glm::vec3 halfExtents =
            (bounds->max - bounds->min) * 0.5f;

        // A mesa não é um corpo dinâmico. Seu tampo é um colisor de caixa
        // estático, orientado pelo Transform real do objeto. Não usamos a
        // bounding box completa porque isso preencheria o espaço entre as pernas.
        if (object.name == "mesa")
        {
            const float halfThickness =
                SANDBOX_TABLETOP_COLLIDER_THICKNESS * 0.5f;
            const glm::vec3 tabletopCenter(
                boundsCenter.x,
                bounds->max.y - halfThickness,
                boundsCenter.z);
            const glm::vec3 tabletopHalfExtents(
                halfExtents.x,
                halfThickness,
                halfExtents.z);

            if (!physics.addStaticBox(
                    object.transform,
                    tabletopCenter,
                    tabletopHalfExtents,
                    0.0f,
                    0.85f,
                    0.65f))
            {
                std::cerr << "Falha ao registrar o colisor estático do tampo da mesa."
                          << std::endl;
                return false;
            }

            tableColliderRegistered = true;
            continue;
        }

        ++expectedBodies;
        PhysicsBinding &binding = physicsBindings.emplace_back();
        binding.object = &object;

        const glm::vec3 absoluteScale = glm::abs(object.transform.scale());
        bool bodyRegistered = false;

        if (object.name == "physics_sphere")
        {
            // A esfera procedural continua usando seu colisor analítico.
            binding.localCenterOfMass = boundsCenter;
            binding.bodyTransform = object.transform;
            binding.bodyTransform.move(object.visualCenter());

            float localRadius = glm::length(halfExtents);
            if (object.mesh)
            {
                const auto &vertices =
                    object.mesh->getGeometry().getStructuredVertices();
                if (!vertices.empty())
                {
                    localRadius = 0.0f;
                    for (const Vertex &vertex : vertices)
                    {
                        localRadius = std::max(
                            localRadius,
                            glm::length(vertex.position - boundsCenter));
                    }
                }
            }

            const float worldRadius = localRadius * std::max(
                absoluteScale.x,
                std::max(absoluteScale.y, absoluteScale.z));
            const float radiusRatio = worldRadius / 0.5f;
            const float mass = std::clamp(
                radiusRatio * radiusRatio * radiusRatio, 0.05f, 1000.0f);
            bodyRegistered = physics.addSphereBody(
                binding.bodyTransform,
                glm::vec3(0.0f),
                localRadius,
                0.12f,
                mass,
                0.75f,
                0.55f);
        }
        else
        {
            glm::vec3 localCenterOfMass = boundsCenter;
            std::shared_ptr<const rotor::physics::TriangleMeshColliderData> meshCollider =
                buildObjectTriangleMeshCollider(object, localCenterOfMass);

            // A posição física acompanha o centro de massa aproximado, que é
            // ponderado pela área dos triângulos. A origem visual continua intacta.
            binding.localCenterOfMass = localCenterOfMass;
            binding.bodyTransform = object.transform;
            const glm::vec3 worldCenterOfMass = glm::vec3(
                object.transform.matrix() * glm::vec4(localCenterOfMass, 1.0f));
            binding.bodyTransform.move(worldCenterOfMass);

            const float worldVolume = 8.0f *
                halfExtents.x * absoluteScale.x *
                halfExtents.y * absoluteScale.y *
                halfExtents.z * absoluteScale.z;
            const float mass = std::clamp(
                worldVolume * 250.0f, 0.05f, 5.0f);

            if (meshCollider)
            {
                bodyRegistered = physics.addTriangleMeshBody(
                    binding.bodyTransform,
                    std::move(meshCollider),
                    0.05f,
                    mass,
                    0.78f,
                    0.58f);
            }
            else
            {
                // Fallback explícito para geometrias não trianguladas ou Raw.
                binding.localCenterOfMass = boundsCenter;
                binding.bodyTransform = object.transform;
                binding.bodyTransform.move(object.visualCenter());
                bodyRegistered = physics.addBoxBody(
                    binding.bodyTransform,
                    halfExtents,
                    glm::vec3(0.0f),
                    0.05f,
                    mass,
                    0.78f,
                    0.58f);
                std::cerr << "[SANDBOX] " << object.name
                          << ": geometria sem triângulos estruturados; usando OBB."
                          << std::endl;
            }
        }

        if (!bodyRegistered)
        {
            std::cerr << "Falha ao registrar o colisor físico do objeto: "
                      << object.name << std::endl;
            physicsBindings.pop_back();
            return false;
        }

        ++registeredBodies;
    }

    if (!tableColliderRegistered || registeredBodies == 0 ||
        registeredBodies != expectedBodies)
    {
        std::cerr << "Registro físico incompleto: " << registeredBodies
                  << " de " << expectedBodies
                  << " corpos dinâmicos; colisor da mesa: "
                  << (tableColliderRegistered ? "sim" : "não") << "."
                  << std::endl;
        return false;
    }

    // Corrige sobreposições herdadas do estado salvo antes da primeira queda.
    // Isso não aplica impulsos: os objetos começam sem energia artificial e a
    // pose já estabilizada passa a ser a referência do comando Reset.
    if (stabilizeOverlaps)
        physics.stabilizeInitialOverlaps(24);
    physics.captureInitialPose();
    syncSceneFromPhysics();

    std::cout << "[SANDBOX] " << registeredBodies
              << " corpos dinâmicos registrados; tampo da mesa estático; grid excluído."
              << std::endl;
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
        const glm::vec3 scaledLocalCenter = scale * binding.localCenterOfMass;
        const glm::vec3 worldCenterOffset = orientation * scaledLocalCenter;

        objectTransform.setScale(scale);
        objectTransform.setOrientation(orientation);
        objectTransform.move(
            binding.bodyTransform.position() - worldCenterOffset);
    }
}

void SandboxApplication::resetPhysicsFromScene(bool stabilizeOverlaps)
{
    if (!registerScenePhysics(stabilizeOverlaps))
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

    if (window.keyPressed(GLFW_KEY_F4))
    {
        const bool enabled = !physics.profilingEnabled();
        physics.setProfilingEnabled(enabled);
        std::cout << "Profiling da física "
                  << (enabled ? "ativado" : "desativado")
                  << ". Consulte physics_profile.log." << std::endl;
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
        resetPhysicsFromScene(false);

    if (!editor.isEditing() && !menu.isOpen())
    {
        physics.setTimeScale(menu.getPhysicsTimeScale());
        physics.update(deltaTime);
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
