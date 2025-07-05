cmake_minimum_required(VERSION 3.29)

# cmake options
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_CLANG_TIDY clang-tidy)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_SKIP_RPATH ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# subprojects
# TODO: Re-enable these once dependencies are properly set up
# include(cmake/FetchDXVK.cmake)
# include(cmake/FetchPeParse.cmake)

# TODO: Remove this when AFMF is fully implemented
# add_subdirectory(lsfg-vk-gen)

# main project
project(lsfg-vk-afmf
    VERSION 0.0.1
    DESCRIPTION "lsfg-vk-afmf: AMD FidelityFX Motion Frames on Linux through Vulkan"
    LANGUAGES CXX)

file(GLOB SOURCES
    "src/loader/*.cpp"
    "src/mini/*.cpp"
    "src/*.cpp"
)

add_library(lsfg-vk-afmf SHARED ${SOURCES})

target_include_directories(lsfg-vk-afmf
    PRIVATE include
    # FidelityFX SDK include paths (will be added once SDK is integrated)
    # PRIVATE ${CMAKE_SOURCE_DIR}/../FidelityFX-SDK/sdk/include
    )
target_link_libraries(lsfg-vk-afmf
    PRIVATE vulkan
    # TODO: Add FidelityFX SDK libraries once integrated
    # PRIVATE ${FidelityFX_SDK_LIBRARIES}
    )
target_compile_options(lsfg-vk-afmf PRIVATE
    -Weverything
    # disable compat c++ flags
    -Wno-pre-c++20-compat-pedantic
    -Wno-pre-c++17-compat
    -Wno-c++98-compat-pedantic
    -Wno-c++98-compat
    # disable other flags
    -Wno-missing-designated-field-initializers
    -Wno-shadow         # allow shadowing
    -Wno-switch-enum    # ignore missing cases
    -Wno-switch-default # ignore missing default
    -Wno-padded         # ignore automatic padding
    -Wno-exit-time-destructors # allow globals
    -Wno-global-constructors   # allow globals
    # required for vulkan
    -Wno-cast-function-type-strict
)

install(FILES "${CMAKE_BINARY_DIR}/liblsfg-vk-afmf.so" DESTINATION lib)
