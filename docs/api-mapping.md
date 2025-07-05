# API Mapping

## Overview

This document provides a detailed mapping between the current LSFG API and the target FidelityFX SDK APIs.

## Current LSFG API

### Interface Definition
**File**: `lsfg-vk/lsfg-vk-gen/public/lsfg.hpp`

```cpp
namespace LSFG {
    void initialize();
    int32_t createContext(uint32_t width, uint32_t height, int in0, int in1, const std::vector<int>& outN);
    void presentContext(int32_t id, int inSem, const std::vector<int>& outSem);
    void deleteContext(int32_t id);
    void finalize();
    
    class vulkan_error : public std::runtime_error {
        VkResult error() const;
    };
}
```

### Implementation Reference
**File**: `lsfg-vk/lsfg-vk-gen/src/lsfg.cpp`

## FidelityFX Frame Interpolation API

### Core APIs
**File**: `FidelityFX-SDK/sdk/include/FidelityFX/host/ffx_frameinterpolation.h`

```cpp
// Context Management
FFX_API FfxErrorCode ffxFrameInterpolationContextCreate(
    FfxFrameInterpolationContext* context, 
    FfxFrameInterpolationContextDescription* contextDescription);

FFX_API FfxErrorCode ffxFrameInterpolationContextDestroy(
    FfxFrameInterpolationContext* context);

// Frame Generation
FFX_API FfxErrorCode ffxFrameInterpolationDispatch(
    FfxFrameInterpolationContext* context, 
    const FfxFrameInterpolationDispatchDescription* params);

// Resource Management
FFX_API FfxErrorCode ffxFrameInterpolationGetSharedResourceDescriptions(
    FfxFrameInterpolationContext* context, 
    FfxFrameInterpolationSharedResourceDescriptions* sharedResources);
```

## Direct API Mapping

### 1. Initialization

#### Current LSFG
```cpp
void LSFG::initialize() {
    // File: lsfg-vk/lsfg-vk-gen/src/lsfg.cpp:18-32
    char* dllPath = getenv("LSFG_DLL_PATH");
    const std::string dllPathStr = dllPath ? std::string(dllPath) : "Lossless.dll";
    
    instance.emplace();
    device.emplace(*instance);
    pool.emplace(dllPathStr);
    
    Globals::initializeGlobals(*device);
}
```

#### Target FidelityFX
```cpp
void AFMF::initialize() {
    // Initialize FidelityFX backend interface
    FfxInterface backendInterface;
    ffxGetInterfaceVK(&backendInterface, vkDevice, vkPhysicalDevice, 
                      vkDeviceContext, vkPhysicalDeviceContext);
    
    // Store global context for frame interpolation
    // No DLL loading required - native implementation
}
```

### 2. Context Creation

#### Current LSFG
```cpp
int32_t LSFG::createContext(uint32_t width, uint32_t height, int in0, int in1, 
                           const std::vector<int>& outN) {
    // File: lsfg-vk/lsfg-vk-gen/src/lsfg.cpp:34-41
    auto id = std::rand();
    contexts.emplace(id, Context(*device, *pool, width, height, in0, in1, outN));
    return id;
}
```

**Parameters**:
- `width`, `height`: Frame dimensions
- `in0`, `in1`: File descriptors for input frame images (ping-pong)
- `outN`: File descriptors for output interpolated frames

#### Target FidelityFX
```cpp
int32_t AFMF::createContext(uint32_t width, uint32_t height, int in0, int in1, 
                           const std::vector<int>& outN) {
    FfxFrameInterpolationContextDescription desc = {};
    desc.backendInterface = g_backendInterface;
    desc.maxRenderSize = {width, height};
    desc.displaySize = {width, height};
    desc.flags = FFX_FRAMEINTERPOLATION_ENABLE_MOTION_VECTORS_JITTER_CANCELLATION;
    
    FfxFrameInterpolationContext* context = new FfxFrameInterpolationContext;
    FFX_CHECK(ffxFrameInterpolationContextCreate(context, &desc));
    
    auto id = std::rand();
    g_contexts[id] = {context, createVulkanResources(in0, in1, outN)};
    return id;
}
```

### 3. Frame Presentation

#### Current LSFG
```cpp
void LSFG::presentContext(int32_t id, int inSem, const std::vector<int>& outSem) {
    // File: lsfg-vk/lsfg-vk-gen/src/lsfg.cpp:43-51
    auto it = contexts.find(id);
    Context& context = it->second;
    context.present(*device, inSem, outSem);
}
```

**Parameters**:
- `id`: Context identifier
- `inSem`: Input semaphore file descriptor (synchronization)
- `outSem`: Output semaphore file descriptors for interpolated frames

#### Target FidelityFX
```cpp
void AFMF::presentContext(int32_t id, int inSem, const std::vector<int>& outSem) {
    auto& ctxData = g_contexts[id];
    
    FfxFrameInterpolationDispatchDescription dispatchDesc = {};
    dispatchDesc.commandList = getCurrentCommandList();
    dispatchDesc.displaySize = ctxData.displaySize;
    dispatchDesc.currentBackBuffer = createResourceFromFD(inSem);
    dispatchDesc.output = ctxData.outputResources;
    dispatchDesc.frameID = getCurrentFrameID();
    
    FFX_CHECK(ffxFrameInterpolationDispatch(ctxData.context, &dispatchDesc));
    
    // Handle output semaphore signaling
    signalOutputSemaphores(outSem);
}
```

