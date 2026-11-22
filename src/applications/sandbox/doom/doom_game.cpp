#include "doom_game.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <thread>

#include <glad/glad.h>

#include "doomgeneric.h"
#include "doomkeys.h"

namespace
{
    DoomGame *activeDoomGame = nullptr;

    constexpr char kVertexShader[] = R"(
        #version 330 core
        layout (location = 0) in vec2 position;
        layout (location = 1) in vec2 textureCoordinate;
        out vec2 uv;
        void main()
        {
            uv = textureCoordinate;
            gl_Position = vec4(position, 0.0, 1.0);
        }
    )";

    constexpr char kFragmentShader[] = R"(
        #version 330 core
        in vec2 uv;
        uniform sampler2D doomFrame;
        out vec4 fragmentColor;
        void main()
        {
            fragmentColor = vec4(texture(doomFrame, uv).rgb, 1.0);
        }
    )";
}

DoomGame::~DoomGame()
{
    if (activeDoomGame == this)
        activeDoomGame = nullptr;
    destroyGraphics();
}

bool DoomGame::start(const std::filesystem::path &wadPath, std::string &error)
{
    if (activeDoomGame && activeDoomGame != this)
    {
        error = "Já existe outra instância do DOOM ativa.";
        return false;
    }

    std::ifstream wadFile(wadPath, std::ios::binary);
    char wadSignature[4]{};
    if (!wadFile.read(wadSignature, sizeof(wadSignature)) ||
        std::string(wadSignature, sizeof(wadSignature)) != "IWAD")
    {
        error = "O arquivo selecionado não é um IWAD válido do DOOM.";
        return false;
    }

    activeDoomGame = this;
    if (initialized)
    {
        if (suspended)
        {
            pausedDuration += std::chrono::steady_clock::now() - suspendedAt;
            suspended = false;
        }
        return true;
    }

    if (!createGraphics(error))
    {
        activeDoomGame = nullptr;
        return false;
    }

    arguments = {"RotorSandbox", "-iwad", wadPath.string()};
    for (std::size_t i = 0; i < arguments.size(); ++i)
        argumentPointers[i] = arguments[i].data();

    startTime = std::chrono::steady_clock::now();
    doomgeneric_Create(static_cast<int>(argumentPointers.size()), argumentPointers.data());
    initialized = true;
    frameReady = true;
    return true;
}

