cmake_minimum_required(VERSION 3.25)

project(XEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(XENGINE_ENABLE_VULKAN "Enable Vulkan backend" OFF)
option(XENGINE_ENABLE_SDL "Enable SDL platform backend" ON)
option(XENGINE_SDL_LINK_SHARED "Link SDL as a shared library" ON)
option(XENGINE_ENABLE_EDITOR "Build XEngine editor" ON)
option(XENGINE_ENABLE_TRACY "Enable Tracy profiler integration" OFF)

add_subdirectory(ThirdParty)
add_subdirectory(Engine)
add_subdirectory(Apps)
