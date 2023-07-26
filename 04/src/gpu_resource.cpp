#include "gpu_resource.h"
#include <algorithm>
#include <cassert>
#include <limits>
#include <set>
#include <Windows.h>
#undef max
#ifdef _WIN32
#include <vulkan/vulkan_win32.h>
#endif
#include <fmt/format.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_vulkan.h>

namespace {
#define CHECK_OR_RETURN_FALSE(ret) \
if (!ret) \
{\
    fmt::print("function {} - line {} : {} call fail", __FUNCTION__, __LINE__, #ret); \
    return false;\
}

#define IF_VK_RETURN_FAIL(ret, function, return_value) \
if (ret != VK_SUCCESS) \
{\
    fmt::print("{} call fail, error: {}\n", #function, ret); \
    return return_value;\
}
}

static VKAPI_ATTR VkBool32 VKAPI_CALL 
DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, 
            VkDebugUtilsMessageTypeFlagsEXT messageType, 
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, 
            void* pUserData) 
{
    fmt::print("validation layer: {}\n", pCallbackData->pMessage);
    return VK_FALSE;
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, 
                                    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
                                    const VkAllocationCallbacks* pAllocator, 
                                    VkDebugUtilsMessengerEXT* pDebugMessenger) 
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, 
                                VkDebugUtilsMessengerEXT debugMessenger, 
                                const VkAllocationCallbacks* pAllocator) 
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) 
    {
        func(instance, debugMessenger, pAllocator);
    }
}

GpuResource::~GpuResource()
{
	UnInit();
}

bool GpuResource::Init(SDL_Window* parent_window)
{
	parent_window_ = parent_window;
    CHECK_OR_RETURN_FALSE(_CreateInstatce());
    //CHECK_OR_RETURN_FALSE(_SetupDebugMessenger());
    CHECK_OR_RETURN_FALSE(_CreateSurface());
	CHECK_OR_RETURN_FALSE(_PickPhysicalDevice());
    // 检测设备是否支持交换链
    CHECK_OR_RETURN_FALSE(_CheckDeviceExtensionSupport(vk_physicaldevice_, VK_KHR_SWAPCHAIN_EXTENSION_NAME));
    CHECK_OR_RETURN_FALSE(_CreateLogicDevice());
    CHECK_OR_RETURN_FALSE(_CreateSwapChain());
    CHECK_OR_RETURN_FALSE(_CreateImageViews());

	return true;
}

void GpuResource::UnInit()
{
    for (auto& index : vk_swapchain_image_views)
    {
        vkDestroyImageView(vk_device_, index, nullptr);
    }
    vk_swapchain_image_views.clear();

    if (vk_swap_chain_ != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(vk_device_, vk_swap_chain_, nullptr);
        vk_swap_chain_ = VK_NULL_HANDLE;
    }

    if (vk_device_ != VK_NULL_HANDLE)
    {
        vkDestroyDevice(vk_device_, nullptr);
        vk_device_ = VK_NULL_HANDLE;
    }

    if (vk_physicaldevice_ != VK_NULL_HANDLE)
    {
        vk_physicaldevice_ = VK_NULL_HANDLE;
    }

    if (vk_debug_messenger_ != VK_NULL_HANDLE)
    {
        DestroyDebugUtilsMessengerEXT(vk_instance_, vk_debug_messenger_, nullptr);
    }

    if (vk_surface_ != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(vk_instance_, vk_surface_, nullptr);
        vk_surface_ = VK_NULL_HANDLE;
    }

	if (vk_instance_ != VK_NULL_HANDLE)
	{
		vkDestroyInstance(vk_instance_, nullptr);
        vk_instance_ = VK_NULL_HANDLE;
	}
}

bool GpuResource::_CreateInstatce()
{
    if (is_debug_)
    {
        if (!_CheckValidationLayerSupport()) {
            return false;
        }
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan Toturial";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    uint32_t extension_count = 0;
    bool ret = SDL_Vulkan_GetInstanceExtensions(parent_window_, &extension_count, nullptr);
    if (!ret)
    {
        return false;
    }
    std::vector<const char*> extension_names(extension_count);
    ret = SDL_Vulkan_GetInstanceExtensions(parent_window_, &extension_count, extension_names.data());
    if (!ret)
    {
        return false;
    }
    
    if (is_debug_)
    {
        extension_names.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    createInfo.enabledExtensionCount = extension_count;
    createInfo.ppEnabledExtensionNames = extension_names.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (is_debug_) 
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validation_layers_.size());
        createInfo.ppEnabledLayerNames = validation_layers_.data();
        _PopulateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    }
    else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }
    createInfo.enabledLayerCount = 0;

    VkResult result = vkCreateInstance(&createInfo, nullptr, &vk_instance_);
    IF_VK_RETURN_FAIL(result, vkCreateInstance, false);

    fmt::print("vkCreateInstance call success\n");
    return true;
}

