cmake_minimum_required(VERSION 3.29)

# cmake options
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_CLANG_TIDY clang-tidy)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_SKIP_RPATH ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# FidelityFX SDK Integration
set(FFX_SDK_PATH "${CMAKE_SOURCE_DIR}/../FidelityFX-SDK")

# Check if FidelityFX SDK is available
if(EXISTS "${FFX_SDK_PATH}/sdk/CMakeLists.txt")
    set(FFX_API_BACKEND "VK_X64" CACHE STRING "FidelityFX API Backend")
    set(FFX_BUILD_AS_DLL OFF CACHE BOOL "Build FidelityFX as DLL")
    
    # Add FidelityFX SDK as subdirectory
    add_subdirectory(${FFX_SDK_PATH}/sdk ${CMAKE_BINARY_DIR}/ffx-sdk)
    set(FFX_SDK_AVAILABLE TRUE)
    message(STATUS "FidelityFX SDK found and integrated")
else()
    set(FFX_SDK_AVAILABLE FALSE)
    message(WARNING "FidelityFX SDK not found at ${FFX_SDK_PATH} - building with stubs only")
endif()

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
    # FidelityFX SDK include paths
    $<$<BOOL:${FFX_SDK_AVAILABLE}>:PRIVATE ${FFX_SDK_PATH}/sdk/include>
    )

target_link_libraries(lsfg-vk-afmf
    PRIVATE vulkan
    # FidelityFX SDK libraries
    $<$<BOOL:${FFX_SDK_AVAILABLE}>:PRIVATE ffx_frameinterpolation_x64>
    $<$<BOOL:${FFX_SDK_AVAILABLE}>:PRIVATE ffx_backend_vk_x64>
    )

# Add FidelityFX compile definitions when available
if(FFX_SDK_AVAILABLE)
    target_compile_definitions(lsfg-vk-afmf PRIVATE
        FFX_SDK_AVAILABLE=1
        FFX_VK=1
    )
endif()
target_compile_options(lsfg-vk-afmf PRIVATE
    -Weverything
    # disable compat c++ flags
    -Wno-pre-c++20-compat-pedantic
    -Wno-pre-c++17-compat
    -Wno-c++98-compat-pedantic
    -Wno-c++98-compat
    # disable other flags
    -Wno-missing-field-initializers  # Changed from -Wno-missing-designated-field-initializers
    -Wno-shadow         # allow shadowing
    -Wno-switch-enum    # ignore missing cases
    -Wno-switch-default # ignore missing default
    -Wno-padded         # ignore automatic padding
    -Wno-exit-time-destructors # allow globals
    -Wno-global-constructors   # allow globals
    # required for vulkan - use more compatible flag
    -Wno-cast-function-type
)

install(FILES "${CMAKE_BINARY_DIR}/liblsfg-vk-afmf.so" DESTINATION lib)
