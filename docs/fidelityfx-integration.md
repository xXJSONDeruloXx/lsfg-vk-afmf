# FidelityFX SDK Integration Strategy

## What Role Does FidelityFX SDK Play?

### Core Technology Provider
The FidelityFX SDK is **not just a dependency** - it's the **core technology** that replaces the Windows-only `Lossless.dll`. Here's what it provides:

1. **Frame Interpolation Algorithm**: The mathematical and GPU compute implementation for generating intermediate frames
2. **Optimized GPU Shaders**: Highly optimized compute shaders for:
   - Motion vector analysis
   - Optical flow computation
   - Frame synthesis and blending
   - Temporal consistency
3. **Vulkan Backend**: Native Vulkan implementation (no DirectX translation needed)
4. **Resource Management**: Proper GPU memory allocation, synchronization, and command buffer management

### Why We Need It (Not Just Lifting Code)

**We CANNOT just copy code because:**
- The shaders are compiled binaries embedded in the library
- Complex GPU state management and optimization
- Ongoing updates and bug fixes from AMD
- Legal compliance (MIT license requires attribution)

**We NEED the actual library because:**
- Frame interpolation requires sophisticated GPU compute shaders
- Performance-critical algorithms need AMD's optimization
- Proper Vulkan resource lifecycle management
- Memory pooling and GPU synchronization

## Current Integration Approach

### Option 1: Git Submodule (RECOMMENDED)
```bash
# Add FidelityFX SDK as a git submodule
cd lsfg-vk-afmf
git submodule add https://github.com/GPUOpen-Effects/FidelityFX-SDK.git external/FidelityFX-SDK
git submodule update --init --recursive
```

**Advantages:**
- ✅ Version pinning and reproducible builds
- ✅ Easy updates via `git submodule update`
- ✅ Self-contained project
- ✅ CI/CD friendly

### Option 2: System Installation (ALTERNATIVE)
Install FidelityFX SDK system-wide and find it via CMake `find_package()`

**Disadvantages:**
- ❌ Requires users to install SDK separately
- ❌ Version compatibility issues
- ❌ Harder CI/CD setup

### Option 3: Copy SDK Source (NOT RECOMMENDED)
Copy the entire SDK into our repo

**Disadvantages:**
- ❌ Bloats our repository
- ❌ Hard to track upstream updates
- ❌ License attribution complexity

## Recommended Implementation Plan

### Step 1: Add as Submodule
```bash
cd /Users/kurt/Developer/lsfg/lsfg-vk-afmf
git submodule add https://github.com/GPUOpen-Effects/FidelityFX-SDK.git external/FidelityFX-SDK
```

### Step 2: Update CMakeLists.txt
```cmake
# Update path to use submodule
set(FFX_SDK_PATH "${CMAKE_SOURCE_DIR}/external/FidelityFX-SDK")

# Check if submodule exists
if(EXISTS "${FFX_SDK_PATH}/sdk/CMakeLists.txt")
    # Configure FidelityFX SDK build
    set(FFX_API_BACKEND "VK_X64" CACHE STRING "FidelityFX API Backend")
    set(FFX_BUILD_AS_DLL OFF CACHE BOOL "Build FidelityFX as static lib")
    
    # Add SDK as subdirectory
    add_subdirectory(${FFX_SDK_PATH}/sdk ${CMAKE_BINARY_DIR}/ffx-sdk)
    
    set(FFX_SDK_AVAILABLE TRUE)
    message(STATUS "FidelityFX SDK integrated from submodule")
else()
    message(FATAL_ERROR "FidelityFX SDK submodule not found. Run: git submodule update --init")
endif()
```

### Step 3: Link Against Specific Components
```cmake
target_link_libraries(lsfg-vk-afmf PRIVATE
    vulkan
    # FidelityFX specific components
    ffx_frameinterpolation_x64
    ffx_opticalflow_x64  
    ffx_backend_vk_x64
)
```

## Build Process Integration

### What Happens During Build:
1. **FidelityFX SDK builds first** (via `add_subdirectory`)
   - Compiles GPU shaders into binary blobs
   - Creates static libraries for each component
   - Builds Vulkan backend interface

2. **Our code links against SDK**
   - Links to pre-built FidelityFX libraries
   - Includes FidelityFX headers
   - Uses FidelityFX API calls instead of stubs

3. **Runtime behavior**
   - Our Vulkan hooks intercept game calls
   - FidelityFX processes frame data on GPU
   - Generated frames inserted into presentation queue

## Architecture Diagram

```
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│   Game Engine   │    │   lsfg-vk-afmf   │    │  FidelityFX SDK │
│                 │    │                  │    │                 │
│ vkQueuePresent  │───▶│ Vulkan Hooks     │───▶│ Frame Interp.   │
│ VkSwapchain     │    │ AFMF Interface   │    │ GPU Shaders     │
│ VkCommandBuffer │    │ Context Mgmt     │    │ Resource Mgmt   │
└─────────────────┘    └──────────────────┘    └─────────────────┘
                              │                        │
                              ▼                        ▼
                       ┌──────────────────┐    ┌─────────────────┐
                       │ Intercepted      │    │ Generated       │
                       │ Frame N         │    │ Frame N+0.5     │
                       │ (Real)          │    │ (Interpolated)  │
                       └──────────────────┘    └─────────────────┘
```

## File Structure After Integration

```
lsfg-vk-afmf/
├── external/                    # Git submodules
│   └── FidelityFX-SDK/         # AMD's SDK (submodule)
│       ├── sdk/
│       │   ├── include/        # Headers we include
│       │   ├── src/            # Source we compile
│       │   └── CMakeLists.txt  # Build config
│       └── samples/            # Reference implementations
├── src/
│   ├── afmf.cpp               # Our wrapper around FidelityFX
│   └── hooks.cpp              # Vulkan interception
├── include/
│   └── afmf.hpp               # Our public interface
└── CMakeLists.txt             # Links to FidelityFX libraries
```

## Legal Compliance

**MIT License Requirements:**
- ✅ Include FidelityFX LICENSE.txt in our distribution
- ✅ Credit AMD in our documentation  
- ✅ Preserve copyright notices in any copied code
- ✅ No restrictions on our use (commercial OK)

## Next Steps

Would you like me to:

1. **Set up the git submodule integration** (recommended first step)
2. **Update CMakeLists.txt for proper submodule build**
3. **Test the build with real FidelityFX linkage**
4. **Document the integration for other developers**

The submodule approach gives us the best balance of:
- ✅ Full FidelityFX functionality
- ✅ Reproducible builds
- ✅ Easy maintenance and updates
- ✅ Self-contained project structure
