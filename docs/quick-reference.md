# Quick Reference

## Current Project Status (July 5, 2025)

### ✅ Working Build System
```bash
# Build command
cd lsfg-vk-afmf && ./build.sh

# Output
build/liblsfg-vk-afmf.so        # 61KB shared library
build/compile_commands.json     # IDE integration
```

### ✅ Completed Implementation
```
lsfg-vk-afmf/
├── src/                        # All files compile successfully
│   ├── afmf.cpp               # AFMF interface (stubs working)
│   ├── hooks.cpp              # Vulkan hooking (updated for AFMF)
│   ├── context.cpp            # Context management (updated)
│   ├── init.cpp               # Library init
│   └── loader/, mini/         # Supporting code
├── include/                    # All headers working
│   ├── afmf.hpp               # Main interface
│   ├── log.hpp                # Enhanced logging with fallbacks
│   └── ...                    # Supporting headers
└── CMakeLists.txt             # Working build config
```

### 🎯 Next Phase: FidelityFX Integration

**Current State**: All stub implementations working
**Target**: Replace stubs with FidelityFX calls

## File Locations

### Original lsfg-vk Repository
```
../lsfg-vk/
├── src/
│   ├── hooks.cpp                     # Vulkan API hooking (218 lines)
│   ├── context.cpp                   # Frame generation context (162 lines)
│   ├── init.cpp                      # Library initialization
│   └── utils.cpp                     # Utility functions
├── include/
│   ├── hooks.hpp                     # Hook interfaces
│   ├── context.hpp                   # Context management (84 lines)
│   └── utils.hpp                     # Helper functions
├── lsfg-vk-gen/                      # LSFG interface wrapper
│   ├── public/lsfg.hpp               # Main LSFG API (83 lines)
│   └── src/lsfg.cpp                  # LSFG implementation (84 lines)
└── CMakeLists.txt                    # Build configuration
```

### FidelityFX SDK Repository
```
../FidelityFX-SDK/
├── sdk/include/FidelityFX/host/
│   ├── ffx_frameinterpolation.h     # Core frame interpolation API
│   ├── ffx_fsr3.h                   # FSR3 complete solution
│   ├── ffx_opticalflow.h            # Motion estimation
│   └── backends/vk/                 # Vulkan backend headers
├── sdk/src/
│   ├── components/frameinterpolation/
│   │   └── ffx_frameinterpolation.cpp  # Frame interpolation implementation
│   ├── components/fsr3/
│   │   └── ffx_fsr3.cpp             # FSR3 implementation
│   └── backends/vk/
│       └── FrameInterpolationSwapchain/  # Vulkan swapchain replacement
├── samples/fsrapi/
│   └── fsrapirendermodule.cpp       # Integration example (1661 lines)
└── docs/techniques/
    ├── frame-interpolation.md       # Frame interpolation documentation
    └── super-resolution-interpolation.md  # FSR3 documentation
```

## Key API Mappings

### Current LSFG → Target FidelityFX

| LSFG Function | FidelityFX Equivalent | Notes |
|---------------|----------------------|-------|
| `LSFG::initialize()` | `ffxGetInterfaceVK()` | Backend initialization |
| `LSFG::createContext()` | `ffxFrameInterpolationContextCreate()` | Context creation |
| `LSFG::presentContext()` | `ffxFrameInterpolationDispatch()` | Frame interpolation |
| `LSFG::deleteContext()` | `ffxFrameInterpolationContextDestroy()` | Cleanup |
| `LSFG::vulkan_error` | `FfxErrorCode` | Error handling |

### Environment Variables

| Current | Proposed | Purpose |
|---------|----------|---------|
| `LSFG_DLL_PATH` | *(removed)* | No longer needed - native implementation |
| `LSFG_MULTIPLIER` | `AFMF_MULTIPLIER` | Frame generation multiplier |
| *(new)* | `AFMF_ENABLE_OPTICAL_FLOW` | Enhanced motion estimation |
| *(new)* | `AFMF_ENABLE_FSR3` | FSR3 upscaling support |

## Command Reference

### Building Original lsfg-vk
```bash
cd ../lsfg-vk/
cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=$PWD/release
cmake --build build
cmake --install build
```

### Usage (Original)
```bash
LD_PRELOAD="/path/to/liblsfg-vk.so" \
LSFG_DLL_PATH="/path/to/Lossless.dll" \
LSFG_MULTIPLIER=4 \
vkcube
```

