# Implementation Details

## Project Setup

### CMake Configuration

**File**: `CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.29)

project(lsfg-vk-afmf
    VERSION 0.1.0
    DESCRIPTION "Linux Native Frame Generation using AMD FidelityFX SDK"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# FidelityFX SDK Integration
set(FFX_SDK_PATH "${CMAKE_SOURCE_DIR}/../FidelityFX-SDK")
add_subdirectory(${FFX_SDK_PATH}/sdk ${CMAKE_BINARY_DIR}/ffx-sdk)

# Vulkan
find_package(Vulkan REQUIRED)

# Source files
file(GLOB SOURCES
    "src/*.cpp"
    "src/hooks/*.cpp"
    "src/afmf/*.cpp"
)

add_library(lsfg-vk-afmf SHARED ${SOURCES})

target_include_directories(lsfg-vk-afmf
    PRIVATE include
    PRIVATE ${FFX_SDK_PATH}/sdk/include
    PRIVATE ${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(lsfg-vk-afmf
    PRIVATE ffx_frameinterpolation_vk
    PRIVATE ffx_fsr3_vk
    PRIVATE ffx_opticalflow_vk
    PRIVATE ${Vulkan_LIBRARIES}
)

target_compile_definitions(lsfg-vk-afmf PRIVATE
    FFX_VK=1
    FFX_GPU_SHARED_MEM_SUPPORT=1
)

install(FILES "${CMAKE_BINARY_DIR}/liblsfg-vk-afmf.so" DESTINATION lib)
```

## Core Implementation Files

### 1. Main AFMF Interface

**File**: `include/afmf.hpp`

```cpp
#ifndef AFMF_HPP
#define AFMF_HPP

#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>

// FidelityFX includes
#include <FidelityFX/host/ffx_frameinterpolation.h>
#include <FidelityFX/host/ffx_fsr3.h>
#include <FidelityFX/host/ffx_opticalflow.h>

namespace AFMF {

    /// Initialize the AFMF library with FidelityFX backend
    void initialize();

    /// Create frame interpolation context
    /// @param width,height Frame dimensions
    /// @param in0,in1 File descriptors for input frame ping-pong buffers
    /// @param outN File descriptors for output interpolated frames
    /// @return Unique context identifier
    int32_t createContext(uint32_t width, uint32_t height, int in0, int in1,
        const std::vector<int>& outN);

    /// Present/interpolate frames
    /// @param id Context identifier  
    /// @param inSem Input semaphore file descriptor
    /// @param outSem Output semaphore file descriptors
    void presentContext(int32_t id, int inSem, const std::vector<int>& outSem);

    /// Delete context and cleanup resources
    void deleteContext(int32_t id);

    /// Finalize and cleanup AFMF library
    void finalize();

    /// Exception class for AFMF errors
    class afmf_error : public std::runtime_error {
    public:
        explicit afmf_error(FfxErrorCode result, const std::string& message);
        [[nodiscard]] FfxErrorCode error() const { return m_result; }
        
    private:
        FfxErrorCode m_result;
    };

    /// Configuration structure for advanced features
    struct AFMFConfig {
        bool enableOpticalFlow = true;
        bool enableFSR3 = false;
        float upscaleRatio = 1.0f;
        bool debugMode = false;
        uint32_t frameGenMultiplier = 2;
    };

    /// Set configuration (Phase 3 feature)
    void setConfig(const AFMFConfig& config);

}

#endif // AFMF_HPP
```

### 2. AFMF Implementation

**File**: `src/afmf.cpp`

