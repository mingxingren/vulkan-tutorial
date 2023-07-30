#include "gpu_program.h"

#include <fstream>
#include <vector>
#include <fmt/format.h>

GpuProgram* GpuProgram::GetInstance()
{
    static GpuProgram obj;
    return &obj;
}

bool GpuProgram::Init(SDL_Window* parent_window)
{
    vk_resource_ = std::make_unique<GpuResource>();
    if (!vk_resource_->Init(parent_window))
    {
        vk_resource_.reset();
        return false;
    }

    std::vector<char> vertex_shader = _ReadFile("shader/vert.spv");
    std::vector<char> fragment_shader = _ReadFile("shader/frag.spv");

    triangle_shader_ = std::make_unique<TriangleShader>(vk_resource_.get());
    TriangleShader::ShaderParam param;
    param.vertex_shader = std::move(vertex_shader);
    param.pixel_shader = std::move(fragment_shader);
    param.viewport = vk_resource_->vk_swapchain_image_extent;
    if (!triangle_shader_->Init(param))
    {
        return false;
    }

    if (!_CreateFrameBuffer())
    {
        return false;
    }

    if (!_CreateCommandPool())
    {
        return false;
    }

    if (!_CreateCommandBuffer())
    {
        return false;
    }
    return true;
}

void GpuProgram::Uninit()
{
    if (vk_commandpool_ != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(vk_resource_->vk_device_, vk_commandpool_, nullptr);
        vk_commandpool_ = VK_NULL_HANDLE;
    }

    if (vk_swapchain_framebuffers_.size() > 0)
    {
        for (auto& index : vk_swapchain_framebuffers_)
        {
            vkDestroyFramebuffer(vk_resource_->vk_device_, index, nullptr);
        }
        vk_swapchain_framebuffers_.clear();
    }

    if (vk_resource_)
    {
        vk_resource_.reset();
    }
}

void GpuProgram::DrawFrame()
{

}

std::vector<char> GpuProgram::_ReadFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}

bool GpuProgram::_CreateFrameBuffer()
{    
    for (uint32_t i = 0; i < vk_resource_->vk_swapchain_image_views.size(); i++)
    {
        VkFramebuffer frame_buffer;

        std::vector<VkImageView> attachments = { vk_resource_->vk_swapchain_image_views[i] };
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = triangle_shader_->vk_render_pass_;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.height = vk_resource_->vk_swapchain_image_extent.height;
        framebufferInfo.width = vk_resource_->vk_swapchain_image_extent.width;
        framebufferInfo.layers = 1;

        VkResult result = vkCreateFramebuffer(vk_resource_->vk_device_, &framebufferInfo, nullptr, &frame_buffer);
        if (result != VK_SUCCESS)
        {
            return false;
        }
        vk_swapchain_framebuffers_.push_back(frame_buffer);
    }

    return true;
}

bool GpuProgram::_CreateCommandPool()
{
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = vk_resource_->vk_graphics_family_;
    poolInfo.pNext = nullptr;

    VkResult ret = vkCreateCommandPool(vk_resource_->vk_device_, &poolInfo, nullptr, &vk_commandpool_);
    if (ret != VK_SUCCESS)
    {
        fmt::print("vkCreateCommandPool return error: {} \n", ret);
        return false;
    }

    return true;
}

bool GpuProgram::_CreateCommandBuffer()
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = vk_commandpool_;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    allocInfo.pNext = nullptr;

    VkResult result = vkAllocateCommandBuffers(vk_resource_->vk_device_, &allocInfo, &vk_commandbuffer_);
    if (result != VK_SUCCESS)
    {
        fmt::print("vkAllocateCommandBuffers return error: {}\n", result);
        return false;
    }
    return true;
}

void GpuProgram::_RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = nullptr;

    VkResult ret = vkBeginCommandBuffer(commandBuffer, &beginInfo);
    if (ret != VK_SUCCESS)
    {
        fmt::print("vkBeginCommandBuffer return error: {}\n", ret);
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = triangle_shader_->vk_render_pass_;
    renderPassInfo.framebuffer = vk_swapchain_framebuffers_[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = vk_resource_->vk_swapchain_image_extent;

    VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, triangle_shader_->vk_graphics_pipeline_);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(vk_resource_->vk_swapchain_image_extent.width);
    viewport.height = static_cast<float>(vk_resource_->vk_swapchain_image_extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = vk_resource_->vk_swapchain_image_extent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    ret = vkEndCommandBuffer(commandBuffer);
    if (ret != VK_SUCCESS)
    {
        fmt::print("vkEndCommandBuffer return error: {} \n", ret);
    }
}