// Simplified FidelityFX Vulkan backend for lsfg-vk-afmf
// Based on FidelityFX SDK - Adapted for Linux compilation

#pragma once

#include <vulkan/vulkan.h>
#include <ffx/host/ffx_interface.h>

#ifdef __cplusplus
extern "C" {
#endif

// Simplified types for our use case
typedef void* FfxDevice;
typedef void* FfxPipeline;

typedef enum FfxResourceUsage {
    FFX_RESOURCE_USAGE_READ_ONLY = 0,
    FFX_RESOURCE_USAGE_RENDERTARGET = 1,
    FFX_RESOURCE_USAGE_UAV = 2,
} FfxResourceUsage;

typedef enum FfxResourceStates {
    FFX_RESOURCE_STATE_COMPUTE_READ = 0,
    FFX_RESOURCE_STATE_UNORDERED_ACCESS = 1,
    FFX_RESOURCE_STATE_RENDER_TARGET = 2,
} FfxResourceStates;

typedef struct FfxResourceDescription {
    FfxSurfaceFormat format;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t mipCount;
    uint32_t flags;
    FfxResourceUsage usage;
} FfxResourceDescription;

// Vulkan device context
typedef struct VkDeviceContext {
    VkDevice                vkDevice;           
    VkPhysicalDevice        vkPhysicalDevice;   
    PFN_vkGetDeviceProcAddr vkDeviceProcAddr;   
} VkDeviceContext;

// Main functions we need
FFX_API size_t ffxGetScratchMemorySizeVK(VkPhysicalDevice physicalDevice, size_t maxContexts);

FFX_API FfxDevice ffxGetDeviceVK(VkDeviceContext* vkDeviceContext);

FFX_API FfxErrorCode ffxGetInterfaceVK(
    FfxInterface* backendInterface,
    FfxDevice device,
    void* scratchBuffer,
    size_t scratchBufferSize, 
    size_t maxContexts);

FFX_API FfxCommandList ffxGetCommandListVK(VkCommandBuffer cmdBuf);

FFX_API FfxResource ffxGetResourceVK(void* vkResource,
    FfxResourceDescription ffxResDescription,
    const wchar_t* ffxResName,
    FfxResourceStates state);

FFX_API FfxSurfaceFormat ffxGetSurfaceFormatVK(VkFormat format);

FFX_API FfxResourceDescription ffxGetImageResourceDescriptionVK(const VkImage image,
                                                                const VkImageCreateInfo createInfo,
                                                                FfxResourceUsage additionalUsages);

#ifdef __cplusplus
}
#endif
