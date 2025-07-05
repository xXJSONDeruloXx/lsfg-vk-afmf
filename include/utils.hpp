#ifndef UTILS_HPP
#define UTILS_HPP

#include <vector>
#include <vulkan/vulkan_core.h>

#include <utility>

namespace Utils {

    ///
    /// Find a queue in the physical device that supports the given queue flags.
    ///
    /// @param device The Vulkan device to use for queue retrieval.
    /// @param physicalDevice The physical device to search in.
    /// @param desc The device creation info, used to determine enabled queue families.
    /// @param flags The queue flags to search for (e.g., VK_QUEUE_GRAPHICS_BIT).
    /// @return Pair of queue family index and queue handle.
    ///
    /// @throws LSFG::vulkan_error if no suitable queue is found.
    ///
    std::pair<uint32_t, VkQueue> findQueue(VkDevice device, VkPhysicalDevice physicalDevice,
        VkDeviceCreateInfo* desc, VkQueueFlags flags);

    ///
    /// Ensure a list of extensions is present in the given array.
    ///
    /// @param extensions The array of extensions to check.
    /// @param requiredExtensions The list of required extensions to ensure are present.
    ///
    std::vector<const char*> addExtensions(const char* const* extensions, size_t count,
        const std::vector<const char*>& requiredExtensions);

    ///
    /// Copy an image from source to destination in a command buffer.
    ///
    /// @param buf The command buffer to record the copy operation into.
    /// @param src The source image to copy from.
    /// @param dst The destination image to copy to.
    /// @param width The width of the image to copy.
    /// @param height The height of the image to copy.
    /// @param pre The pipeline stage to wait on.
    /// @param post The pipeline stage to provide after the copy.
    /// @param makeSrcPresentable If true, the source image will be made presentable after the copy.
    /// @param makeDstPresentable If true, the destination image will be made presentable after the copy.
    ///
    void copyImage(VkCommandBuffer buf,
            VkImage src, VkImage dst,
            uint32_t width, uint32_t height,
            VkPipelineStageFlags pre, VkPipelineStageFlags post,
            bool makeSrcPresentable, bool makeDstPresentable);

}

#endif // UTILS_HPP
