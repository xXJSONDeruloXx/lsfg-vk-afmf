# lsfg-vk-afmf
**Linux Native Frame Generation using AMD FidelityFX SDK**

## Project Overview

This project aims to replace the Windows-dependent `Lossless.dll` in the existing `lsfg-vk` project with AMD's open-source FidelityFX SDK, creating a truly native Linux frame generation solution.

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
│   └── implementation.md     # Implementation details
├── src/                      # Source code (future)
├── include/                  # Headers (future)
└── examples/                 # Example implementations (future)
```

## Quick Start

See `docs/migration-plan.md` for detailed implementation steps.

## References

- **Original Project**: `../lsfg-vk/` - Linux frame generation using Lossless.dll
- **FidelityFX SDK**: `../FidelityFX-SDK/` - AMD's open-source rendering techniques
- **Key Sample**: `../FidelityFX-SDK/samples/fsrapi/` - Reference implementation

## License

TBD - Will align with FidelityFX SDK licensing (MIT)
