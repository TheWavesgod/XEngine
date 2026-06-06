file(GLOB_RECURSE XENGINE_RHI_SOURCES CONFIGURE_DEPENDS
    Public/*.h
    Private/*.h
    Private/*.cpp
)

if(NOT XENGINE_ENABLE_VULKAN)
    list(FILTER XENGINE_RHI_SOURCES EXCLUDE REGEX "/Vulkan/")
endif()

add_library(XEngineRHI ${XENGINE_RHI_SOURCES})

target_include_directories(XEngineRHI
    PUBLIC
        Public
    PRIVATE
        Private
)

target_compile_features(XEngineRHI PUBLIC cxx_std_20)
target_link_libraries(XEngineRHI
    PUBLIC
        XEngineFoundation
        XEngineCoreRuntime
        XEnginePlatform
        XEngineShader
)

if(XENGINE_ENABLE_VULKAN)
    find_package(Vulkan REQUIRED)

    target_compile_definitions(XEngineRHI
        PRIVATE
            XENGINE_ENABLE_VULKAN
            VK_NO_PROTOTYPES
    )

    target_include_directories(XEngineRHI
        PRIVATE
            ${Vulkan_INCLUDE_DIRS}
            ${CMAKE_SOURCE_DIR}/ThirdParty/VulkanMemoryAllocator/include
            ${XENGINE_STB_INCLUDE_DIR}
    )

    if(TARGET volk::volk)
        target_link_libraries(XEngineRHI PRIVATE volk::volk)
    elseif(TARGET volk)
        target_link_libraries(XEngineRHI PRIVATE volk)
    else()
        message(FATAL_ERROR "volk target not found.")
    endif()
endif()
