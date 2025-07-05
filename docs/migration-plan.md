# Migration Plan

## Current Status (July 5, 2025)

### ✅ Phase 1: Foundation Complete
- [x] **File Structure**: Copied all necessary files from `lsfg-vk` to `lsfg-vk-afmf`
- [x] **Interface Replacement**: Created `include/afmf.hpp` to replace `lsfg.hpp`
- [x] **Code Migration**: Updated all LSFG references to AFMF in source files
- [x] **CMake Configuration**: Updated `CMakeLists.txt` for the new project structure
- [x] **AFMF Implementation Stub**: Created `src/afmf.cpp` with FidelityFX SDK placeholders
- [x] **Build System Working**: Successfully compiles to `liblsfg-vk-afmf.so` (61KB)
- [x] **C++20 Compatibility**: Fixed std::format and std::ranges compatibility issues for older toolchains
- [x] **CI/CD Ready**: GitHub Actions workflow validated

### 📂 Files Successfully Built
```bash
# Working Build Output
build/liblsfg-vk-afmf.so     # 61KB shared library
build/compile_commands.json  # For IDE integration

# Core Implementation Files (All Compiling)
include/afmf.hpp           # New AFMF interface (replaces lsfg.hpp)
src/afmf.cpp              # New AFMF implementation with FidelityFX placeholders
src/hooks.cpp             # Updated to use AFMF instead of LSFG  
src/context.cpp           # Updated to use AFMF instead of LSFG
src/init.cpp              # Updated project branding

# Supporting Infrastructure (All Working)
include/hooks.hpp, src/hooks.cpp
include/utils.hpp, src/utils.cpp
include/log.hpp            # Enhanced with fallback for std::format
include/loader/, src/loader/  # Fixed std::ranges compatibility
include/mini/, src/mini/
CMakeLists.txt            # Updated for lsfg-vk-afmf project
build.sh                  # Local build script
```

### 🎯 Phase 2: FidelityFX SDK Integration (Next Steps)
1. **FidelityFX SDK Integration** - Add proper CMake configuration for FidelityFX SDK
2. **FidelityFX Implementation** - Replace TODO placeholders with actual FidelityFX calls
3. **Testing Framework** - Set up basic testing with simple Vulkan applications
4. **Performance Validation** - Compare with original Lossless.dll implementation

## Overview

This document outlines a step-by-step plan to migrate from the current `lsfg-vk` implementation using `Lossless.dll` to a native FidelityFX SDK-based solution.

## Phase 1: Direct API Replacement

### Goal
Replace the `lsfg-vk-gen` interface with FidelityFX Frame Interpolation while maintaining the existing Vulkan hook structure.

### Steps

#### 1.1 Environment Setup
```bash
# In /Users/kurt/Developer/lsfg/lsfg-vk-afmf/
mkdir -p src include cmake
```

#### 1.2 FidelityFX Integration
- **Reference**: `FidelityFX-SDK/sdk/CMakeLists.txt`
- **Action**: Create CMake configuration to include FidelityFX SDK
- **Files to Study**:
  - `FidelityFX-SDK/sdk/include/FidelityFX/host/ffx_frameinterpolation.h`
  - `FidelityFX-SDK/sdk/src/components/frameinterpolation/ffx_frameinterpolation.cpp`

#### 1.3 Replace LSFG Interface
**Current Interface** (`lsfg-vk/lsfg-vk-gen/public/lsfg.hpp`):
```cpp
namespace LSFG {
    void initialize();
    int32_t createContext(uint32_t width, uint32_t height, int in0, int in1, const std::vector<int>& outN);
    void presentContext(int32_t id, int inSem, const std::vector<int>& outSem);
    void deleteContext(int32_t id);
    void finalize();
}
```

**New FidelityFX Interface** (to be created):
```cpp
namespace AFMF {
    void initialize();
    int32_t createContext(uint32_t width, uint32_t height, int in0, int in1, const std::vector<int>& outN);
    void presentContext(int32_t id, int inSem, const std::vector<int>& outSem);
    void deleteContext(int32_t id);
    void finalize();
}
```

#### 1.4 Context Management Translation
**Reference Files**:
- `lsfg-vk/src/context.cpp` (current implementation)
- `FidelityFX-SDK/sdk/src/components/frameinterpolation/ffx_frameinterpolation.cpp` (target implementation)

**Key Mapping**:
- `LsContext` → `FfxFrameInterpolationContext`
- Frame buffer management → FidelityFX resource management
- Semaphore handling → FidelityFX synchronization

