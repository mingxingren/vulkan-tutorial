#pragma once

#include <vulkan/vulkan.hpp>
#include "gpu_resource.h"

class GpuShader
{
public:
	GpuShader(GpuResource* device);
	~GpuShader();

	bool Init(const char* vertex_shader, const char* pixel_shader);

private:
	GpuResource* vk_resource_ = nullptr;
};