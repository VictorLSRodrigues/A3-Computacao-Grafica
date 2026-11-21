#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <string>

#include "engine/graphics/resources/shader.h"
#include "engine/platform/window.h"

class DoomGame
{
public:
    DoomGame() = default;
    ~DoomGame();

    DoomGame(const DoomGame &) = delete;
    DoomGame &operator=(const DoomGame &) = delete;

    bool start(const std::filesystem::path &wadPath, std::string &error);
    void update(const Window &window);
    void render(int width, int height);
    void suspend();

    void markFrameReady() { frameReady = true; }
    uint32_t ticksMs() const;
    int getKey(int *pressed, unsigned char *key);

private:
    struct KeyEvent
    {
        int pressed;
        unsigned char key;
    };

    bool createGraphics(std::string &error);
    void destroyGraphics();
    void collectInput(const Window &window);
    static unsigned char translateKey(int key);

    bool initialized = false;
    bool suspended = false;
    bool frameReady = false;

    GLuint framebufferTexture = 0;
    GLuint vertexArray = 0;
    GLuint vertexBuffer = 0;
    Shader screenShader;

    std::array<std::string, 3> arguments;
    std::array<char *, 3> argumentPointers{};
    std::array<bool, 256> heldDoomKeys{};
    std::deque<KeyEvent> keyEvents;

    std::chrono::steady_clock::time_point startTime{};
    std::chrono::steady_clock::time_point suspendedAt{};
    std::chrono::steady_clock::duration pausedDuration{};
};
