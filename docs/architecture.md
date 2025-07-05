# System Architecture

## Current lsfg-vk Architecture

The existing `lsfg-vk` project uses the following architecture:

```
Game Process
    ↓ (LD_PRELOAD)
Vulkan API Hooks (lsfg-vk/src/hooks.cpp)
    ↓
Frame Generation Context (lsfg-vk/src/context.cpp)
    ↓
LSFG Interface (lsfg-vk/lsfg-vk-gen/)
    ↓
Lossless.dll (Windows DLL via DXVK)
    ↓
DirectX 11 Compute Shaders → SPIR-V (via DXVK)
    ↓
Vulkan Execution
```

### Key Components Analysis

#### 1. Vulkan Hooks (`lsfg-vk/src/hooks.cpp`)
- **Purpose**: Intercepts Vulkan API calls (`vkCreateSwapchainKHR`, `vkQueuePresentKHR`, etc.)
- **Current Implementation**: 218 lines of hook management
- **Key Functions**:
  - `myvkCreateInstance()` - Adds required extensions for external memory/semaphores
  - `myvkCreateDevice()` - Sets up device info with frame generation multiplier
  - `myvkCreateSwapchainKHR()` - Creates frame generation context
  - `myvkQueuePresentKHR()` - Implements frame interpolation logic

#### 2. Frame Generation Context (`lsfg-vk/src/context.cpp`)
- **Purpose**: Manages frame interpolation between game frames
- **Current Implementation**: 162 lines managing swapchain images and frame generation
- **Key Features**:
  - Multiple swapchain image management
  - Semaphore synchronization for frame timing
  - Image copying between game frames and generated frames
  - Command buffer management for frame operations

#### 3. LSFG Interface (`lsfg-vk/lsfg-vk-gen/public/lsfg.hpp`)
- **Purpose**: C++ wrapper around Lossless.dll functionality
- **Current API**:
  ```cpp
  void initialize();
  int32_t createContext(uint32_t width, uint32_t height, int in0, int in1, const std::vector<int>& outN);
  void presentContext(int32_t id, int inSem, const std::vector<int>& outSem);
  void deleteContext(int32_t id);
  void finalize();
  ```

## Proposed FidelityFX Architecture

```
Game Process
    ↓ (LD_PRELOAD)
Vulkan API Hooks (Enhanced)
    ↓
FidelityFX Frame Interpolation Context
    ↓
FidelityFX SDK Native Components:
├── Frame Interpolation (sdk/include/FidelityFX/host/ffx_frameinterpolation.h)
├── Optical Flow (sdk/include/FidelityFX/host/ffx_opticalflow.h)
├── FSR3 Upscaler (sdk/include/FidelityFX/host/ffx_fsr3upscaler.h)
└── Frame Interpolation Swapchain (sdk/src/backends/vk/FrameInterpolationSwapchain/)
    ↓
Native Vulkan Compute Shaders
    ↓
Vulkan Execution
```

### FidelityFX Components

#### 1. Frame Interpolation (`FidelityFX-SDK/sdk/include/FidelityFX/host/ffx_frameinterpolation.h`)
- **Purpose**: Core frame generation algorithm
- **Key Functions**:
  ```cpp
  FfxErrorCode ffxFrameInterpolationContextCreate(FfxFrameInterpolationContext* context, 
                                                  FfxFrameInterpolationContextDescription* contextDescription);
  FfxErrorCode ffxFrameInterpolationDispatch(FfxFrameInterpolationContext* context, 
                                              const FfxFrameInterpolationDispatchDescription* params);
  ```
- **Features**: Motion vector processing, depth reconstruction, frame interpolation

#### 2. Frame Interpolation Swapchain (`FidelityFX-SDK/sdk/src/backends/vk/FrameInterpolationSwapchain/`)
- **Purpose**: Drop-in replacement for Vulkan swapchain with built-in frame generation
- **Key Files**:
  - `FrameInterpolationSwapchainVK.cpp` - Main swapchain implementation
  - `FrameInterpolationSwapchainVK_UiComposition.cpp` - UI composition handling
- **Features**: Frame pacing, VRR support, automatic UI composition

#### 3. Optical Flow (`FidelityFX-SDK/sdk/include/FidelityFX/host/ffx_opticalflow.h`)
- **Purpose**: Motion estimation for high-quality frame interpolation
- **Features**: Hardware-accelerated motion vector generation

## Migration Strategy

### Phase 1: Direct API Replacement
Replace `lsfg-vk-gen` interface with FidelityFX Frame Interpolation while keeping existing hook structure.

### Phase 2: Swapchain Integration
Replace custom swapchain management with FidelityFX Frame Interpolation Swapchain.

### Phase 3: Full FidelityFX Integration
Add FSR3 upscaling and advanced features for complete solution.

## Benefits of New Architecture

1. **No Windows Dependencies**: Pure Linux-native implementation
2. **Better Performance**: Hardware-optimized AMD shaders
3. **Enhanced Features**: Advanced motion estimation, VRR support
4. **Open Source**: Full control and debugging capability
5. **Production Ready**: Used in shipped games and applications
6. **Extensible**: Easy to add FSR upscaling and other FidelityFX techniques
