#include "gpu_device.h"

#include <vector>
#include <fmt/format.h>

GpuDevice* GpuDevice::GetInstance()
{
    static GpuDevice obj;
    return &obj;
}

bool GpuDevice::Init(SDL_Window* parent_window)
{
    vk_resource_ = std::make_unique<GpuResource>();
    if (!vk_resource_->Init(parent_window))
    {
        vk_resource_.reset();
        return false;
    }
    return true;
}

void GpuDevice::Uninit()
{
    if (vk_resource_)
    {
        vk_resource_.reset();
    }
}