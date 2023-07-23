#include "gpu_resource.h"
#include <cassert>
#include <set>
#include <Windows.h>
#ifdef _WIN32
#include <vulkan/vulkan_win32.h>
#endif
#include <fmt/format.h>
#include <SDL2/SDL_vulkan.h>
#include <SDL2/SDL_syswm.h>

namespace {
#define CHECK_OR_RETURN_FALSE(ret) \
if (!ret) \
{\
    fmt::print("{} call fail", #ret); \
    return false;\
}

#define IF_VK_RETURN_FAIL(ret, function, return_value) \
if (ret != VK_SUCCESS) \
{\
    fmt::print("{} call fail, error: {}\n", #function, ret); \
    return return_value;\
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
    CHECK_OR_RETURN_FALSE(_CreateSurface());
	CHECK_OR_RETURN_FALSE(_PickPhysicalDevice());
    CHECK_OR_RETURN_FALSE(_CreateLogicDevice());

	return true;
}

void GpuResource::UnInit()
{
    if (vk_device_ != VK_NULL_HANDLE)
    {
        vkDestroyDevice(vk_device_, nullptr);
        vk_device_ = VK_NULL_HANDLE;
    }

    if (vk_physicaldevice_ != VK_NULL_HANDLE)
    {
        vk_physicaldevice_ = VK_NULL_HANDLE;
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

    createInfo.enabledExtensionCount = extension_count;
    createInfo.ppEnabledExtensionNames = extension_names.data();

    createInfo.enabledLayerCount = 0;

    VkResult result = vkCreateInstance(&createInfo, nullptr, &vk_instance_);
    IF_VK_RETURN_FAIL(result, vkCreateInstance, false);

    fmt::print("vkCreateInstance call success\n");
    return true;
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
        QueueFamilyIndices families =  _findQueueFamilies(device, surface);
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

QueueFamilyIndices GpuResource::_findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
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

        if (persentSupport != 0)
        {
            indices.presentFamily = i;
        }
        
        i++;
    }

    return indices;
}

bool GpuResource::_CreateLogicDevice()
{
    QueueFamilyIndices indices = _findQueueFamilies(vk_physicaldevice_, vk_surface_);
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

    VkPhysicalDeviceFeatures deviceFeature{};
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pEnabledFeatures = &deviceFeature;
    createInfo.enabledExtensionCount = 0;

    // 验证层
    createInfo.enabledLayerCount = 0;

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