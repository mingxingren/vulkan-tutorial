#pragma once

#include <vulkan/vulkan.hpp>
#include "gpu_resource.h"

class TriangleShader
{
public:
	struct ShaderParam
	{
		std::vector<char> vertex_shader;
		std::vector<char> pixel_shader;
		VkExtent2D viewport;
	};

public:
	TriangleShader(GpuResource* device);
	~TriangleShader();

	bool Init(const ShaderParam& param);

private:
	std::optional<VkShaderModule> _CreateShaderModule(const std::vector<char>& shader);
	void _DestroyShaderModule(VkShaderModule shader);
	bool _CreatePipeline(VkShaderModule vertex_shader, VkShaderModule pixel_shader, 
						const VkExtent2D& image_exent);
	bool _CreateRenderPass();

public:
	VkRenderPass vk_render_pass_ = VK_NULL_HANDLE;
	VkPipeline vk_graphics_pipeline_ = VK_NULL_HANDLE;

private:
	GpuResource* vk_resource_ = nullptr;
	VkPipelineLayout vk_pipeline_layout_ = VK_NULL_HANDLE;
};