```cpp
#include "afmf.hpp"
#include "afmf_internal.hpp"
#include "vulkan_helpers.hpp"

#include <FidelityFX/host/ffx_interface.h>
#include <FidelityFX/host/backends/vk/ffx_vk.h>

#include <cstdlib>
#include <ctime>
#include <unordered_map>
#include <memory>

namespace AFMF {

namespace {
    // Global FidelityFX interface
    FfxInterface g_backendInterface{};
    VkDevice g_device = VK_NULL_HANDLE;
    VkPhysicalDevice g_physicalDevice = VK_NULL_HANDLE;
    
    // Context storage
    std::unordered_map<int32_t, std::unique_ptr<AFMFContext>> g_contexts;
    
    // Configuration
    AFMFConfig g_config{};
    bool g_initialized = false;
}

void initialize() {
    if (g_initialized) return;
    
    // Get Vulkan device from environment or current context
    // This would be set by the hook system
    g_device = GetCurrentVulkanDevice();
    g_physicalDevice = GetCurrentVulkanPhysicalDevice();
    
    // Initialize FidelityFX Vulkan backend
    const size_t scratchBufferSize = ffxGetScratchMemorySizeVK(g_physicalDevice);
    void* scratchBuffer = malloc(scratchBufferSize);
    
    FfxErrorCode result = ffxGetInterfaceVK(
        &g_backendInterface,
        ffxGetDeviceVK(g_device),
        scratchBuffer,
        scratchBufferSize,
        g_physicalDevice);
        
    if (result != FFX_OK) {
        throw afmf_error(result, "Failed to initialize FidelityFX Vulkan backend");
    }
    
    // Load configuration from environment
    loadConfigFromEnvironment();
    
    g_initialized = true;
    std::srand(static_cast<uint32_t>(std::time(nullptr)));
}

int32_t createContext(uint32_t width, uint32_t height, int in0, int in1,
                     const std::vector<int>& outN) {
    if (!g_initialized) {
        throw afmf_error(FFX_ERROR_INVALID_POINTER, "AFMF not initialized");
    }
    
    // Create unique context ID
    int32_t id = std::rand();
    while (g_contexts.find(id) != g_contexts.end()) {
        id = std::rand();
    }
    
    // Create AFMF context wrapper
    auto context = std::make_unique<AFMFContext>();
    
    // Initialize FidelityFX Frame Interpolation context
    FfxFrameInterpolationContextDescription desc{};
    desc.backendInterface = g_backendInterface;
    desc.maxRenderSize = {width, height};
    desc.displaySize = {width, height};
    desc.flags = FFX_FRAMEINTERPOLATION_ENABLE_MOTION_VECTORS_JITTER_CANCELLATION;
    
    if (g_config.enableOpticalFlow) {
        desc.flags |= FFX_FRAMEINTERPOLATION_ENABLE_OPTICAL_FLOW;
    }
    
    FfxErrorCode result = ffxFrameInterpolationContextCreate(
        &context->frameInterpolationContext, &desc);
        
    if (result != FFX_OK) {
        throw afmf_error(result, "Failed to create frame interpolation context");
    }
    
    // Create Vulkan resources from file descriptors
    context->vulkanResources = createVulkanResourcesFromFDs(
        g_device, g_physicalDevice, width, height, in0, in1, outN);
    
    g_contexts[id] = std::move(context);
    return id;
}

void presentContext(int32_t id, int inSem, const std::vector<int>& outSem) {
    auto it = g_contexts.find(id);
    if (it == g_contexts.end()) {
        throw afmf_error(FFX_ERROR_INVALID_ARGUMENT, "Invalid context ID");
    }
    
    AFMFContext& context = *it->second;
    
    // Convert semaphore FDs to Vulkan semaphores
    VkSemaphore inputSemaphore = importSemaphoreFromFD(g_device, inSem);
    std::vector<VkSemaphore> outputSemaphores;
    for (int fd : outSem) {
        outputSemaphores.push_back(importSemaphoreFromFD(g_device, fd));
    }
    
    // Get current command buffer (would be provided by hook system)
    VkCommandBuffer cmdBuffer = GetCurrentCommandBuffer();
    
    // Prepare dispatch description
    FfxFrameInterpolationDispatchDescription dispatchDesc{};
    dispatchDesc.commandList = ffxGetCommandListVK(cmdBuffer);
    dispatchDesc.displaySize = context.vulkanResources.displaySize;
    dispatchDesc.renderSize = context.vulkanResources.displaySize;
    dispatchDesc.currentBackBuffer = context.vulkanResources.currentFrame;
    dispatchDesc.currentBackBuffer_HUDLess = context.vulkanResources.currentFrame;
    dispatchDesc.output = context.vulkanResources.outputFrames[0]; // First interpolated frame
    dispatchDesc.frameID = GetCurrentFrameID();
    dispatchDesc.interpolationRect = {0, 0, 
        context.vulkanResources.displaySize.width,
        context.vulkanResources.displaySize.height};
    
    // Dispatch frame interpolation
    FfxErrorCode result = ffxFrameInterpolationDispatch(
        &context.frameInterpolationContext, &dispatchDesc);
        
    if (result != FFX_OK) {
        throw afmf_error(result, "Frame interpolation dispatch failed");
    }
    
    // Signal output semaphores
    signalOutputSemaphores(cmdBuffer, outputSemaphores);
}

void deleteContext(int32_t id) {
    auto it = g_contexts.find(id);
    if (it == g_contexts.end()) {
        throw afmf_error(FFX_ERROR_INVALID_ARGUMENT, "Invalid context ID");
    }
    
    AFMFContext& context = *it->second;
    
    // Wait for device idle
    vkDeviceWaitIdle(g_device);
    
    // Destroy FidelityFX context
    ffxFrameInterpolationContextDestroy(&context.frameInterpolationContext);
    
    // Cleanup Vulkan resources
    cleanupVulkanResources(g_device, context.vulkanResources);
    
    g_contexts.erase(it);
}

void finalize() {
    if (!g_initialized) return;
    
    // Wait for all operations to complete
    vkDeviceWaitIdle(g_device);
    
    // Destroy all remaining contexts
    g_contexts.clear();
    
    // Cleanup FidelityFX backend
    if (g_backendInterface.scratchBuffer) {
        free(g_backendInterface.scratchBuffer);
    }
    
    g_initialized = false;
}

void setConfig(const AFMFConfig& config) {
    g_config = config;
}

// Exception implementation
afmf_error::afmf_error(FfxErrorCode result, const std::string& message)
    : std::runtime_error(message), m_result(result) {}

} // namespace AFMF
```

