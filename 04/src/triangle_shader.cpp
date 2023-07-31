#include "triangle_shader.h"

#include <fmt/format.h>

TriangleShader::TriangleShader(GpuResource* device)
{
	vk_resource_ = device;
}

TriangleShader::~TriangleShader()
{
	if (vk_graphics_pipeline_ != VK_NULL_HANDLE)
	{
		vkDestroyPipeline(vk_resource_->vk_device_, vk_graphics_pipeline_, nullptr);
		vk_graphics_pipeline_ = VK_NULL_HANDLE;
	}

	if (vk_render_pass_ != VK_NULL_HANDLE)
	{
		vkDestroyRenderPass(vk_resource_->vk_device_, vk_render_pass_, nullptr);
		vk_render_pass_ = VK_NULL_HANDLE;
	}

	if (vk_pipeline_layout_ == VK_NULL_HANDLE)
	{
		vkDestroyPipelineLayout(vk_resource_->vk_device_, vk_pipeline_layout_, nullptr);
		vk_pipeline_layout_ = VK_NULL_HANDLE;
	}
}

bool TriangleShader::Init(const ShaderParam& param)
{
	std::optional<VkShaderModule> vertex_module = _CreateShaderModule(param.vertex_shader);
	std::optional<VkShaderModule> pixel_module = _CreateShaderModule(param.pixel_shader);

	bool result = false;
	if (!vertex_module || !pixel_module)
	{
		goto DESTROY_SHADER_RESOURCE;
	}

	if (!_CreateRenderPass())
	{
		goto DESTROY_SHADER_RESOURCE;
	}

	result = _CreatePipeline(vertex_module.value(), pixel_module.value(), param.viewport);

DESTROY_SHADER_RESOURCE:
	if (vertex_module)
	{
		_DestroyShaderModule(vertex_module.value());
	}
	
	if (pixel_module)
	{
		_DestroyShaderModule(pixel_module.value());
	}
	return result;
}

std::optional<VkShaderModule> TriangleShader::_CreateShaderModule(const std::vector<char>& shader)
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = shader.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(shader.data());

	VkShaderModule shaderModule;
	VkResult result = vkCreateShaderModule(vk_resource_->vk_device_, &createInfo, nullptr, &shaderModule);
	if (result != VK_SUCCESS)
	{
		return {};
	}
	return shaderModule;
}

void TriangleShader::_DestroyShaderModule(VkShaderModule shader)
{
	vkDestroyShaderModule(vk_resource_->vk_device_, shader, nullptr);
}

bool TriangleShader::_CreatePipeline(VkShaderModule vertex_shader, VkShaderModule pixel_shader, 
									const VkExtent2D& image_exent)
{
	// 创建着色器
	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertex_shader;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = pixel_shader;
	fragShaderStageInfo.pName = "main";

	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	shaderStages.push_back(vertShaderStageInfo);
	shaderStages.push_back(fragShaderStageInfo);

	// 创建顶点输入描述
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexAttributeDescriptionCount = 0;
	vertexInputInfo.pVertexAttributeDescriptions = nullptr;
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.pVertexBindingDescriptions = nullptr;

	// 创建图元描述
	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	// 视口
	VkViewport viewport{};
	viewport.height = static_cast<float>(image_exent.height);
	viewport.width = static_cast<float>(image_exent.width);
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	// 裁剪区域
	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = image_exent;

	// 创建可变的管线状态
	std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.pDynamicStates = dynamicStates.data();
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	//创建光栅化
	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT; // 开启背面提出模式
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;	// 顺时针
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasSlopeFactor = 0.0f;

	// 多重采样
	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;

	// 深度测试 和 模板测试

	// 混合
	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.blendEnable = VK_FALSE; // 禁用颜色混合
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
		| VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	//colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	//colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
	//colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	//colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	//colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	//colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	// 创建空的uniform
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0;
	pipelineLayoutInfo.pSetLayouts = nullptr;
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;
	VkResult ret = vkCreatePipelineLayout(vk_resource_->vk_device_, &pipelineLayoutInfo, nullptr, &vk_pipeline_layout_);
	if (ret != VK_SUCCESS)
	{
		fmt::print("vkCreatePipelineLayout return error: {}\n", ret);
		return false;
	}

	// 创建图形管线
	VkGraphicsPipelineCreateInfo pipelineInfo;
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.pNext = nullptr;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = nullptr;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.pTessellationState = nullptr;
	pipelineInfo.layout = vk_pipeline_layout_;
	pipelineInfo.renderPass = vk_render_pass_;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	ret = vkCreateGraphicsPipelines(vk_resource_->vk_device_, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &vk_graphics_pipeline_);
	if (ret != VK_SUCCESS)
	{
		fmt::print("vkCreateGraphicsPipelines return error: {} \n", ret);
		return false;
	}

	return true;
}

bool TriangleShader::_CreateRenderPass()
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = vk_resource_->vk_swapchain_image_format;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	
	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;
	VkResult ret = vkCreateRenderPass(vk_resource_->vk_device_, &renderPassInfo, nullptr, &vk_render_pass_);
	if (ret != VK_SUCCESS)
	{
		fmt::print("vkCreateRenderPass return error: {}\n", ret);
		return false;
	}
	
	return true;
}