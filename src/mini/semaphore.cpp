#include "mini/semaphore.hpp"

#include <lsfg.hpp>

using namespace Mini;

Semaphore::Semaphore(VkDevice device) {
    // create semaphore
    const VkSemaphoreCreateInfo desc{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };
    VkSemaphore semaphoreHandle{};
    auto res = vkCreateSemaphore(device, &desc, nullptr, &semaphoreHandle);
    if (res != VK_SUCCESS || semaphoreHandle == VK_NULL_HANDLE)
        throw LSFG::vulkan_error(res, "Unable to create semaphore");

    // store semaphore in shared ptr
    this->semaphore = std::shared_ptr<VkSemaphore>(
        new VkSemaphore(semaphoreHandle),
        [dev = device](VkSemaphore* semaphoreHandle) {
            vkDestroySemaphore(dev, *semaphoreHandle, nullptr);
        }
    );
}

Semaphore::Semaphore(VkDevice device, int* fd) {
    // create semaphore
    const VkExportSemaphoreCreateInfo exportInfo{
        .sType = VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO,
        .handleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT
    };
    const VkSemaphoreCreateInfo desc{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = &exportInfo
    };
    VkSemaphore semaphoreHandle{};
    auto res = vkCreateSemaphore(device, &desc, nullptr, &semaphoreHandle);
    if (res != VK_SUCCESS || semaphoreHandle == VK_NULL_HANDLE)
        throw LSFG::vulkan_error(res, "Unable to create semaphore");

    // export semaphore to fd
    auto vkGetSemaphoreFdKHR = reinterpret_cast<PFN_vkGetSemaphoreFdKHR>(
        vkGetDeviceProcAddr(device, "vkGetSemaphoreFdKHR"));

    const VkSemaphoreGetFdInfoKHR fdInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR,
        .semaphore = semaphoreHandle,
        .handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT
    };
    res = vkGetSemaphoreFdKHR(device, &fdInfo, fd);
    if (res != VK_SUCCESS || *fd < 0)
        throw LSFG::vulkan_error(res, "Unable to export semaphore to fd");

    // store semaphore in shared ptr
    this->semaphore = std::shared_ptr<VkSemaphore>(
        new VkSemaphore(semaphoreHandle),
        [dev = device](VkSemaphore* semaphoreHandle) {
            vkDestroySemaphore(dev, *semaphoreHandle, nullptr);
        }
    );
}