### 3. Internal Data Structures

**File**: `src/afmf_internal.hpp`

```cpp
#ifndef AFMF_INTERNAL_HPP
#define AFMF_INTERNAL_HPP

#include <FidelityFX/host/ffx_frameinterpolation.h>
#include <vulkan/vulkan_core.h>
#include <vector>

namespace AFMF {

struct VulkanResources {
    VkImage currentFrame;
    VkImage previousFrame;
    std::vector<VkImage> outputFrames;
    VkExtent2D displaySize;
    
    // Memory and views
    std::vector<VkDeviceMemory> memories;
    std::vector<VkImageView> imageViews;
};

struct AFMFContext {
    FfxFrameInterpolationContext frameInterpolationContext;
    VulkanResources vulkanResources;
    uint64_t frameCounter = 0;
};

// Helper functions
VulkanResources createVulkanResourcesFromFDs(
    VkDevice device, VkPhysicalDevice physicalDevice,
    uint32_t width, uint32_t height,
    int in0, int in1, const std::vector<int>& outN);

void cleanupVulkanResources(VkDevice device, VulkanResources& resources);

VkSemaphore importSemaphoreFromFD(VkDevice device, int fd);
void signalOutputSemaphores(VkCommandBuffer cmdBuffer, 
                           const std::vector<VkSemaphore>& semaphores);

void loadConfigFromEnvironment();

// Hook integration helpers
VkDevice GetCurrentVulkanDevice();
VkPhysicalDevice GetCurrentVulkanPhysicalDevice();
VkCommandBuffer GetCurrentCommandBuffer();
uint64_t GetCurrentFrameID();

} // namespace AFMF

#endif // AFMF_INTERNAL_HPP
```

### 4. Hook Integration

**File**: `src/hooks/vulkan_hooks.cpp`

