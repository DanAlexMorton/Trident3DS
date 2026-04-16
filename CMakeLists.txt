cmake_minimum_required(VERSION 3.16)
project(Trident3DS VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Build options
option(TRIDENT_USE_DYNARMIC "Use Dynarmic for CPU emulation" ON)
option(TRIDENT_ENABLE_AI "Enable AI texture upscaling" OFF)

# Platform detection
if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(TRIDENT_IOS TRUE)
elseif(APPLE)
    set(TRIDENT_MACOS TRUE)
elseif(ANDROID)
    set(TRIDENT_ANDROID TRUE)
elseif(WIN32)
    set(TRIDENT_WINDOWS TRUE)
elseif(UNIX)
    set(TRIDENT_LINUX TRUE)
endif()

# Sources
set(CORE_SOURCES
    src/core/memory/memory.cpp
    src/core/cpu/cpu.cpp
    src/core/kernel/kernel.cpp
    src/core/gpu/gpu.cpp
    src/core/audio/audio.cpp
    src/core/loader/loader.cpp
    src/core/patcher/patcher.cpp
    src/core/services/service_manager.cpp
    src/core/ai/texture_upscaler.cpp
    src/core/emulator.cpp
)

set(CORE_HEADERS
    src/core/memory/memory.hpp
    src/core/cpu/cpu.hpp
    src/core/kernel/kernel.hpp
    src/core/gpu/gpu.hpp
    src/core/audio/audio.hpp
    src/core/loader/loader.hpp
    src/core/patcher/patcher.hpp
    src/core/services/service_manager.hpp
    src/core/ai/texture_upscaler.hpp
    src/core/emulator.hpp
)

set(FRONTEND_SOURCES
    src/frontend/libretro/libretro_core.cpp
)

# Core library (static, linked into the shared libretro core)
add_library(trident_core STATIC ${CORE_SOURCES} ${CORE_HEADERS})
target_include_directories(trident_core PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/core
)

# Dynarmic integration
if(TRIDENT_USE_DYNARMIC)
    if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/dynarmic/CMakeLists.txt")
        set(DYNARMIC_TESTS OFF CACHE BOOL "" FORCE)
        set(DYNARMIC_FRONTENDS "A32" CACHE STRING "" FORCE)
        # Dynarmic must be static — it doesn't export symbols for a DLL import lib
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
        add_subdirectory(third_party/dynarmic)
        target_link_libraries(trident_core PUBLIC dynarmic)
        target_compile_definitions(trident_core PUBLIC TRIDENT_USE_DYNARMIC)
    else()
        message(WARNING "Dynarmic not found at third_party/dynarmic. CPU will use stub implementation.")
    endif()
endif()

# Libretro shared library
add_library(trident_libretro SHARED ${FRONTEND_SOURCES})
target_link_libraries(trident_libretro PRIVATE trident_core)
target_include_directories(trident_libretro PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# Set output name (RetroArch expects corename_libretro.so/.dll/.dylib)
set_target_properties(trident_libretro PROPERTIES
    OUTPUT_NAME "trident_libretro"
    PREFIX ""
)

# Platform-specific output suffix
if(WIN32)
    set_target_properties(trident_libretro PROPERTIES SUFFIX ".dll")
elseif(APPLE)
    set_target_properties(trident_libretro PROPERTIES SUFFIX ".dylib")
else()
    set_target_properties(trident_libretro PROPERTIES SUFFIX ".so")
endif()

# Platform-specific settings
if(TRIDENT_IOS)
    target_compile_definitions(trident_core PRIVATE TRIDENT_PLATFORM_IOS)
    # Set entitlements for JIT
    set_target_properties(trident_libretro PROPERTIES
        XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${CMAKE_SOURCE_DIR}/ios/Trident.entitlements"
    )
elseif(TRIDENT_ANDROID)
    target_compile_definitions(trident_core PRIVATE TRIDENT_PLATFORM_ANDROID)
    target_link_libraries(trident_libretro PRIVATE log GLESv3 EGL)
elseif(TRIDENT_WINDOWS)
    target_compile_definitions(trident_core PRIVATE TRIDENT_PLATFORM_WINDOWS)
endif()

# Compiler warnings
if(MSVC)
    target_compile_options(trident_core PRIVATE /W4)
    target_compile_options(trident_libretro PRIVATE /W4)
else()
    target_compile_options(trident_core PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(trident_libretro PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Install
install(TARGETS trident_libretro
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

message(STATUS "Trident3DS v${PROJECT_VERSION}")
message(STATUS "  Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Dynarmic: ${TRIDENT_USE_DYNARMIC}")
message(STATUS "  AI Upscaling: ${TRIDENT_ENABLE_AI}")
