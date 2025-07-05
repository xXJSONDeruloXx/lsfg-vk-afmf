// Simplified FidelityFX types for lsfg-vk-afmf
// Based on FidelityFX SDK - Adapted for Linux compilation

#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct FfxInterface FfxInterface;

// Version information
typedef uint32_t FfxVersionNumber;

// Basic types
typedef struct FfxDimensions2D {
    uint32_t width;
    uint32_t height;
} FfxDimensions2D;

typedef struct FfxFloatCoords2D {
    float x;
    float y;
} FfxFloatCoords2D;

typedef struct FfxFloat32x3 {
    float x, y, z;
} FfxFloat32x3;

typedef struct FfxRect2D {
    int32_t left;
    int32_t top;
    int32_t width;
    int32_t height;
} FfxRect2D;

// Surface format enumeration
typedef enum FfxSurfaceFormat {
    FFX_SURFACE_FORMAT_UNKNOWN,
    FFX_SURFACE_FORMAT_R8G8B8A8_UNORM,
    FFX_SURFACE_FORMAT_B8G8R8A8_UNORM,
    FFX_SURFACE_FORMAT_R16G16B16A16_FLOAT,
    FFX_SURFACE_FORMAT_R32G32B32A32_FLOAT,
    FFX_SURFACE_FORMAT_R11G11B10_FLOAT,
    FFX_SURFACE_FORMAT_R16G16_FLOAT,
    FFX_SURFACE_FORMAT_R32_UINT,
    FFX_SURFACE_FORMAT_R8_UNORM,
    FFX_SURFACE_FORMAT_R8_UINT,
    FFX_SURFACE_FORMAT_R16_UINT,
    FFX_SURFACE_FORMAT_R16_UNORM,
    FFX_SURFACE_FORMAT_R16_FLOAT,
    FFX_SURFACE_FORMAT_R32_FLOAT,
    FFX_SURFACE_FORMAT_COUNT
} FfxSurfaceFormat;

// Resource and command list types
typedef void* FfxCommandList;
typedef void* FfxResource;

// Error codes
typedef enum FfxErrorCode {
    FFX_OK = 0,
    FFX_ERROR_INVALID_POINTER,
    FFX_ERROR_INVALID_ALIGNMENT,
    FFX_ERROR_INVALID_SIZE,
    FFX_ERROR_EOF,
    FFX_ERROR_INVALID_PATH,
    FFX_ERROR_ERROR,
    FFX_ERROR_INCOMPLETE_INTERFACE,
    FFX_ERROR_INVALID_ENUM,
    FFX_ERROR_INVALID_ARGUMENT,
    FFX_ERROR_OUT_OF_MEMORY,
    FFX_ERROR_NULL_DEVICE,
    FFX_ERROR_BACKEND_API_ERROR,
    FFX_ERROR_INSUFFICIENT_MEMORY,
    FFX_ERROR_COUNT
} FfxErrorCode;

// Backbuffer transfer function
typedef enum FfxBackbufferTransferFunction {
    FFX_BACKBUFFER_TRANSFER_FUNCTION_SRGB = 0,
    FFX_BACKBUFFER_TRANSFER_FUNCTION_PQ = 1,
    FFX_BACKBUFFER_TRANSFER_FUNCTION_SCRGB = 2,
    FFX_BACKBUFFER_TRANSFER_FUNCTION_COUNT
} FfxBackbufferTransferFunction;

// Resource creation description
typedef struct FfxCreateResourceDescription {
    FfxSurfaceFormat format;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t mipCount;
    uint32_t flags;
} FfxCreateResourceDescription;

// Effect memory usage
typedef struct FfxEffectMemoryUsage {
    uint64_t totalUsageInBytes;
    uint64_t aliasableUsageInBytes;
} FfxEffectMemoryUsage;

// Message callback
typedef void (*ffxMessageCallback)(uint32_t type, const wchar_t* message);

// API definition
#ifndef FFX_API
#define FFX_API
#endif

// Default context size
#define FFX_SDK_DEFAULT_CONTEXT_SIZE (16536)

#ifdef __cplusplus
}
#endif
