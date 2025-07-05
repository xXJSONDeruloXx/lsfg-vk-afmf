cmake_minimum_required(VERSION 3.29)

# cmake options
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_CLANG_TIDY clang-tidy)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_SKIP_RPATH ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# FidelityFX Integration Strategy:
# Instead of building the entire SDK (which has generator compatibility issues),
# we extract only the frame interpolation components we need and build them directly.
# This gives us cross-platform support and better build control.

set(FFX_SDK_PATH "${CMAKE_SOURCE_DIR}/external/FidelityFX-SDK")

# Check if we have access to FidelityFX source code
if(EXISTS "${FFX_SDK_PATH}/sdk/src/components/frameinterpolation")
    message(STATUS "FidelityFX SDK source found at: ${FFX_SDK_PATH}")
    set(FFX_SDK_AVAILABLE TRUE)
    
    # We'll build minimal FidelityFX components ourselves
    message(STATUS "Building minimal FidelityFX frame interpolation components from source")
    
else()
    set(FFX_SDK_AVAILABLE FALSE)
    message(WARNING "FidelityFX SDK submodule not found at ${FFX_SDK_PATH}")
    message(STATUS "Building with frame interpolation stubs - run: git submodule update --init --recursive")
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
)

# Build minimal FidelityFX components when SDK is available
if(FFX_SDK_AVAILABLE)
    # Create a minimal FidelityFX library with just what we need
    set(FFX_SOURCES
        # Core FidelityFX files
        "${FFX_SDK_PATH}/sdk/src/shared/ffx_assert.cpp"
        "${FFX_SDK_PATH}/sdk/src/shared/ffx_object_management.cpp"
        
        # Frame interpolation component
        "${FFX_SDK_PATH}/sdk/src/components/frameinterpolation/ffx_frameinterpolation.cpp"
        
        # Optical flow component (needed for frame interpolation)
        "${FFX_SDK_PATH}/sdk/src/components/opticalflow/ffx_opticalflow.cpp"
        
        # Vulkan backend
        "${FFX_SDK_PATH}/sdk/src/backends/vk/ffx_vk.cpp"
    )
    
    # Create static library for FidelityFX components
    add_library(ffx_minimal STATIC ${FFX_SOURCES})
    
    # Set up include directories for FidelityFX
    target_include_directories(ffx_minimal
        PUBLIC 
            "${FFX_SDK_PATH}/sdk/include"
        PRIVATE
            "${FFX_SDK_PATH}/sdk/src/backends/vk"
            "${FFX_SDK_PATH}/sdk/src/shared"
    )
    
    # Link Vulkan to the FidelityFX library
    target_link_libraries(ffx_minimal PUBLIC vulkan)
    
    # Set compile definitions for FidelityFX
    target_compile_definitions(ffx_minimal PRIVATE
        FFX_VK=1
        FFX_GPU_COMPONENT_FRAMEINTERPOLATION=1
        FFX_GPU_COMPONENT_OPTICALFLOW=1
    )
    
    # Link our minimal FidelityFX library to the main target
    target_link_libraries(lsfg-vk-afmf PRIVATE ffx_minimal)
    
    message(STATUS "FidelityFX minimal components library created")
endif()

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