bool DoomGame::createGraphics(std::string &error)
{
    if (!screenShader.create(kVertexShader, kFragmentShader))
    {
        error = "Não foi possível compilar o shader de apresentação do DOOM.";
        return false;
    }

    constexpr float vertices[] = {
        -1.0f, -1.0f, 0.0f, 1.0f,
         1.0f, -1.0f, 1.0f, 1.0f,
         1.0f,  1.0f, 1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f, 1.0f,
         1.0f,  1.0f, 1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f, 0.0f};

    glGenVertexArrays(1, &vertexArray);
    glGenBuffers(1, &vertexBuffer);
    glGenTextures(1, &framebufferTexture);
    if (vertexArray == 0 || vertexBuffer == 0 || framebufferTexture == 0)
    {
        error = "Não foi possível criar os recursos OpenGL para o DOOM.";
        destroyGraphics();
        return false;
    }

    glBindVertexArray(vertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          reinterpret_cast<void *>(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindTexture(GL_TEXTURE_2D, framebufferTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, DOOMGENERIC_RESX, DOOMGENERIC_RESY,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    screenShader.use();
    screenShader.setInt("doomFrame", 0);

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    return true;
}

void DoomGame::destroyGraphics()
{
    if (framebufferTexture != 0)
    {
        glDeleteTextures(1, &framebufferTexture);
        framebufferTexture = 0;
    }
    if (vertexBuffer != 0)
    {
        glDeleteBuffers(1, &vertexBuffer);
        vertexBuffer = 0;
    }
    if (vertexArray != 0)
    {
        glDeleteVertexArrays(1, &vertexArray);
        vertexArray = 0;
    }
    screenShader.destroy();
}

void DoomGame::update(const Window &window)
{
    if (!initialized || suspended)
        return;

    collectInput(window);
    doomgeneric_Tick();
}

void DoomGame::render(int width, int height)
{
    if (!initialized || width <= 0 || height <= 0)
        return;

    const GLboolean depthEnabled = glIsEnabled(GL_DEPTH_TEST);
    const GLboolean cullEnabled = glIsEnabled(GL_CULL_FACE);
    const GLboolean blendEnabled = glIsEnabled(GL_BLEND);
    const GLboolean scissorEnabled = glIsEnabled(GL_SCISSOR_TEST);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
    glViewport(0, 0, width, height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (frameReady && DG_ScreenBuffer)
    {
        GLint unpackAlignment = 0;
        glGetIntegerv(GL_UNPACK_ALIGNMENT, &unpackAlignment);
        glBindTexture(GL_TEXTURE_2D, framebufferTexture);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                        DOOMGENERIC_RESX, DOOMGENERIC_RESY,
                        GL_BGRA, GL_UNSIGNED_BYTE, DG_ScreenBuffer);
        glPixelStorei(GL_UNPACK_ALIGNMENT, unpackAlignment);
        frameReady = false;
    }

    const float scale = std::min(
        static_cast<float>(width) / DOOMGENERIC_RESX,
        static_cast<float>(height) / DOOMGENERIC_RESY);
    const int renderWidth = static_cast<int>(DOOMGENERIC_RESX * scale);
    const int renderHeight = static_cast<int>(DOOMGENERIC_RESY * scale);
    glViewport((width - renderWidth) / 2, (height - renderHeight) / 2,
               renderWidth, renderHeight);

    screenShader.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, framebufferTexture);
    glBindVertexArray(vertexArray);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (depthEnabled)
        glEnable(GL_DEPTH_TEST);
    if (cullEnabled)
        glEnable(GL_CULL_FACE);
    if (blendEnabled)
        glEnable(GL_BLEND);
    if (scissorEnabled)
        glEnable(GL_SCISSOR_TEST);
    glViewport(0, 0, width, height);
}

void DoomGame::suspend()
{
    if (initialized && !suspended)
    {
        suspendedAt = std::chrono::steady_clock::now();
        suspended = true;
    }
}

uint32_t DoomGame::ticksMs() const
{
    const auto now = suspended ? suspendedAt : std::chrono::steady_clock::now();
    const auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            now - startTime - pausedDuration).count();
    return static_cast<uint32_t>(elapsed);
}

int DoomGame::getKey(int *pressed, unsigned char *key)
{
    if (keyEvents.empty())
        return 0;

    const KeyEvent event = keyEvents.front();
    keyEvents.pop_front();
    *pressed = event.pressed;
    *key = event.key;
    return 1;
}

unsigned char DoomGame::translateKey(int key)
{
    if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z)
        return static_cast<unsigned char>('a' + key - GLFW_KEY_A);
    if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9)
        return static_cast<unsigned char>('0' + key - GLFW_KEY_0);

    switch (key)
    {
    case GLFW_KEY_UP:
    case GLFW_KEY_KP_8:
        return KEY_UPARROW;
    case GLFW_KEY_DOWN:
    case GLFW_KEY_KP_2:
        return KEY_DOWNARROW;
    case GLFW_KEY_LEFT:
    case GLFW_KEY_KP_4:
        return KEY_LEFTARROW;
    case GLFW_KEY_RIGHT:
    case GLFW_KEY_KP_6:
        return KEY_RIGHTARROW;
    case GLFW_KEY_LEFT_CONTROL:
    case GLFW_KEY_RIGHT_CONTROL:
        return KEY_FIRE;
    case GLFW_KEY_SPACE:
        return KEY_USE;
    case GLFW_KEY_LEFT_SHIFT:
    case GLFW_KEY_RIGHT_SHIFT:
        return KEY_RSHIFT;
    case GLFW_KEY_LEFT_ALT:
    case GLFW_KEY_RIGHT_ALT:
        return KEY_RALT;
    case GLFW_KEY_ESCAPE:
        return KEY_ESCAPE;
    case GLFW_KEY_ENTER:
    case GLFW_KEY_KP_ENTER:
        return KEY_ENTER;
    case GLFW_KEY_TAB:
        return KEY_TAB;
    case GLFW_KEY_BACKSPACE:
        return KEY_BACKSPACE;
    case GLFW_KEY_COMMA:
        return KEY_STRAFE_L;
    case GLFW_KEY_PERIOD:
        return KEY_STRAFE_R;
    case GLFW_KEY_MINUS:
        return KEY_MINUS;
    case GLFW_KEY_EQUAL:
        return KEY_EQUALS;
    case GLFW_KEY_F1:
        return KEY_F1;
    case GLFW_KEY_F2:
        return KEY_F2;
    case GLFW_KEY_F3:
        return KEY_F3;
    case GLFW_KEY_F4:
        return KEY_F4;
    case GLFW_KEY_F5:
        return KEY_F5;
    case GLFW_KEY_F6:
        return KEY_F6;
    case GLFW_KEY_F7:
        return KEY_F7;
    case GLFW_KEY_F8:
        return KEY_F8;
    case GLFW_KEY_F9:
        return KEY_F9;
    case GLFW_KEY_F11:
        return KEY_F11;
    case GLFW_KEY_F12:
        return KEY_F12;
    default:
        return 0;
    }
}

void DoomGame::collectInput(const Window &window)
{
    std::array<bool, 256> currentKeys{};
    for (int key = 0; key <= GLFW_KEY_LAST; ++key)
    {
        if (!window.keyDown(key))
            continue;

        const unsigned char doomKey = translateKey(key);
        if (doomKey != 0)
            currentKeys[doomKey] = true;
    }

    for (std::size_t key = 1; key < currentKeys.size(); ++key)
    {
        if (currentKeys[key] != heldDoomKeys[key])
        {
            keyEvents.push_back({currentKeys[key] ? 1 : 0,
                                 static_cast<unsigned char>(key)});
            heldDoomKeys[key] = currentKeys[key];
        }
    }
}

extern "C"
{
    void DG_Init() {}

    void DG_DrawFrame()
    {
        if (activeDoomGame)
            activeDoomGame->markFrameReady();
    }

    void DG_SleepMs(uint32_t ms)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }

    uint32_t DG_GetTicksMs()
    {
        return activeDoomGame ? activeDoomGame->ticksMs() : 0;
    }

    int DG_GetKey(int *pressed, unsigned char *key)
    {
        return activeDoomGame ? activeDoomGame->getKey(pressed, key) : 0;
    }

    void DG_SetWindowTitle(const char *) {}
}