### Proposed Usage (AFMF)
```bash
LD_PRELOAD="/path/to/liblsfg-vk-afmf.so" \
AFMF_MULTIPLIER=4 \
AFMF_ENABLE_OPTICAL_FLOW=1 \
vkcube
```

## Implementation Phases

### Phase 1: Direct Replacement *(Weeks 1-4)*
- [ ] Create AFMF interface wrapper
- [ ] Replace `lsfg-vk-gen` with FidelityFX calls
- [ ] Maintain existing hook structure
- [ ] Test basic frame generation

### Phase 2: Swapchain Integration *(Weeks 5-6)*
- [ ] Integrate `FrameInterpolationSwapchainVK`
- [ ] Simplify hook implementation
- [ ] Add automatic frame pacing
- [ ] Support UI composition

### Phase 3: Advanced Features *(Weeks 7-8)*
- [ ] Add FSR3 upscaling
- [ ] Integrate optical flow
- [ ] Performance optimization
- [ ] Advanced configuration

## Key Dependencies

### Required Extensions (Vulkan Instance)
```cpp
// From lsfg-vk/src/hooks.cpp:25-30
"VK_KHR_get_physical_device_properties2",
"VK_KHR_external_memory_capabilities", 
"VK_KHR_external_semaphore_capabilities"
```

### Required Extensions (Vulkan Device)
```cpp
// From lsfg-vk/src/hooks.cpp:52-57
"VK_KHR_external_memory",
"VK_KHR_external_memory_fd",
"VK_KHR_external_semaphore", 
"VK_KHR_external_semaphore_fd"
```

### FidelityFX SDK Components
```cmake
# CMake dependencies
ffx_frameinterpolation_vk    # Core frame interpolation
ffx_fsr3_vk                  # FSR3 upscaling (Phase 3)
ffx_opticalflow_vk           # Motion estimation (Phase 2/3)
```

## Testing Strategy

### Unit Tests
- Context creation/destruction
- Frame interpolation accuracy
- Resource management
- Error handling

### Integration Tests
```bash
# Simple Vulkan applications
vkcube
vkgears

# Real games (compatibility validation)
# Use same test suite as original lsfg-vk
```

### Performance Benchmarks
- Frame time comparison vs. lsfg-vk
- Memory usage analysis
- GPU utilization metrics
- Visual quality assessment

## Troubleshooting

### Common Issues

1. **Missing FidelityFX SDK**
   ```bash
   # Ensure FidelityFX-SDK is at ../FidelityFX-SDK/
   ls ../FidelityFX-SDK/sdk/include/FidelityFX/host/
   ```

2. **Vulkan Extension Missing**
   ```bash
   # Check Vulkan instance/device extensions
   vulkaninfo | grep -E "(external_memory|external_semaphore)"
   ```

3. **AFMF Context Creation Failed**
   ```bash
   # Check environment variables
   echo $AFMF_MULTIPLIER
   echo $LD_PRELOAD
   ```

### Debug Mode
```bash
# Enable debug output
AFMF_DEBUG_MODE=1 \
AFMF_MULTIPLIER=2 \
LD_PRELOAD="/path/to/liblsfg-vk-afmf.so" \
vkcube
```

## Next Steps

1. **Start with Phase 1**: Create basic project structure and AFMF interface
2. **Study Reference Code**: Analyze `fsrapirendermodule.cpp` for integration patterns
3. **Create Test Plan**: Set up validation strategy against original lsfg-vk
4. **Implement Incrementally**: Build and test each component separately

## Useful Commands

### Code Analysis
```bash
# Count lines in original implementation
find ../lsfg-vk/src -name "*.cpp" -exec wc -l {} +

# Find FidelityFX frame interpolation examples
grep -r "ffxFrameInterpolation" ../FidelityFX-SDK/samples/

# Check FidelityFX API documentation
find ../FidelityFX-SDK/docs -name "*frame*" -o -name "*interpolation*"
```

### Development Workflow
```bash
# Create feature branch
git checkout -b feature/afmf-phase1

# Build and test
mkdir build && cd build
cmake .. && make -j$(nproc)

# Test with simple application
LD_PRELOAD="./liblsfg-vk-afmf.so" vkcube
```
