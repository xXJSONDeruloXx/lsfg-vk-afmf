#include "utils.hpp"

#include <lsfg.hpp>

#include <algorithm>
#include <optional>

using namespace Utils;

std::pair<uint32_t, VkQueue> Utils::findQueue(VkDevice device, VkPhysicalDevice physicalDevice,
        VkDeviceCreateInfo* desc, VkQueueFlags flags) {
    std::vector<VkDeviceQueueCreateInfo> enabledQueues(desc->queueCreateInfoCount);
    std::copy_n(desc->pQueueCreateInfos, enabledQueues.size(), enabledQueues.data());

    uint32_t familyCount{};
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &familyCount, nullptr);
    std::vector<VkQueueFamilyProperties> families(familyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &familyCount, families.data());

    std::optional<uint32_t> idx;
    for (const auto& queueInfo : enabledQueues) {
        if ((queueInfo.queueFamilyIndex < families.size()) &&
            (families[queueInfo.queueFamilyIndex].queueFlags & flags)) {
            idx = queueInfo.queueFamilyIndex;
            break;
        }
    }
    if (!idx.has_value())
        throw LSFG::vulkan_error(VK_ERROR_INITIALIZATION_FAILED, "No suitable queue found");

    VkQueue queue{};
    vkGetDeviceQueue(device, *idx, 0, &queue);

    return { *idx, queue };
}

std::vector<const char*> Utils::addExtensions(const char* const* extensions, size_t count,
        const std::vector<const char*>& requiredExtensions) {
    std::vector<const char*> ext(count);
    std::copy_n(extensions, count, ext.data());

    for (const auto& e : requiredExtensions) {
        auto it = std::ranges::find(ext, e);
        if (it == ext.end())
            ext.push_back(e);
    }

    return ext;
}

void Utils::copyImage(VkCommandBuffer buf,
        VkImage src, VkImage dst,
        uint32_t width, uint32_t height,
        VkPipelineStageFlags pre, VkPipelineStageFlags post,
        bool makeSrcPresentable, bool makeDstPresentable) {
    const VkImageMemoryBarrier srcBarrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .image = src,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .layerCount = 1
        }
    };
    const VkImageMemoryBarrier dstBarrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .image = dst,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .layerCount = 1
        }
    };
    const std::vector<VkImageMemoryBarrier> barriers = { srcBarrier, dstBarrier };
    vkCmdPipelineBarrier(buf,
        pre, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
        0, nullptr, 0, nullptr,
        static_cast<uint32_t>(barriers.size()), barriers.data());

    const VkImageCopy imageCopy{
        .srcSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .layerCount = 1
        },
        .dstSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .layerCount = 1
        },
        .extent = {
            .width = width,
            .height = height,
            .depth = 1
        }
    };
    vkCmdCopyImage(buf,
        src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, &imageCopy);

    if (makeSrcPresentable) {
        const VkImageMemoryBarrier presentBarrier{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .image = src,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1
            }
        };
        vkCmdPipelineBarrier(buf,
            VK_PIPELINE_STAGE_TRANSFER_BIT, post, 0,
            0, nullptr, 0, nullptr,
            1, &presentBarrier);
    }

    if (makeDstPresentable) {
        const VkImageMemoryBarrier presentBarrier{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .image = dst,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1
            }
        };
        vkCmdPipelineBarrier(buf,
            VK_PIPELINE_STAGE_TRANSFER_BIT, post, 0,
            0, nullptr, 0, nullptr,
            1, &presentBarrier);
    }

}
