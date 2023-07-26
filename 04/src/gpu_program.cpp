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

    std::vector<char> vertex_shader = _ReadFile("vert.spv");
    std::vector<char> fragment_shader = _ReadFile("frag.spv");

    triangle_shader_ = std::make_unique<GpuShader>(vk_resource_.get());
    if (!triangle_shader_->Init(vertex_shader.data(), fragment_shader.data()))
    {
        return false;
    }

    return true;
}

void GpuProgram::Uninit()
{
    if (vk_resource_)
    {
        vk_resource_.reset();
    }
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