### 4. Context Destruction

#### Current LSFG
```cpp
void LSFG::deleteContext(int32_t id) {
    // File: lsfg-vk/lsfg-vk-gen/src/lsfg.cpp:53-60
    vkDeviceWaitIdle(device->handle());
    contexts.erase(it);
}
```

#### Target FidelityFX
```cpp
void AFMF::deleteContext(int32_t id) {
    auto& ctxData = g_contexts[id];
    
    vkDeviceWaitIdle(g_device);
    ffxFrameInterpolationContextDestroy(ctxData.context);
    
    delete ctxData.context;
    cleanupVulkanResources(ctxData.resources);
    g_contexts.erase(id);
}
```

## Resource Management Mapping

### Current LSFG Resource Handling
**File**: `lsfg-vk/src/context.cpp:10-40`

```cpp
// File descriptor based resource sharing
Mini::Image frame_0 = Mini::Image(info.device, info.physicalDevice,
    extent, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_TRANSFER_DST_BIT,
    VK_IMAGE_ASPECT_COLOR_BIT, &frame_0_fd);

Mini::Image frame_1 = Mini::Image(info.device, info.physicalDevice,
    extent, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_TRANSFER_DST_BIT,
    VK_IMAGE_ASPECT_COLOR_BIT, &frame_1_fd);
```

### FidelityFX Resource Handling
**Reference**: `FidelityFX-SDK/sdk/src/components/frameinterpolation/ffx_frameinterpolation.cpp:461-490`

```cpp
// FidelityFX internal resource management
FfxCreateResourceDescription resourceDescs[] = {
    {FFX_FRAMEINTERPOLATION_RESOURCE_IDENTIFIER_RECONSTRUCTED_DEPTH_PREVIOUS_FRAME, 
     L"FI_ReconstructedDepthPreviousFrame",
     FFX_RESOURCE_TYPE_TEXTURE2D, FFX_RESOURCE_USAGE_UAV,
     FFX_SURFACE_FORMAT_R32_UINT, width, height, 1, 
     FFX_RESOURCE_FLAGS_ALIASABLE, {FFX_RESOURCE_INIT_DATA_TYPE_UNINITIALIZED}},
    // ... more resources
};
```

## Environment Variable Mapping

### Current LSFG Environment Variables
```bash
LSFG_DLL_PATH="/path/to/Lossless.dll"  # Path to Windows DLL
LSFG_MULTIPLIER=4                      # Frame generation multiplier
```

### Proposed AFMF Environment Variables
```bash
# Basic frame generation
AFMF_MULTIPLIER=4                      # Frame generation multiplier
AFMF_ENABLE_OPTICAL_FLOW=1             # Use optical flow for motion estimation
AFMF_DEBUG_MODE=0                      # Enable debug visualization

# Advanced features (Phase 3)
AFMF_ENABLE_FSR3=1                     # Enable FSR3 upscaling
AFMF_UPSCALE_RATIO=1.5                 # FSR3 upscaling ratio
AFMF_MOTION_VECTOR_SCALE=1.0           # Motion vector scaling factor
```

## Error Handling Mapping

### Current LSFG
```cpp
class vulkan_error : public std::runtime_error {
    VkResult result;
public:
    vulkan_error(VkResult result, const std::string& message);
    VkResult error() const { return result; }
};
```

### FidelityFX
```cpp
// FidelityFX uses FfxErrorCode enum
enum FfxErrorCode {
    FFX_OK = 0,
    FFX_ERROR_INVALID_POINTER,
    FFX_ERROR_INVALID_ALIGNMENT,
    FFX_ERROR_INVALID_SIZE,
    FFX_ERROR_INVALID_PATH,
    FFX_ERROR_EOF,
    FFX_ERROR_MALFORMED_DATA,
    FFX_ERROR_OUT_OF_MEMORY,
    FFX_ERROR_INCOMPLETE_INTERFACE,
    FFX_ERROR_INVALID_ENUM,
    FFX_ERROR_INVALID_ARGUMENT,
    FFX_ERROR_OUT_OF_RANGE,
    FFX_ERROR_NULL_DEVICE,
    FFX_ERROR_BACKEND_API_ERROR,
    FFX_ERROR_INSUFFICIENT_MEMORY
};

// Adapter class for compatibility
class afmf_error : public std::runtime_error {
    FfxErrorCode errorCode;
public:
    afmf_error(FfxErrorCode code, const std::string& message);
    FfxErrorCode error() const { return errorCode; }
};
```

## Key Differences and Adaptations Required

1. **Resource Creation**: FidelityFX manages resources internally vs. external file descriptor sharing
2. **Synchronization**: FidelityFX uses command lists vs. direct semaphore file descriptors  
3. **Context Lifecycle**: FidelityFX has more structured creation/destruction patterns
4. **Error Handling**: Different error code systems require adaptation
5. **Configuration**: FidelityFX offers more granular control and features

## Implementation Strategy

1. **Phase 1**: Create adapter layer that maps LSFG API calls to FidelityFX calls
2. **Phase 2**: Gradually expose more FidelityFX features through extended API
3. **Phase 3**: Full migration to native FidelityFX API patterns
