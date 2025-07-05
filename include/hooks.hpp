#ifndef HOOKS_HPP
#define HOOKS_HPP

#include <vulkan/vulkan_core.h>

#include <utility>

namespace Hooks {

    /// Vulkan device information structure.
    struct DeviceInfo {
        VkDevice device;
        VkPhysicalDevice physicalDevice;
        std::pair<uint32_t, VkQueue> queue; // graphics family
        uint64_t frameGen; // amount of frames to generate
    };

    ///
    /// Install overrides for hooked Vulkan functions.
    ///
    void initialize();

}

#endif // HOOKS_HPP
