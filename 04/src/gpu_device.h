#pragma once

#include <memory>
#include <vulkan/vulkan.hpp>
#include "gpu_resource.h"

class GpuDevice
{
public:
    ~GpuDevice() = default;
    static GpuDevice* GetInstance();
    bool Init(SDL_Window* parent_window);
    void Uninit();

private:
    std::unique_ptr<GpuResource> vk_resource_ = nullptr;
};