```cpp
#include "afmf.hpp"
#include "vulkan_loader.hpp"

#include <unordered_map>
#include <vulkan/vulkan_core.h>

// Reference implementation from: lsfg-vk/src/hooks.cpp

namespace {
    std::unordered_map<VkDevice, DeviceInfo> g_devices;
    std::unordered_map<VkSwapchainKHR, AFMFSwapchainContext> g_swapchains;
}

VkResult VKAPI_CALL hookedVkCreateInstance(
    const VkInstanceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkInstance* pInstance) {
    
    // Initialize AFMF library
    AFMF::initialize();
    
    // Add required extensions for external memory/semaphores
    auto extensions = addRequiredExtensions(pCreateInfo);
    
    VkInstanceCreateInfo modifiedCreateInfo = *pCreateInfo;
    modifiedCreateInfo.enabledExtensionCount = extensions.size();
    modifiedCreateInfo.ppEnabledExtensionNames = extensions.data();
    
    return vkCreateInstance(&modifiedCreateInfo, pAllocator, pInstance);
}

VkResult VKAPI_CALL hookedVkCreateDevice(
    VkPhysicalDevice physicalDevice,
    const VkDeviceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDevice* pDevice) {
    
    // Add required device extensions
    auto extensions = addRequiredDeviceExtensions(pCreateInfo);
    
    VkDeviceCreateInfo modifiedCreateInfo = *pCreateInfo;
    modifiedCreateInfo.enabledExtensionCount = extensions.size();
    modifiedCreateInfo.ppEnabledExtensionNames = extensions.data();
    
    VkResult result = vkCreateDevice(physicalDevice, &modifiedCreateInfo, 
                                    pAllocator, pDevice);
    
    if (result == VK_SUCCESS) {
        // Store device info for AFMF context creation
        DeviceInfo info{};
        info.device = *pDevice;
        info.physicalDevice = physicalDevice;
        info.queue = findGraphicsQueue(*pDevice, physicalDevice, &modifiedCreateInfo);
        info.frameGenMultiplier = getFrameGenMultiplierFromEnv();
        
        g_devices[*pDevice] = info;
    }
    
    return result;
}

VkResult VKAPI_CALL hookedVkCreateSwapchainKHR(
    VkDevice device,
    const VkSwapchainCreateInfoKHR* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkSwapchainKHR* pSwapchain) {
    
    VkResult result = vkCreateSwapchainKHR(device, pCreateInfo, pAllocator, pSwapchain);
    
    if (result == VK_SUCCESS) {
        // Create AFMF context for this swapchain
        auto& deviceInfo = g_devices[device];
        
        AFMFSwapchainContext swapchainContext{};
        swapchainContext.deviceInfo = deviceInfo;
        swapchainContext.extent = pCreateInfo->imageExtent;
        
        // Get swapchain images
        uint32_t imageCount;
        vkGetSwapchainImagesKHR(device, *pSwapchain, &imageCount, nullptr);
        swapchainContext.swapchainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device, *pSwapchain, &imageCount, 
                               swapchainContext.swapchainImages.data());
        
        // Create AFMF context - this will need adaptation for FD-based interface
        swapchainContext.afmfContextId = createAFMFContextForSwapchain(
            deviceInfo, pCreateInfo->imageExtent, swapchainContext.swapchainImages);
        
        g_swapchains[*pSwapchain] = swapchainContext;
    }
    
    return result;
}

VkResult VKAPI_CALL hookedVkQueuePresentKHR(
    VkQueue queue,
    const VkPresentInfoKHR* pPresentInfo) {
    
    // Intercept present call and perform frame interpolation
    for (uint32_t i = 0; i < pPresentInfo->swapchainCount; ++i) {
        VkSwapchainKHR swapchain = pPresentInfo->pSwapchains[i];
        uint32_t imageIndex = pPresentInfo->pImageIndices[i];
        
        auto it = g_swapchains.find(swapchain);
        if (it != g_swapchains.end()) {
            AFMFSwapchainContext& ctx = it->second;
            
            // Perform frame interpolation using AFMF
            return performFrameInterpolation(ctx, queue, pPresentInfo, i);
        }
    }
    
    // Fallback to normal present
    return vkQueuePresentKHR(queue, pPresentInfo);
}

// Hook installation
void installVulkanHooks() {
    // Install function pointer overrides
    // Implementation depends on loader approach
}
```

## Key Implementation Considerations

### 1. File Descriptor Handling
- **Challenge**: FidelityFX doesn't use file descriptor-based resource sharing
- **Solution**: Create adapter layer that converts between FDs and Vulkan resources
- **Reference**: `lsfg-vk/src/context.cpp:15-25` for current FD usage

### 2. Synchronization Adaptation
- **Challenge**: Different synchronization models (FDs vs command lists)
- **Solution**: Maintain semaphore mapping and translation layer
- **Reference**: `lsfg-vk/src/context.cpp:50-100` for current sync logic

### 3. Resource Lifetime Management
- **Challenge**: FidelityFX manages resources differently than current implementation
- **Solution**: Create resource wrapper that bridges both models
- **Reference**: `FidelityFX-SDK/sdk/src/components/frameinterpolation/ffx_frameinterpolation.cpp:460-490`

### 4. Error Handling Consistency
- **Challenge**: Maintain compatible error interface
- **Solution**: Create error translation layer between FfxErrorCode and VkResult
- **Reference**: `lsfg-vk/lsfg-vk-gen/public/lsfg.hpp:67-80` for current error handling

This implementation provides a foundation for Phase 1 of the migration plan while maintaining compatibility with the existing lsfg-vk hook system.
