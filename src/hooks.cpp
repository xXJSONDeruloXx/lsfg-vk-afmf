#include "loader/dl.hpp"
#include "loader/vk.hpp"
#include "context.hpp"
#include "hooks.hpp"
#include "log.hpp"
#include "utils.hpp"

#include <afmf.hpp>

#include <algorithm>
#include <string>
#include <unordered_map>

using namespace Hooks;

namespace {

    // instance hooks

    VkResult myvkCreateInstance(
            const VkInstanceCreateInfo* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkInstance* pInstance) {
        // create afmf
        Loader::DL::disableHooks();
        AFMF::initialize();
        Loader::DL::enableHooks();

        // add extensions
        auto extensions = Utils::addExtensions(pCreateInfo->ppEnabledExtensionNames,
            pCreateInfo->enabledExtensionCount, {
                "VK_KHR_get_physical_device_properties2",
                "VK_KHR_external_memory_capabilities",
                "VK_KHR_external_semaphore_capabilities"
            });

        VkInstanceCreateInfo createInfo = *pCreateInfo;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();
        return vkCreateInstance(&createInfo, pAllocator, pInstance);
    }

    void myvkDestroyInstance(
            VkInstance instance,
            const VkAllocationCallbacks* pAllocator) {
        AFMF::finalize(); // destroy afmf
        vkDestroyInstance(instance, pAllocator);
    }

    // device hooks

    std::unordered_map<VkDevice, DeviceInfo> devices;

    VkResult myvkCreateDevice(
            VkPhysicalDevice physicalDevice,
            const VkDeviceCreateInfo* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkDevice* pDevice) {
        // add extensions
        auto extensions = Utils::addExtensions(pCreateInfo->ppEnabledExtensionNames,
            pCreateInfo->enabledExtensionCount, {
                "VK_KHR_external_memory",
                "VK_KHR_external_memory_fd",
                "VK_KHR_external_semaphore",
                "VK_KHR_external_semaphore_fd"
            });

        VkDeviceCreateInfo createInfo = *pCreateInfo;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();
        auto res = vkCreateDevice(physicalDevice, &createInfo, pAllocator, pDevice);

        // store device info
        try {
            const char* frameGen = std::getenv("AFMF_MULTIPLIER");
            if (!frameGen) frameGen = "2";
            devices.emplace(*pDevice, DeviceInfo {
                .device = *pDevice,
                .physicalDevice = physicalDevice,
                .queue = Utils::findQueue(*pDevice, physicalDevice, &createInfo,
                    VK_QUEUE_GRAPHICS_BIT),
                .frameGen = std::max<size_t>(1, std::stoul(frameGen) - 1)
            });
        } catch (const std::exception& e) {
            Log::error("Failed to create device info: {}", e.what());
            return VK_ERROR_INITIALIZATION_FAILED;
        }
        return res;
    }

    void myvkDestroyDevice(VkDevice device, const VkAllocationCallbacks* pAllocator) {
        devices.erase(device); // erase device info
        vkDestroyDevice(device, pAllocator);
    }

    // swapchain hooks

    std::unordered_map<VkSwapchainKHR, LsContext> swapchains;
    std::unordered_map<VkSwapchainKHR, VkDevice> swapchainToDeviceTable;

