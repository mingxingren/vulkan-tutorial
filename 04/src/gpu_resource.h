#pragma once

#include <optional>
#include <vulkan/vulkan.hpp>
#include <SDL2/SDL_video.h>

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct QueueFamilyIndices 
{
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
	bool _CheckDeviceExtensionSupport(VkPhysicalDevice device, std::string extension_name);
	SwapChainSupportDetails _QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
	std::optional<VkSurfaceFormatKHR> _ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	std::optional<VkPresentModeKHR> _ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D _ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
public:
	SDL_Window* parent_window_ = nullptr;
	VkInstance vk_instance_ = VK_NULL_HANDLE;	///< vulkan实例
	VkPhysicalDevice vk_physicaldevice_ = VK_NULL_HANDLE;	///< 显卡设备
	VkDevice vk_device_ = VK_NULL_HANDLE;	///< 逻辑设备
	VkQueue vk_graphics_queue_ = VK_NULL_HANDLE;	///< 图形显卡队列

	VkSurfaceKHR vk_surface_ = VK_NULL_HANDLE;	///< 表面
	VkQueue vk_present_queue_ = VK_NULL_HANDLE;	///<
};