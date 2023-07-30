#include "app.h"

#include <fmt/format.h>

#include "gpu_program.h"

Application::~Application()
{
    if (window_)
    {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }

    SDL_Quit();
}

Application* Application::GetInstance()
{
    static Application obj;
    return &obj;
}

bool Application::Init()
{
    SDL_Init(SDL_INIT_EVERYTHING);
    return true;
}

int32_t Application::Exec()
{

    int32_t window_flag = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN;

    SDL_Window* tmp_window = SDL_CreateWindow(title_.c_str(), 
                                            SDL_WINDOWPOS_CENTERED, 
                                            SDL_WINDOWPOS_CENTERED, 
                                            800, 600,
                                            window_flag);
    if (tmp_window == nullptr)
    {
        fmt::print("SDL_CreateWindows call fail! error: {}\n", SDL_GetError());
        return -1;
    }

    window_ = tmp_window;

    if (!GpuProgram::GetInstance()->Init(window_))
    {
        return false;
    }

    while (true)
    {
        SDL_Event event;
        int32_t ret = SDL_PollEvent(&event);

        if (ret != 0) {
            if (event.type == SDL_QUIT) 
            {
                break;
            }
        }
        else{
            SDL_Delay(2);
        }

        GpuProgram::GetInstance()->DrawFrame();
    }

    GpuProgram::GetInstance()->Uninit();

    return 0;
}