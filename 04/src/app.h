#pragma once

#include <memory>
#include <string>
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

class Application
{
public:
    static Application* GetInstance();
    bool Init();
    int32_t Exec();

private:
    Application() = default;
    ~Application();

private:
    SDL_Window* window_ = nullptr;
    std::string title_ = "hello vulkan";
};