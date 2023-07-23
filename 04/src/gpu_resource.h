#pragma once

#include <optional>
#include <vulkan/vulkan.hpp>
#include <SDL2/SDL_video.h>

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool isComplete()
	{
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};


class GpuResource final
{
public:
	GpuResource() = default;
	~GpuResource();

	bool Init(SDL_Window* parent_window);
	void UnInit();

private:
	bool _CreateInstatce();
	bool _IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface);
	bool _PickPhysicalDevice();
	QueueFamilyIndices _findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
	bool _CreateLogicDevice();
	bool _CreateSurface();

public:
	SDL_Window* parent_window_ = nullptr;
	VkInstance vk_instance_ = VK_NULL_HANDLE;	///< vulkanʵ��
	VkPhysicalDevice vk_physicaldevice_ = VK_NULL_HANDLE;	///< �Կ��豸
	VkDevice vk_device_ = VK_NULL_HANDLE;	///< �߼��豸
	VkQueue vk_graphics_queue_ = VK_NULL_HANDLE;	///< ͼ���Կ�����

	VkSurfaceKHR vk_surface_ = VK_NULL_HANDLE;	///< ����
	VkQueue vk_present_queue_ = VK_NULL_HANDLE;	///<
};