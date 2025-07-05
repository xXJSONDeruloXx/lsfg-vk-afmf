#include <afmf.hpp>
#include "log.hpp"

#ifdef FFX_SDK_AVAILABLE
#include <FidelityFX/host/ffx_frameinterpolation.h>
#include <FidelityFX/host/backends/vk/ffx_vk.h>
#include <FidelityFX/host/ffx_interface.h>
#include <vulkan/vulkan.h>
#endif

#include <unordered_map>
#include <memory>

namespace AFMF {

namespace {

struct AFMFContext {
#ifdef FFX_SDK_AVAILABLE
    FfxFrameInterpolationContext ffxContext;
    FfxInterface backendInterface;
    bool ffxInitialized = false;
#endif
    uint32_t width, height;
    std::vector<int> outputDescriptors;
    int input0, input1;
};

std::unordered_map<int32_t, std::unique_ptr<AFMFContext>> contexts;
int32_t nextContextId = 1;
bool initialized = false;

#ifdef FFX_SDK_AVAILABLE
// Global backend state
FfxDevice ffxDevice = nullptr;
void* scratchBuffer = nullptr;
size_t scratchBufferSize = 0;
#endif

} // anonymous namespace

vulkan_error::vulkan_error(VkResult result, const std::string& message)
    : std::runtime_error(message), result(result) {
}

vulkan_error::~vulkan_error() noexcept = default;

void initialize() {
    if (initialized) {
        Log::warn("AFMF already initialized");
        return;
    }
    
    Log::info("Initializing AFMF (AMD FidelityFX Motion Frames)");
    
#ifdef FFX_SDK_AVAILABLE
    Log::info("FidelityFX SDK available - initializing native backend");
    
    // TODO: Get Vulkan device and physical device from our Vulkan hooks
    // For now, we'll defer this until we have proper device access
    // This will be implemented when we connect with the Vulkan hook system
    
    Log::info("FidelityFX backend initialization deferred until device creation");
#else
    Log::info("FidelityFX SDK not available - using stub implementation");
#endif
    
    initialized = true;
    Log::info("AFMF initialized successfully");
}

int32_t createContext(uint32_t width, uint32_t height, int in0, int in1, 
                      const std::vector<int>& outN) {
    if (!initialized) {
        throw vulkan_error(VK_ERROR_INITIALIZATION_FAILED, "AFMF not initialized");
    }
    
    Log::info("Creating AFMF context: {}x{}, inputs: {}, {}, outputs: {}", 
              width, height, in0, in1, outN.size());
    
    auto context = std::make_unique<AFMFContext>();
    context->width = width;
    context->height = height;
    context->input0 = in0;
    context->input1 = in1;
    context->outputDescriptors = outN;
    
#ifdef FFX_SDK_AVAILABLE
    Log::debug("Preparing FidelityFX Frame Interpolation context");
    
    // TODO: Create FidelityFX Frame Interpolation context
    // This requires Vulkan device access which will come from the hook system
    /*
    FfxFrameInterpolationContextDescription desc = {};
    desc.maxRenderSize.width = width;
    desc.maxRenderSize.height = height;
    desc.displaySize.width = width;
    desc.displaySize.height = height;
    desc.backBufferFormat = FFX_SURFACE_FORMAT_R8G8B8A8_UNORM; // TODO: Get from swapchain
    desc.backendInterface = backendInterface;
    
    FfxErrorCode result = ffxFrameInterpolationContextCreate(&context->ffxContext, &desc);
    if (result == FFX_OK) {
        context->ffxInitialized = true;
        Log::info("FidelityFX Frame Interpolation context created successfully");
    } else {
        Log::error("Failed to create FidelityFX context: {}", static_cast<int>(result));
    }
    */
    
    Log::debug("FidelityFX context creation deferred until Vulkan device available");
#else
    Log::debug("Using stub AFMF context (FidelityFX SDK not available)");
#endif
    
    int32_t id = nextContextId++;
    contexts[id] = std::move(context);
    
    Log::info("AFMF context created with ID: {}", id);
    return id;
}

void presentContext(int32_t id, int inSem, const std::vector<int>& outSem) {
    auto it = contexts.find(id);
    if (it == contexts.end()) {
        throw vulkan_error(VK_ERROR_INVALID_EXTERNAL_HANDLE, 
                          "Invalid context ID: " + std::to_string(id));
    }
    
    auto& context = it->second;
    
    Log::debug("Presenting AFMF context ID: {}, inSem: {}, outSem count: {}", 
               id, inSem, outSem.size());
    
    // TODO: Implement FidelityFX frame interpolation dispatch
    // FfxFrameInterpolationDispatchDescription dispatchDesc = {};
    // auto result = ffxFrameInterpolationDispatch(&context->context, &dispatchDesc);
}

void deleteContext(int32_t id) {
    auto it = contexts.find(id);
    if (it == contexts.end()) {
        Log::warn("Attempted to delete non-existent AFMF context ID: {}", id);
        return;
    }
    
    Log::info("Deleting AFMF context ID: {}", id);
    
#ifdef FFX_SDK_AVAILABLE
    auto& context = it->second;
    if (context->ffxInitialized) {
        FfxErrorCode result = ffxFrameInterpolationContextDestroy(&context->ffxContext);
        if (result != FFX_OK) {
            Log::error("Failed to destroy FidelityFX context: {}", static_cast<int>(result));
        } else {
            Log::debug("FidelityFX context destroyed successfully");
        }
    }
#endif
    
    contexts.erase(it);
}

void finalize() {
    if (!initialized) {
        return;
    }
    
    Log::info("Finalizing AFMF");
    
    // Clean up all remaining contexts
    for (auto& [id, context] : contexts) {
        Log::warn("Cleaning up remaining AFMF context ID: {}", id);
#ifdef FFX_SDK_AVAILABLE
        if (context->ffxInitialized) {
            FfxErrorCode result = ffxFrameInterpolationContextDestroy(&context->ffxContext);
            if (result != FFX_OK) {
                Log::error("Failed to destroy FidelityFX context {}: {}", id, static_cast<int>(result));
            }
        }
#endif
    }
    contexts.clear();
    
#ifdef FFX_SDK_AVAILABLE
    // Clean up global FidelityFX resources
    if (scratchBuffer) {
        free(scratchBuffer);
        scratchBuffer = nullptr;
        scratchBufferSize = 0;
    }
    Log::debug("FidelityFX backend resources cleaned up");
#endif
    
    initialized = false;
    Log::info("AFMF finalized");
}

} // namespace AFMF
