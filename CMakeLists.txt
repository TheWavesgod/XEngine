cmake_minimum_required(VERSION 3.25)

project(XEngine LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# -----------------------------------------------------------------------------
# Module-level build options.
#
# Per Docs/AI helper/prompts.md the new RHI rebuild takes a "推倒式"
# (tear-down) approach: the rebuilt RHI and VulkanRHI land as their own
# independent targets; the rest of the engine (Renderer, Editor, Asset,
# Scene, Engine runtime, Shader compiler, Sandbox, EditorApp) is
# temporarily frozen and is gated off by default.
#
# The defaults below therefore disable the legacy engine stack and
# enable only what the new RHI refactor needs:
#   - XEngineBuildRHI        : the new XEngineRHI target
#   - XEngineBuildVulkanRHI  : the new XEngineVulkanRHI target
#   - XEngineBuildTests      : Unit / Integration / Smoke test tree
#
# Other options are kept (XENGINE_ENABLE_VULKAN, XENGINE_ENABLE_SDL,
# XENGINE_ENABLE_SHADER_COMPILER) so that the ThirdParty / SDL /
# Vulkan wiring still works for whatever the active targets need; the
# legacy engine-side consumers of those flags are simply not built by
# default. They can be re-enabled module-by-module later.
# -----------------------------------------------------------------------------

option(XENGINE_BUILD_RHI            "Build the new XEngineRHI target"             ON)
option(XENGINE_BUILD_RHI_LOADER     "Build the new XEngineRHILoader target"        ON)
option(XENGINE_BUILD_VULKAN_RHI     "Build the new XEngineVulkanRHI target"       ON)
option(XENGINE_BUILD_D3D12_RHI      "Build the new XEngineD3D12RHI target (Windows only)" ON)
option(XENGINE_BUILD_TESTS          "Build the RHI / VulkanRHI / D3D12RHI test tree"     ON)
option(XENGINE_ENABLE_ASAN          "Enable AddressSanitizer for supported test targets" OFF)

# MSVC's ASan-enabled STL changes its object annotations, so every C++ target
# in the link graph must be compiled consistently (including GoogleTest and
# static engine libraries). The debug-asan preset removes /RTC1, which is not
# compatible with AddressSanitizer.
if(MSVC AND XENGINE_ENABLE_ASAN)
    add_compile_options(
        $<$<COMPILE_LANGUAGE:CXX>:/fsanitize=address>
        $<$<COMPILE_LANGUAGE:CXX>:/Zi>
        $<$<COMPILE_LANGUAGE:CXX>:/FS>
    )
    add_link_options(/INCREMENTAL:NO)
endif()

option(XENGINE_BUILD_RENDERER       "Build the Renderer module (frozen for now)"  OFF)
option(XENGINE_BUILD_EDITOR         "Build the Editor module (frozen for now)"    OFF)
option(XENGINE_BUILD_APPS           "Build the Sandbox / EditorApp executables"  OFF)

# Legacy / preserved flags (still drive ThirdParty + SDL / Vulkan / Slang
# wiring but do not by themselves bring legacy engine modules online).
option(XENGINE_ENABLE_VULKAN         "Enable Vulkan SDK / volk / VMA in ThirdParty" ON)
option(XENGINE_ENABLE_SDL            "Enable SDL3 in ThirdParty / Platform"        ON)
option(XENGINE_SDL_LINK_SHARED       "Link SDL3 as a shared library"               ON)
option(XENGINE_ENABLE_SHADER_COMPILER "Enable Slang shader compiler in ThirdParty" ON)

# Enable CTest at the project root so `ctest --test-dir build` discovers
# tests registered by the new test tree.
include(CTest)
enable_testing()

add_subdirectory(ThirdParty)

if(XENGINE_BUILD_RHI OR XENGINE_BUILD_VULKAN_RHI OR XENGINE_BUILD_D3D12_RHI OR XENGINE_BUILD_TESTS
   OR XENGINE_BUILD_RENDERER OR XENGINE_BUILD_EDITOR OR XENGINE_BUILD_APPS)
    add_subdirectory(Engine)
endif()

if(XENGINE_BUILD_APPS AND EXISTS "${CMAKE_SOURCE_DIR}/Apps/CMakeLists.txt")
    add_subdirectory(Apps)
endif()
