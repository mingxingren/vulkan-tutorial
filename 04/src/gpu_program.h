#pragma once

#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>
#include "gpu_resource.h"
#include "gpu_shader.h"

class GpuProgram
{
public:
    ~GpuProgram() = default;
    static GpuProgram* GetInstance();
    bool Init(SDL_Window* parent_window);
    void Uninit();

private:
    std::vector<char> _ReadFile(const std::string& filename);

private:
    std::unique_ptr<GpuResource> vk_resource_ = nullptr;
    std::unique_ptr<GpuShader> triangle_shader_ = nullptr;
};