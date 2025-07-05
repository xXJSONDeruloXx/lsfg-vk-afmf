// Simplified FidelityFX interface for lsfg-vk-afmf
// Based on FidelityFX SDK - Adapted for Linux compilation

#pragma once

#include <ffx/host/ffx_assert.h>
#include <ffx/host/ffx_types.h>
#include <ffx/host/ffx_error.h>
#include <ffx/host/ffx_message.h>

#ifdef __cplusplus
extern "C" {
#endif

// Interface structure (simplified)
typedef struct FfxInterface {
    void* scratchBuffer;
    uint32_t scratchBufferSize;
    
    // Function pointers for backend operations
    FfxErrorCode (*fpCreateDevice)(FfxInterface* backendInterface, void* device);
    FfxErrorCode (*fpGetDeviceCapabilities)(FfxInterface* backendInterface, void* deviceCapabilities);
    FfxErrorCode (*fpCreateResource)(FfxInterface* backendInterface, const FfxCreateResourceDescription* createResourceDescription, FfxResource* outResource);
    FfxErrorCode (*fpRegisterResource)(FfxInterface* backendInterface, const void* inResource, FfxResource* outResource);
    FfxErrorCode (*fpUnregisterResources)(FfxInterface* backendInterface);
    FfxErrorCode (*fpGetResourceDescription)(FfxInterface* backendInterface, FfxResource resource, FfxCreateResourceDescription* outResourceDescription);
    FfxErrorCode (*fpDestroyResource)(FfxInterface* backendInterface, FfxResource resource);
    FfxErrorCode (*fpCreatePipeline)(FfxInterface* backendInterface, uint32_t effectContextId, const void* pipelineDescription, void* outPipeline);
    FfxErrorCode (*fpDestroyPipeline)(FfxInterface* backendInterface, void* pipeline);
    FfxErrorCode (*fpScheduleGpuJob)(FfxInterface* backendInterface, const void* job);
    FfxErrorCode (*fpExecuteGpuJobs)(FfxInterface* backendInterface, FfxCommandList commandList);
} FfxInterface;

#ifdef __cplusplus
}
#endif