bool GpuResource::_SetupDebugMessenger()
{
    if (!is_debug_) 
    {
        return true;
    }

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    _PopulateDebugMessengerCreateInfo(createInfo);

    VkResult ret = CreateDebugUtilsMessengerEXT(vk_instance_, &createInfo, nullptr, &vk_debug_messenger_);
    IF_VK_RETURN_FAIL(ret, CreateDebugUtilsMessengerEXT, false)
    return true;
}

bool GpuResource::_CheckValidationLayerSupport()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validation_layers_) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

void GpuResource::_PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = DebugCallback;
}

bool GpuResource::_IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU 
        || deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {   // 集成显卡
        QueueFamilyIndices families =  _FindQueueFamilies(device, surface);
        return families.isComplete();
    }
    else {
        return false;
    }
}

bool GpuResource::_PickPhysicalDevice()
{
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(vk_instance_, &device_count, nullptr);
    if (device_count == 0)
    {
        return false;
    }
    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(vk_instance_, &device_count, devices.data());

    for (auto index : devices)
    {
        if (_IsDeviceSuitable(index, vk_surface_))
        {
            vk_physicaldevice_ = index;
            break;
        }
    }

    if (vk_physicaldevice_ == VK_NULL_HANDLE)
    {
        return false;
    }

    return true;
}

QueueFamilyIndices GpuResource::_FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    assert((device != VK_NULL_HANDLE) && "VkPhysicalDevice param cant be empty!");
    assert((surface != VK_NULL_HANDLE) && "VkSurfaceKHR param cant be empty!");

    QueueFamilyIndices indices;

    // 获取显卡协议
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    uint32_t i = 0;

    for (const auto& index : queueFamilies)
    {
        if (index.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamily = i;
        }

        VkBool32 persentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &persentSupport);

        if (persentSupport != false)
        {
            indices.presentFamily = i;
        }
        
        i++;
    }

    return indices;
}

bool GpuResource::_CreateLogicDevice()
{
    QueueFamilyIndices indices = _FindQueueFamilies(vk_physicaldevice_, vk_surface_);
    if (!indices.isComplete())
    {
        return false;
    }
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    float queuePriprity = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriprity;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    VkPhysicalDeviceFeatures deviceFeature{};
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    // 开启设备扩展
    createInfo.pEnabledFeatures = &deviceFeature;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    // 验证层
    if (is_debug_)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validation_layers_.size());
        createInfo.ppEnabledLayerNames = validation_layers_.data();
    }
    else {
        createInfo.enabledLayerCount = 0;
    }

    VkResult ret = vkCreateDevice(vk_physicaldevice_, &createInfo, nullptr, &vk_device_);
    IF_VK_RETURN_FAIL(ret, vkCreateDevice, false)

    vkGetDeviceQueue(vk_device_, indices.graphicsFamily.value(), 0, &vk_graphics_queue_);
    vkGetDeviceQueue(vk_device_, indices.presentFamily.value(), 0, &vk_present_queue_);
    return true;
}

bool GpuResource::_CreateSurface()
{
#ifdef _WIN32
    SDL_SysWMinfo system_info;
    SDL_VERSION(&system_info.version);
    if (SDL_GetWindowWMInfo(parent_window_, &system_info) == 0)
    {
        fmt::print("SDL_Window cant get hwnd\n");
        return false;
    }
    HWND tmp_hwnd = system_info.info.win.window;
    VkWin32SurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hwnd = tmp_hwnd;
    createInfo.hinstance = GetModuleHandle(nullptr);

    VkResult ret = vkCreateWin32SurfaceKHR(vk_instance_, &createInfo, nullptr, &vk_surface_);
    IF_VK_RETURN_FAIL(ret, vkCreateWin32SurfaceKHR, false)
    return true;
#else
    return false;
#endif
}

bool GpuResource::_CheckDeviceExtensionSupport(VkPhysicalDevice device, const std::string& extension_name)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    for (const auto& index : availableExtensions)
    {
        if (index.extensionName == extension_name)
        {
            return true;
        }
    }

    return false;
}