#### 1.5 Hook Adaptation
**Reference**: `lsfg-vk/src/hooks.cpp`
**Changes Required**:
- Replace `#include <lsfg.hpp>` with `#include <afmf.hpp>`
- Update device info structure to match FidelityFX requirements
- Adapt extension requirements for FidelityFX

### Deliverables Phase 1
- [ ] CMake configuration for FidelityFX SDK integration
- [ ] AFMF interface header (`include/afmf.hpp`)
- [ ] AFMF implementation (`src/afmf.cpp`)
- [ ] Basic context management
- [ ] Hook adaptation for new interface

## Phase 2: Swapchain Integration

### Goal
Replace custom swapchain management with FidelityFX Frame Interpolation Swapchain.

### Steps

#### 2.1 Study FidelityFX Swapchain Implementation
**Key Files to Analyze**:
- `FidelityFX-SDK/sdk/src/backends/vk/FrameInterpolationSwapchain/FrameInterpolationSwapchainVK.cpp`
- `FidelityFX-SDK/sdk/src/backends/vk/FrameInterpolationSwapchain/FrameInterpolationSwapchainVK.h`
- `FidelityFX-SDK/samples/fsrapi/fsrapirendermodule.cpp` (integration example)

#### 2.2 Swapchain Hook Replacement
**Current Implementation** (`lsfg-vk/src/hooks.cpp:100-150`):
```cpp
VkResult myvkCreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR* pCreateInfo, 
                                const VkAllocationCallbacks* pAllocator, VkSwapchainKHR* pSwapchain)
```

**Target Implementation**:
- Use `FrameInterpolationSwapChainVK` class
- Integrate frame generation configuration
- Handle automatic UI composition

#### 2.3 Present Logic Simplification
**Current Complex Logic** (`lsfg-vk/src/context.cpp:50-162`):
- Manual swapchain image acquisition
- Custom semaphore management
- Manual frame copying and presentation

**Target Simplified Logic**:
- Use FidelityFX built-in frame pacing
- Automatic frame interpolation
- Built-in synchronization

### Deliverables Phase 2
- [ ] FidelityFX Swapchain integration
- [ ] Simplified hook implementation
- [ ] Automatic frame pacing
- [ ] UI composition support

## Phase 3: Full FidelityFX Integration

### Goal
Add FSR3 upscaling and advanced features for a complete solution.

### Steps

#### 3.1 FSR3 Integration
**Reference**: `FidelityFX-SDK/sdk/include/FidelityFX/host/ffx_fsr3.h`
**Features to Add**:
- Temporal upscaling
- Motion vector enhancement
- Reactive mask support

#### 3.2 Optical Flow Integration
**Reference**: `FidelityFX-SDK/sdk/include/FidelityFX/host/ffx_opticalflow.h`
**Benefits**:
- Better motion estimation
- Higher quality frame interpolation
- Reduced artifacts

#### 3.3 Advanced Configuration
**Environment Variables Mapping**:
```bash
# Current
LSFG_MULTIPLIER=4

# Proposed
AFMF_MULTIPLIER=4
AFMF_UPSCALE_RATIO=1.5  # New: FSR3 upscaling
AFMF_MOTION_ESTIMATION=optical_flow  # New: Enhanced motion vectors
```

### Deliverables Phase 3
- [ ] FSR3 upscaling integration
- [ ] Optical flow motion estimation
- [ ] Advanced configuration options
- [ ] Performance optimizations

## Implementation Timeline

### Week 1-2: Phase 1 Foundation
- Set up project structure
- Integrate FidelityFX SDK
- Create basic AFMF interface
- Adapt hooks for FidelityFX

### Week 3-4: Phase 1 Completion
- Implement frame interpolation context
- Test basic frame generation
- Debug and optimize

### Week 5-6: Phase 2 Implementation
- Integrate FidelityFX swapchain
- Simplify presentation logic
- Add UI composition support

### Week 7-8: Phase 3 Enhancement
- Add FSR3 upscaling
- Integrate optical flow
- Performance testing and optimization

## Testing Strategy

### Unit Testing
- Context creation/destruction
- Frame interpolation accuracy
- Memory management

### Integration Testing
- Game compatibility testing
- Performance benchmarking vs. original lsfg-vk
- Visual quality comparison

### Target Games for Testing
```bash
# Simple test cases
vkcube
vkgears

# Real games (same as original lsfg-vk testing)
# Whatever games were used to validate lsfg-vk
```

## Success Criteria

1. **Functionality**: Frame generation works with same games as original lsfg-vk
2. **Performance**: Equal or better performance than lossless.dll solution
3. **Quality**: Visual quality matches or exceeds original implementation
4. **Stability**: No crashes or memory leaks during extended testing
5. **Compatibility**: Works with same range of Vulkan applications
