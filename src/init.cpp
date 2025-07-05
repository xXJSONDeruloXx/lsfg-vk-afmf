#include "loader/dl.hpp"
#include "loader/vk.hpp"
#include "hooks.hpp"
#include "log.hpp"

extern "C" void __attribute__((constructor)) init();
extern "C" [[noreturn]] void __attribute__((destructor)) deinit();

void init() {
    Log::info("lsfg-vk-afmf: init() called");

    // hook loaders
    Loader::DL::initialize();
    Loader::VK::initialize();

    // setup hooks
    Hooks::initialize();

    Log::info("lsfg-vk-afmf: init() completed successfully");
}

void deinit() {
    Log::debug("lsfg-vk-afmf: deinit() called, exiting");
    // for some reason some applications unload the library despite it containing
    // the dl functions. this will lead to a segmentation fault, so we exit early.
    exit(EXIT_SUCCESS);
}
