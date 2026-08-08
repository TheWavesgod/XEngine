# New XEngineRHILoader target — backend factory registry implementation.
#
# Per Docs/AI helper/plan.md (M2 multi-backend runtime selection), this
# target implements XEngine::RHIRuntime declared in
# XEngineRHI/Public/XEngine/RHI/RHIRuntime.h.
#
# It is intentionally separated from XEngineRHI so that:
#   1. XEngineRHI does not depend on any backend (plan §4 preserved).
#   2. Apps that don't want multi-backend selection can still link
#      XEngineRHI directly without pulling in the registry code.
#
# Hard rules enforced by review:
#   - Depends only on XEngineRHI (which already only depends on
#     XEngineFoundation). No Vulkan / volk / VMA / SDL3 / backend headers.
#   - No public symbols other than those declared in XEngineRHI public
#     headers (the registry is private to this target).
#   - Thread-safe by design (the registry will be touched from App startup
#     and possibly from parallel test fixtures).

file(GLOB_RECURSE XENGINE_RHI_LOADER_SOURCES
    CONFIGURE_DEPENDS
        Private/*.cpp
)

add_library(XEngineRHILoader ${XENGINE_RHI_LOADER_SOURCES})

target_include_directories(XEngineRHILoader
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
)

target_compile_features(XEngineRHILoader PUBLIC cxx_std_20)

target_link_libraries(XEngineRHILoader
    PUBLIC
        XEngineRHI
        XEngineFoundation
)