cmake_minimum_required(VERSION 3.25)

project(XEngine LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(XENGINE_ENABLE_VULKAN "Enable Vulkan backend" ON)
option(XENGINE_ENABLE_SDL "Enable SDL platform backend" ON)
option(XENGINE_SDL_LINK_SHARED "Link SDL as a shared library" ON)
option(XENGINE_ENABLE_SHADER_COMPILER "Enable runtime shader compilation" ON)
option(XENGINE_ENABLE_EDITOR "Build XEngine editor" ON)
option(XENGINE_ENABLE_TRACY "Enable Tracy profiler integration" OFF)

# Enable CTest at the project root so `ctest --test-dir build` discovers tests
# registered by Engine/Tests subdirectories. Engine/Tests/CMakeLists.txt also
# calls include(CTest) and enable_testing() defensively in case this is ever
# built as a standalone project tree.
include(CTest)
enable_testing()

add_subdirectory(ThirdParty)
add_subdirectory(Engine)
add_subdirectory(Apps)
