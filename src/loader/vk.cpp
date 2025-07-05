#include "loader/vk.hpp"
#include "loader/dl.hpp"
#include "log.hpp"

using namespace Loader;

namespace {
    // original function pointers
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr_ptr{};
    PFN_vkGetDeviceProcAddr   vkGetDeviceProcAddr_ptr{};

    // map of all overridden symbols
    auto& symbols() {
        static std::unordered_map<std::string, void*> symbols;
        return symbols;
    }
}

void VK::initialize() {
    if (vkGetInstanceProcAddr_ptr || vkGetDeviceProcAddr_ptr) {
        Log::warn("lsfg-vk(vk): Vulkan loader already initialized, did you call it twice?");
        return;
    }

    // get original function pointers
    auto* handle = DL::odlopen("libvulkan.so.1", 0x2);
    vkGetInstanceProcAddr_ptr =
        reinterpret_cast<PFN_vkGetInstanceProcAddr>(DL::odlsym(handle, "vkGetInstanceProcAddr"));
    vkGetDeviceProcAddr_ptr =
        reinterpret_cast<PFN_vkGetDeviceProcAddr>  (DL::odlsym(handle, "vkGetDeviceProcAddr"));
    if (!vkGetInstanceProcAddr_ptr || !vkGetDeviceProcAddr_ptr) {
        Log::error("lsfg-vk(vk): Failed to initialize Vulkan loader, missing symbols");
        exit(EXIT_FAILURE);
    }

    // register dynamic loader overrides
    DL::File vulkanLib{"libvulkan.so.1"};
    vulkanLib.defineSymbol("vkGetInstanceProcAddr",
        reinterpret_cast<void*>(myvkGetInstanceProcAddr));
    vulkanLib.defineSymbol("vkGetDeviceProcAddr",
        reinterpret_cast<void*>(myvkGetDeviceProcAddr));
    DL::registerFile(vulkanLib);

    DL::File vulkanLib2{"libvulkan.so"};
    vulkanLib2.defineSymbol("vkGetInstanceProcAddr",
        reinterpret_cast<void*>(myvkGetInstanceProcAddr));
    vulkanLib2.defineSymbol("vkGetDeviceProcAddr",
        reinterpret_cast<void*>(myvkGetDeviceProcAddr));
    DL::registerFile(vulkanLib2);

    // register vulkan loader overrides
    VK::registerSymbol("vkGetInstanceProcAddr", reinterpret_cast<void*>(myvkGetInstanceProcAddr));
    VK::registerSymbol("vkGetDeviceProcAddr", reinterpret_cast<void*>(myvkGetDeviceProcAddr));

    Log::debug("lsfg-vk(vk): Initialized Vulkan loader with original functions");
}

void VK::registerSymbol(const std::string& symbol, void* address) {
    auto& syms = symbols();

    const auto it = syms.find(symbol);
    if (it != syms.end()) {
        Log::warn("lsfg-vk(vk): Tried registering symbol {}, but it is already defined", symbol);
        return;
    }

    syms.emplace(symbol, address);
}

PFN_vkVoidFunction myvkGetInstanceProcAddr(VkInstance instance, const char* pName) {
    const auto& syms = symbols();

    if (!pName)
        return vkGetInstanceProcAddr_ptr(instance, pName);

    // try to find an override
    const std::string pName_str(pName);
    const auto it = syms.find(pName_str);
    if (it == syms.end())
        return vkGetInstanceProcAddr_ptr(instance, pName);

    Log::debug("lsfg-vk(vk): Intercepted Vulkan symbol {}", pName_str);
    return reinterpret_cast<PFN_vkVoidFunction>(it->second);
}

PFN_vkVoidFunction myvkGetDeviceProcAddr(VkDevice device, const char* pName) {
    const auto& syms = symbols();

    if (!pName)
        return vkGetDeviceProcAddr_ptr(device, pName);

    const std::string pName_str(pName);
    auto it = syms.find(pName_str);
    if (it == syms.end())
        return vkGetDeviceProcAddr_ptr(device, pName);

    Log::debug("lsfg-vk(vk): Intercepted Vulkan symbol {}", pName_str);
    return reinterpret_cast<PFN_vkVoidFunction>(it->second);
}

// original function calls

PFN_vkVoidFunction VK::ovkGetInstanceProcAddr(VkInstance instance, const char* pName) {
    return vkGetInstanceProcAddr_ptr(instance, pName);
}

PFN_vkVoidFunction VK::ovkGetDeviceProcAddr(VkDevice device, const char* pName) {
    return vkGetDeviceProcAddr_ptr(device, pName);
}
