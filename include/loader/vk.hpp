#ifndef VK_HPP
#define VK_HPP

#include <vulkan/vulkan.h>

#include <string>

//
// Similar to the dynamic loader, the Vulkan loader replaces the standard
// vkGetInstanceProcAddr and vkGetDeviceProcAddr functions.
//
// One thing that should be noted, is that not every application uses the
// Vulkan loader for every method. On Linux it's not unusual to see dlsym
// being used for vulkan functions, so make sure to register the same
// symbol on both loaders.
//

namespace Loader::VK {

    ///
    /// Initialize the Vulkan loader.
    ///
    void initialize();

    ///
    /// Register a symbol to the Vulkan loader.
    ///
    /// @param symbol The name of the Vulkan function to override.
    /// @param address The address of the Vulkan function.
    ///
    void registerSymbol(const std::string& symbol, void* address);

    ///
    /// Call the original vkGetInstanceProcAddr function.
    ///
    /// @param instance The (optional) Vulkan instance.
    /// @param pName The name of the function to retrieve.
    /// @return The address of the function, or nullptr if not found.
    ///
    PFN_vkVoidFunction ovkGetInstanceProcAddr(VkInstance instance, const char* pName);

    ///
    /// Call the original vkGetDeviceProcAddr function.
    ///
    /// @param device The Vulkan device.
    /// @param pName The name of the function to retrieve.
    /// @return The address of the function, or nullptr if not found.
    ///
    PFN_vkVoidFunction ovkGetDeviceProcAddr(VkDevice device, const char* pName);

}

/// Modified version of the vkGetInstanceProcAddr function.
extern "C" PFN_vkVoidFunction myvkGetInstanceProcAddr(VkInstance instance, const char* pName);
/// Modified version of the vkGetDeviceProcAddr function.
extern "C" PFN_vkVoidFunction myvkGetDeviceProcAddr(VkDevice device, const char* pName);

#endif // VK_HPP
