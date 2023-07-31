#pragma once

#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>
#include "gpu_resource.h"
#include "triangle_shader.h"

class GpuProgram
{
public:
    ~GpuProgram() = default;
    static GpuProgram* GetInstance();
    bool Init(SDL_Window* parent_window);
    void Uninit();
    void DrawFrame();

private:
    std::vector<char> _ReadFile(const std::string& filename);
    bool _CreateFrameBuffer();
    bool _CreateCommandPool();
    bool _CreateCommandBuffer();
    void _RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    bool _CreateSyncObjects();

private:
    std::unique_ptr<GpuResource> vk_resource_ = nullptr;
    std::unique_ptr<TriangleShader> triangle_shader_ = nullptr;
    std::vector<VkFramebuffer> vk_swapchain_framebuffers_;

    VkCommandPool vk_commandpool_ = VK_NULL_HANDLE;
    VkCommandBuffer vk_commandbuffer_ = VK_NULL_HANDLE;

    // 创建信号量
    VkSemaphore vk_imageavailable_semaphore_ = VK_NULL_HANDLE;
    VkSemaphore vk_renderfinshed_semaphore_ = VK_NULL_HANDLE;
    VkFence vk_inflight_fence_ = VK_NULL_HANDLE;
};