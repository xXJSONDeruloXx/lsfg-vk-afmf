cmake_minimum_required(VERSION 3.29)

# cmake options
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_CLANG_TIDY clang-tidy)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_SKIP_RPATH ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# FidelityFX SDK Integration
set(FFX_SDK_PATH "${CMAKE_SOURCE_DIR}/external/FidelityFX-SDK")

# Check if FidelityFX SDK submodule is available
if(EXISTS "${FFX_SDK_PATH}/PrebuiltSignedDLL")
    message(STATUS "FidelityFX SDK found at: ${FFX_SDK_PATH}")
    
    # Use prebuilt libraries instead of building from source
    # This avoids CMake generator compatibility issues and reduces build time
    set(FFX_SDK_AVAILABLE TRUE)
    
    # Set up paths for prebuilt libraries
    if(WIN32)
        # Windows: Use prebuilt DLLs
        set(FFX_VK_LIB_PATH "${FFX_SDK_PATH}/PrebuiltSignedDLL/amd_fidelityfx_vk.lib")
        set(FFX_VK_DLL_PATH "${FFX_SDK_PATH}/PrebuiltSignedDLL/amd_fidelityfx_vk.dll")
        
        # Alternative: newer FFX API version
        # set(FFX_VK_LIB_PATH "${FFX_SDK_PATH}/ffx-api/bin/amd_fidelityfx_vk.lib")
        # set(FFX_VK_DLL_PATH "${FFX_SDK_PATH}/ffx-api/bin/amd_fidelityfx_vk.dll")
    else()
        # Linux: We'll need to extract/convert Windows DLLs or build minimal components
        # For now, fall back to stub implementation on Linux
        message(WARNING "FidelityFX prebuilt libraries are Windows-only. Using stubs on Linux.")
        set(FFX_SDK_AVAILABLE FALSE)
    endif()
    
    message(STATUS "FidelityFX SDK using prebuilt libraries - no build required")
else()
    set(FFX_SDK_AVAILABLE FALSE)
    message(FATAL_ERROR "FidelityFX SDK submodule not found at ${FFX_SDK_PATH}. Run: git submodule update --init --recursive")
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

# Link FidelityFX libraries when available
if(FFX_SDK_AVAILABLE AND WIN32)
    # Add the prebuilt library
    target_link_libraries(lsfg-vk-afmf PRIVATE ${FFX_VK_LIB_PATH})
    
    # Copy DLL to output directory for runtime
    add_custom_command(TARGET lsfg-vk-afmf POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${FFX_VK_DLL_PATH}
        $<TARGET_FILE_DIR:lsfg-vk-afmf>
        COMMENT "Copying FidelityFX VK DLL to output directory"
    )
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