    VkResult myvkCreateSwapchainKHR(
            VkDevice device,
            const VkSwapchainCreateInfoKHR* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkSwapchainKHR* pSwapchain) {
        auto& deviceInfo = devices.at(device);

        // update swapchain create info
        VkSwapchainCreateInfoKHR createInfo = *pCreateInfo;
        createInfo.minImageCount += 1 + deviceInfo.frameGen; // 1 deferred + N framegen, FIXME: check hardware max
        createInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT; // allow copy from/to images
        createInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR; // force vsync
        auto res = vkCreateSwapchainKHR(device, &createInfo, pAllocator, pSwapchain);
        if (res != VK_SUCCESS) {
            Log::error("Failed to create swapchain: {:x}", static_cast<uint32_t>(res));
            return res;
        }

        try {
            // get swapchain images
            uint32_t imageCount{};
            res = vkGetSwapchainImagesKHR(device, *pSwapchain, &imageCount, nullptr);
            if (res != VK_SUCCESS || imageCount == 0)
                throw AFMF::vulkan_error(res, "Failed to get swapchain images count");

            std::vector<VkImage> swapchainImages(imageCount);
            res = vkGetSwapchainImagesKHR(device, *pSwapchain, &imageCount, swapchainImages.data());
            if (res != VK_SUCCESS)
                throw AFMF::vulkan_error(res, "Failed to get swapchain images");

            // create swapchain context
            swapchains.emplace(*pSwapchain, LsContext(
                deviceInfo, *pSwapchain, pCreateInfo->imageExtent,
                swapchainImages
            ));

            swapchainToDeviceTable.emplace(*pSwapchain, device);
            Log::debug("Created swapchain with {} images", imageCount);
        } catch (const AFMF::vulkan_error& e) {
            Log::error("Encountered Vulkan error {:x} while creating swapchain: {}",
                static_cast<uint32_t>(e.error()), e.what());
            return e.error();
        } catch (const std::exception& e) {
            Log::error("Encountered error while creating swapchain: {}", e.what());
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        return res;
    }

    VkResult myvkQueuePresentKHR(
            VkQueue queue,
            const VkPresentInfoKHR* pPresentInfo) {
        auto& deviceInfo = devices.at(swapchainToDeviceTable.at(*pPresentInfo->pSwapchains));
        auto& swapchain = swapchains.at(*pPresentInfo->pSwapchains);

        try {
            std::vector<VkSemaphore> waitSemaphores(pPresentInfo->waitSemaphoreCount);
            std::copy_n(pPresentInfo->pWaitSemaphores, static_cast<std::ptrdiff_t>(waitSemaphores.size()), waitSemaphores.data());

            // present the next frame
            return swapchain.present(deviceInfo, pPresentInfo->pNext,
                queue, waitSemaphores, *pPresentInfo->pImageIndices);
        } catch (const AFMF::vulkan_error& e) {
            Log::error("Encountered Vulkan error {:x} while presenting: {}",
                static_cast<uint32_t>(e.error()), e.what());
            return e.error();
        } catch (const std::exception& e) {
            Log::error("Encountered error while creating presenting: {}", e.what());
            return VK_ERROR_INITIALIZATION_FAILED;
        }
    }

    void myvkDestroySwapchainKHR(
            VkDevice device,
            VkSwapchainKHR swapchain,
            const VkAllocationCallbacks* pAllocator) {
        swapchains.erase(swapchain); // erase swapchain context
        swapchainToDeviceTable.erase(swapchain);
        vkDestroySwapchainKHR(device, swapchain, pAllocator);
    }

    bool initialized{false};
}

void Hooks::initialize() {
    if (initialized) {
        Log::warn("Vulkan hooks already initialized, did you call it twice?");
        return;
    }

    // list of hooks to register
    const std::vector<std::pair<std::string, void*>> hooks = {
        { "vkCreateInstance",      reinterpret_cast<void*>(myvkCreateInstance) },
        { "vkDestroyInstance",     reinterpret_cast<void*>(myvkDestroyInstance) },
        { "vkCreateDevice",        reinterpret_cast<void*>(myvkCreateDevice) },
        { "vkDestroyDevice",       reinterpret_cast<void*>(myvkDestroyDevice) },
        { "vkCreateSwapchainKHR",  reinterpret_cast<void*>(myvkCreateSwapchainKHR) },
        { "vkQueuePresentKHR",     reinterpret_cast<void*>(myvkQueuePresentKHR) },
        { "vkDestroySwapchainKHR", reinterpret_cast<void*>(myvkDestroySwapchainKHR) }
    };

    // register hooks to Vulkan loader
    for (const auto& hook : hooks)
        Loader::VK::registerSymbol(hook.first, hook.second);

    // register hooks to dynamic loader under libvulkan.so.1 and libvulkan.so
    for (const char* libName : {"libvulkan.so.1", "libvulkan.so"}) {
        Loader::DL::File vkLib(libName);
        for (const auto& hook : hooks)
            vkLib.defineSymbol(hook.first, hook.second);
        Loader::DL::registerFile(vkLib);
    }

    initialized = true;
    Log::info("Vulkan hooks initialized successfully");
}
