file(GLOB_RECURSE XENGINE_FOUNDATION_SOURCES CONFIGURE_DEPENDS
    Core/*.h
    Core/*.cpp
    Diagnostics/*.h
    Diagnostics/*.cpp
    Logging/*.h
    Logging/*.cpp
    Math/*.h
    Math/*.cpp
)

set(XENGINE_GENERATED_INCLUDE_DIR "${CMAKE_BINARY_DIR}/Generated")
set(XENGINE_DEV_ENGINE_ROOT "${CMAKE_SOURCE_DIR}/Engine")
set(XENGINE_DEV_PROJECT_ROOT "${CMAKE_SOURCE_DIR}")
configure_file(
    Core/Public/XEngine/Core/Generated/XEngineBuildPaths.h.in
    "${XENGINE_GENERATED_INCLUDE_DIR}/XEngine/Core/Generated/XEngineBuildPaths.h"
    @ONLY
)

add_library(XEngineFoundation ${XENGINE_FOUNDATION_SOURCES})

target_include_directories(XEngineFoundation
    PUBLIC
        Core/Public
        "${XENGINE_GENERATED_INCLUDE_DIR}"
        Diagnostics/Public
        Logging/Public
        Math/Public
    PRIVATE
        Core/Private
        Diagnostics/Private
        Logging/Private
        Math/Private
)

target_compile_features(XEngineFoundation PUBLIC cxx_std_20)
target_link_libraries(XEngineFoundation
    PUBLIC
        glm::glm
    PRIVATE
        spdlog::spdlog
)
