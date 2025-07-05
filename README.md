# lsfg-vk-afmf
**Linux Native Frame Generation using AMD FidelityFX SDK**

## Project Overview

This project aims to replace the Windows-dependent `Lossless.dll` in the existing `lsfg-vk` project with AMD's open-source FidelityFX SDK, creating a truly native Linux frame generation solution.

## Current Status ✅

**Phase 1 Complete**: Foundation build system working
- ✅ Compiles to `liblsfg-vk-afmf.so` (61KB)
- ✅ GitHub Actions CI/CD validated
- ✅ C++20 compatibility fixes for older toolchains
- ✅ Complete API migration from LSFG to AFMF

**Phase 2 Next**: FidelityFX SDK integration

## Quick Start

### Build Locally
```bash
cd lsfg-vk-afmf
./build.sh
# Output: build/liblsfg-vk-afmf.so
```

### Requirements
- CMake 3.22+
- Clang 14+ or GCC 12+
- Vulkan SDK
- Ninja (optional, faster builds)

## Goals

1. **Eliminate Windows Dependencies**: Remove reliance on `Lossless.dll` and DXVK translation layers
2. **Native Vulkan Implementation**: Use FidelityFX's native Vulkan backend for frame interpolation
3. **Enhanced Performance**: Leverage AMD-optimized shaders and algorithms
4. **Cross-Platform Compatibility**: Create a solution that works natively on Linux
5. **Open Source**: Provide a fully open-source frame generation implementation

## Repository Structure

```
lsfg-vk-afmf/
├── README.md                 # This file
├── docs/                     # Detailed documentation
│   ├── architecture.md       # System architecture overview
│   ├── migration-plan.md     # Step-by-step migration plan
│   ├── api-mapping.md        # API mapping between LSFG and FidelityFX
│   ├── implementation.md     # Implementation details
│   ├── build-status.md       # Current build status & next steps
│   └── quick-reference.md    # Developer quick reference
├── src/                      # Source code (working)
│   ├── afmf.cpp             # AFMF implementation (stub → FidelityFX)
│   ├── hooks.cpp            # Vulkan API interception
│   ├── context.cpp          # Context management
│   ├── init.cpp             # Library initialization
│   └── loader/, mini/       # Supporting infrastructure
├── include/                  # Headers (working)
│   ├── afmf.hpp             # Main AFMF interface
│   ├── hooks.hpp, context.hpp, log.hpp
│   └── loader/, mini/       # Supporting headers
├── build.sh                  # Local build script
├── CMakeLists.txt           # Build configuration
└── build/                   # Build output
    ├── liblsfg-vk-afmf.so  # 61KB shared library
    └── compile_commands.json
```

## Documentation

- **Current Status**: See `docs/build-status.md` for latest progress
- **Migration Plan**: `docs/migration-plan.md` for detailed implementation steps
- **Quick Reference**: `docs/quick-reference.md` for API usage

## References

- **Original Project**: `../lsfg-vk/` - Linux frame generation using Lossless.dll
- **FidelityFX SDK**: `../FidelityFX-SDK/` - AMD's open-source rendering techniques
- **Key Sample**: `../FidelityFX-SDK/samples/fsrapi/` - Reference implementation

## License

TBD - Will align with FidelityFX SDK licensing (MIT)
