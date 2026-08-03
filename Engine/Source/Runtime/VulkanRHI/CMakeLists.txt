# New XEngineVulkanRHI target — minimal skeleton only.
#
# Per Docs/AI helper/prompts.md, this target establishes a separate
# build identity for the Vulkan backend. It is intentionally minimal
# at this stage; the goal is to verify that the new XEngineVulkanRHI
# target:
#
#   * Compiles standalone.
#   * Links Vulkan SDK, volk, and VMA privately (i.e. none of them leak
#     into consumers).
#   * Exposes a clean public header surface (XEngine/VulkanRHI/Base.h)
#     that contains zero Vulkan symbols.
#   * Depends on XEngineRHI publicly so its (future) backend factory
#     can live behind a unified RHI namespace.
#
# Hard rules enforced by review:
#   - Vulkan SDK / volk / VMA includes and links MUST be PRIVATE here.
#   - No Renderer / Editor / Asset / Scene / Shader link.
#   - SDL3 may only be linked from the Smoke test target, never from
#     XEngineVulkanRHI itself.
#   - VK_NO_PROTOTYPES and other Vulkan-specific compile definitions
#     are PRIVATE here, not PUBLIC.

file(GLOB_RECURSE XENGINE_VULKAN_RHI_SOURCES
    CONFIGURE_DEPENDS
        Public/*.h
        Private/*.cpp
)

add_library(XEngineVulkanRHI ${XENGINE_VULKAN_RHI_SOURCES})

target_include_directories(XEngineVulkanRHI
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
)

target_compile_features(XEngineVulkanRHI PUBLIC cxx_std_20)

# The new XEngineVulkanRHI is currently a static library, so its export
# macro is a no-op. Define XENGINE_VULKAN_RHI_STATIC when building this
# target so the XENGINE_VULKAN_RHI_API tokens in Export.h resolve to
# empty.
target_compile_definitions(XEngineVulkanRHI
    PUBLIC
        XENGINE_VULKAN_RHI_STATIC
)

target_link_libraries(XEngineVulkanRHI
    PUBLIC
        XEngineRHI
    PRIVATE
        XEngineFoundation
)

# Optional Vulkan SDK + volk + VMA wiring. The new XEngineVulkanRHI
# target is gated by XENGINE_BUILD_VULKAN_RHI at the project level; if
# the project requests the Vulkan backend but the Vulkan SDK is
# unavailable we fail loudly during configure instead of producing an
# empty library that pretends to be a Vulkan backend.
if(XENGINE_BUILD_VULKAN_RHI)
    find_package(Vulkan REQUIRED)

    target_compile_definitions(XEngineVulkanRHI
        PRIVATE
            VK_NO_PROTOTYPES
    )

    target_include_directories(XEngineVulkanRHI
        PRIVATE
            ${Vulkan_INCLUDE_DIRS}
            ${CMAKE_SOURCE_DIR}/ThirdParty/VulkanMemoryAllocator/include
    )

    if(TARGET volk::volk)
        target_link_libraries(XEngineVulkanRHI PRIVATE volk::volk)
    elseif(TARGET volk)
        target_link_libraries(XEngineVulkanRHI PRIVATE volk)
    else()
        message(FATAL_ERROR
            "XEngineVulkanRHI requested (XENGINE_BUILD_VULKAN_RHI=ON) "
            "but the ThirdParty volk target was not created by "
            "ThirdParty/CMakeLists.txt.")
    endif()

    if(TARGET Vulkan::Vulkan)
        target_link_libraries(XEngineVulkanRHI PRIVATE Vulkan::Vulkan)
    else()
        target_link_libraries(XEngineVulkanRHI PRIVATE ${Vulkan_LIBRARIES})
    endif()
endif()
