#pragma once

#include <optional>
#include <string>
#include <vector>
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

struct SwapChainSupportDetails 
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

/**
 * @brief 该类管理一些通用资源 
 */
class GpuResource final
{
public:
	GpuResource() = default;
	~GpuResource();

	bool Init(SDL_Window* parent_window);
	void UnInit();

private:
	bool _CreateInstatce();
	bool _SetupDebugMessenger();
	bool _CheckValidationLayerSupport();
	void _PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	bool _IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface);
	bool _PickPhysicalDevice();
	QueueFamilyIndices _FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);

	// 创建逻辑设备 和 窗体表面
	bool _CreateLogicDevice();
	bool _CreateSurface();
	bool _CheckDeviceExtensionSupport(VkPhysicalDevice device, const std::string& extension_name);

	// 交换链创建
	bool _CreateSwapChain();
	SwapChainSupportDetails _QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
	std::optional<VkSurfaceFormatKHR> _ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	std::optional<VkPresentModeKHR> _ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D _ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

private:
	SDL_Window* parent_window_ = nullptr;
	bool is_debug_ = true;
	std::vector<const char*> validation_layers_ = { "VK_LAYER_KHRONOS_validation" };
	VkDebugUtilsMessengerEXT vk_debug_messenger_ = VK_NULL_HANDLE;

public:
	VkInstance vk_instance_ = VK_NULL_HANDLE;	///< vulkan实例
	VkPhysicalDevice vk_physicaldevice_ = VK_NULL_HANDLE;	///< 显卡设备
	VkDevice vk_device_ = VK_NULL_HANDLE;	///< 逻辑设备
	VkQueue vk_graphics_queue_ = VK_NULL_HANDLE;	///< 图形显卡队列

	VkSurfaceKHR vk_surface_ = VK_NULL_HANDLE;	///< 窗体表面
	VkQueue vk_present_queue_ = VK_NULL_HANDLE;	///< 交换链命令队列
	VkSwapchainKHR vk_swap_chain_ = VK_NULL_HANDLE;	///< 交换链对象
	std::vector<VkImage> vk_swapchain_images_;	///< 交换链的后备缓冲
	VkFormat vk_swapchain_image_format = VK_FORMAT_UNDEFINED;	///< 交换链后备缓冲格式
	VkExtent2D vk_swapchain_image_extent = { 0, 0 };	///< 交换链后备缓冲宽高
};