bool GpuResource::_CreateSwapChain()
{
    SwapChainSupportDetails swap_chain_support = _QuerySwapChainSupport(vk_physicaldevice_, vk_surface_);
    if (swap_chain_support.formats.empty() || swap_chain_support.presentModes.empty())
    {
        return false;
    }

    // 选择可支持的交换链
    std::optional<VkSurfaceFormatKHR> surfaceFormat =  _ChooseSwapSurfaceFormat(swap_chain_support.formats);

    // 选择呈现模式
    std::optional<VkPresentModeKHR> presentMode = _ChooseSwapPresentMode(swap_chain_support.presentModes);

    // 获取视口输出大小
    VkExtent2D extent = _ChooseSwapExtent(swap_chain_support.capabilities);

    if (!surfaceFormat.has_value() || !presentMode.has_value())
    {
        return false;
    }
    
    // 多申请一张图片
    uint32_t imageCount = swap_chain_support.capabilities.minImageCount + 1;
    if (swap_chain_support.capabilities.maxImageCount > 0 && imageCount > swap_chain_support.capabilities.maxImageCount)
    {
        imageCount = swap_chain_support.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = vk_surface_;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.value().format;
    createInfo.imageColorSpace = surfaceFormat.value().colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    QueueFamilyIndices indices = _FindQueueFamilies(vk_physicaldevice_, vk_surface_);
    CHECK_OR_RETURN_FALSE(indices.isComplete())
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value()};

    if (indices.graphicsFamily != indices.presentFamily)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    createInfo.preTransform = swap_chain_support.capabilities.currentTransform; // 画面翻转
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode.value();
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE; // 记录旧的交换链

    VkResult ret = vkCreateSwapchainKHR(vk_device_, &createInfo, nullptr, &vk_swap_chain_);
    IF_VK_RETURN_FAIL(ret, vkCreateSwapchainKHR, false)

    uint32_t swapchainImageCount = 0;
    vkGetSwapchainImagesKHR(vk_device_, vk_swap_chain_, &swapchainImageCount, nullptr);
    vk_swapchain_images_.resize(swapchainImageCount);
    vkGetSwapchainImagesKHR(vk_device_, vk_swap_chain_, &swapchainImageCount, vk_swapchain_images_.data());

    vk_swapchain_image_format = surfaceFormat.value().format;
    vk_swapchain_image_extent = extent;

    return true;
}

SwapChainSupportDetails GpuResource::_QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    SwapChainSupportDetails details;
    // 获取 Surface 参数
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, vk_surface_, &details.capabilities);

    // 查询支持的格式
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, vk_surface_, &formatCount, nullptr);
    if (formatCount > 0)
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, vk_surface_, &formatCount, details.formats.data());
    }

    // 查询支持的呈现模式
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, vk_surface_, &presentModeCount, nullptr);
    if (presentModeCount > 0)
    {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, vk_surface_, &presentModeCount, details.presentModes.data());
    }

    return details;
}

std::optional<VkSurfaceFormatKHR> GpuResource::_ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    std::optional<VkSurfaceFormatKHR> result;

    for (const auto& availableFormat : availableFormats)
    {
        if (availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR && availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB)
        {
            result = availableFormat;
            break;
        }
    }

    return result;
}

std::optional<VkPresentModeKHR> GpuResource::_ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    std::optional<VkPresentModeKHR> result;

    std::set<VkPresentModeKHR> present_mode(availablePresentModes.begin(), availablePresentModes.end());

    // 呈现策略
    if (present_mode.contains(VK_PRESENT_MODE_MAILBOX_KHR))
    {
        // 开垂直同步 但新帧会替代等待队列里帧 减少了等待事件
        result = VK_PRESENT_MODE_MAILBOX_KHR;
    }
    else if (present_mode.contains(VK_PRESENT_MODE_FIFO_KHR))
    {
        // 开垂直同步 并显示按序列显示
        result = VK_PRESENT_MODE_FIFO_KHR;
    }
    else if (present_mode.contains(VK_PRESENT_MODE_IMMEDIATE_KHR))
    {
        // 立即模式 不开垂直同步
        result = VK_PRESENT_MODE_IMMEDIATE_KHR;
    }

    return result;
}

VkExtent2D GpuResource::_ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
    VkExtent2D result;

    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }
    else {
        int32_t width = 0;
        int32_t height = 0;
        SDL_Vulkan_GetDrawableSize(parent_window_, 
                                reinterpret_cast<int32_t*>(&result.width), 
                                reinterpret_cast<int32_t*>(&result.height));


        result.width = std::clamp(result.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        result.height = std::clamp(result.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    }

    return result;
}

bool GpuResource::_CreateImageViews()
{
    vk_swapchain_image_views.resize(vk_swapchain_images_.size());
    for (size_t i = 0; i < vk_swapchain_images_.size(); i++)
    {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = vk_swapchain_images_[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = vk_swapchain_image_format;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        VkResult result = vkCreateImageView(vk_device_, &createInfo, nullptr, &vk_swapchain_image_views[i]);
        if (result != VK_SUCCESS)
        {
            fmt::print("create image view fail!\n");
        }
    }

    return true;
}