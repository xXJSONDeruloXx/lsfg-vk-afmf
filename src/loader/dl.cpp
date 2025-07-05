#include "loader/dl.hpp"
#include "log.hpp"

#include <vector>

using namespace Loader;

using dlopen_t  = void* (*)(const char*, int);
using dlsym_t   = void* (*)(void*, const char*);
using dlclose_t = int   (*)(void*);

// glibc exclusive function to get versioned symbols
extern "C" void* dlvsym(long, const char*, const char*);

namespace {
    // original function pointers
    dlopen_t  dlopen_ptr;
    dlsym_t   dlsym_ptr;
    dlclose_t dlclose_ptr;

    // map of all registered overrides
    auto& overrides() {
        // this has to be a function rather than a static variable
        // because of weird initialization order issues.
        static std::unordered_map<std::string, DL::File> overrides;
        return overrides;
    }

    // vector of loaded handles
    auto& handles() {
        static std::vector<void*> handles;
        return handles;
    }

    bool enable_hooks{true};
}

void DL::initialize() {
    if (dlopen_ptr || dlsym_ptr || dlclose_ptr) {
        Log::warn("lsfg-vk(dl): Dynamic loader already initialized, did you call it twice?");
        return;
    }

    dlopen_ptr  = reinterpret_cast<dlopen_t> (dlvsym(-1, "dlopen",  "GLIBC_2.2.5"));
    dlsym_ptr   = reinterpret_cast<dlsym_t>  (dlvsym(-1, "dlsym",   "GLIBC_2.2.5"));
    dlclose_ptr = reinterpret_cast<dlclose_t>(dlvsym(-1, "dlclose", "GLIBC_2.2.5"));
    if (!dlopen_ptr || !dlsym_ptr || !dlclose_ptr) {
        Log::error("lsfg-vk(dl): Failed to initialize dynamic loader, missing symbols");
        exit(EXIT_FAILURE);
    }

    Log::debug("lsfg-vk(dl): Initialized dynamic loader with original functions");
}

void DL::registerFile(const File& file) {
    auto& files = overrides();

    auto it = files.find(file.getFilename());
    if (it == files.end()) {
        // simply register if the file hasn't been registered yet
        files.emplace(file.getFilename(), file);
        return;
    }

    // merge the new file's symbols into the previously registered one
    auto& existing_file = it->second;
    for (const auto& [symbol, func] : file.getSymbols())
        if (existing_file.findSymbol(symbol) == nullptr)
            existing_file.defineSymbol(symbol, func);
        else
            Log::warn("lsfg-vk(dl): Tried registering symbol {}::{}, but it is already defined",
                existing_file.getFilename(), symbol);
}

void DL::disableHooks() { enable_hooks = false; }
void DL::enableHooks()  { enable_hooks = true; }

void* dlopen(const char* filename, int flag) noexcept {
    auto& files = overrides();
    auto& loaded = handles();

    // ALWAYS load the library and ensure it's tracked
    auto* handle = dlopen_ptr(filename, flag);
    if (handle && std::ranges::find(loaded, handle) == loaded.end())
        loaded.push_back(handle);

    // no need to check for overrides if hooks are disabled
    if (!enable_hooks || !filename)
        return handle;

    // try to find an override for this filename
    const std::string filename_str(filename);
    auto it = files.find(filename_str);
    if (it == files.end())
        return handle;

    auto& file = it->second;
    file.setOriginalHandle(handle);
    file.setHandle(reinterpret_cast<void*>(&file));

    Log::debug("lsfg-vk(dl): Intercepted module load for {}", file.getFilename());
    return file.getHandle();
}

void* dlsym(void* handle, const char* symbol) noexcept {
    const auto& files = overrides();

    if (!enable_hooks || !handle || !symbol)
        return dlsym_ptr(handle, symbol);

    // see if handle is a fake one
    const auto it = std::ranges::find_if(files, [handle](const auto& pair) {
        return pair.second.getHandle() == handle;
    });
    if (it == files.end())
        return dlsym_ptr(handle, symbol);
    const auto& file = it->second;

    // find a symbol override
    const std::string symbol_str(symbol);
    auto* func = file.findSymbol(symbol_str);
    if (func == nullptr)
        return dlsym_ptr(file.getOriginalHandle(), symbol);

    Log::debug("lsfg-vk(dl): Intercepted symbol {}::{}", file.getFilename(), symbol_str);
    return func;
}

int dlclose(void* handle) noexcept {
    auto& files = overrides();
    auto& loaded = handles();

    // no handle, let the original dlclose handle it
    if (!handle)
        return dlclose_ptr(handle);

    // see if the handle is a fake one
    auto it = std::ranges::find_if(files, [handle](const auto& pair) {
        return pair.second.getHandle() == handle;
    });
    if (it == files.end()) {
        // if the handle is not fake, check if it's still loaded.
        // this is necessary to avoid double closing when
        // one handle was acquired while hooks were disabled
        auto l_it = std::ranges::find(loaded, handle);
        if (l_it == loaded.end())
            return 0;
        loaded.erase(l_it);
        return dlclose_ptr(handle);
    }

    auto& file = it->second;
    handle = file.getOriginalHandle();
    file.setHandle(nullptr);
    file.setOriginalHandle(nullptr);

    // similarly, if it is fake, check if it's still loaded
    // before unloading it again.
    auto l_it = std::ranges::find(loaded, handle);
    if (l_it == loaded.end()) {
        Log::debug("lsfg-vk(dl): Skipping unload for {} (already unloaded)", file.getFilename());
        return 0;
    }
    loaded.erase(l_it);

    Log::debug("lsfg-vk(dl): Unloaded {}", file.getFilename());
    return dlclose_ptr(handle);
}

// original function calls

void* DL::odlopen(const char* filename, int flag) {
    return dlopen_ptr(filename, flag);
}

void* DL::odlsym(void* handle, const char* symbol) {
    return dlsym_ptr(handle, symbol);
}

int DL::odlclose(void* handle) {
    return dlclose_ptr(handle);
}
