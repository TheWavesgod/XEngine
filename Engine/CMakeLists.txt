file(GLOB_RECURSE XENGINE_FOUNDATION_SOURCES CONFIGURE_DEPENDS
    Source/Foundation/*.h
    Source/Foundation/*.cpp
)

add_library(XEngineFoundation ${XENGINE_FOUNDATION_SOURCES})

target_include_directories(XEngineFoundation
    PUBLIC
        Source/Foundation/Core/Public
        Source/Foundation/Math/Public
        Source/Foundation/Logging/Public
        Source/Foundation/Diagnostics/Public
)

target_compile_features(XEngineFoundation PUBLIC cxx_std_20)

target_link_libraries(XEngineFoundation
    PRIVATE
        spdlog::spdlog
)

file(GLOB_RECURSE XENGINE_RUNTIME_SOURCES CONFIGURE_DEPENDS
    Source/Runtime/*.h
    Source/Runtime/*.cpp
)

set(XENGINE_EXCLUDED_RUNTIME_SOURCES)
foreach(sourceFile IN LISTS XENGINE_RUNTIME_SOURCES)
    file(TO_CMAKE_PATH "${sourceFile}" normalizedSourceFile)

    if(NOT XENGINE_ENABLE_VULKAN AND normalizedSourceFile MATCHES "/Vulkan/")
        list(APPEND XENGINE_EXCLUDED_RUNTIME_SOURCES "${sourceFile}")
    endif()

    if(NOT XENGINE_ENABLE_SDL AND normalizedSourceFile MATCHES "/SDL/")
        list(APPEND XENGINE_EXCLUDED_RUNTIME_SOURCES "${sourceFile}")
    endif()
endforeach()

if(XENGINE_EXCLUDED_RUNTIME_SOURCES)
    list(REMOVE_ITEM XENGINE_RUNTIME_SOURCES ${XENGINE_EXCLUDED_RUNTIME_SOURCES})
endif()

add_library(XEngineRuntime ${XENGINE_RUNTIME_SOURCES})

target_include_directories(XEngineRuntime
    PUBLIC
        Source/Runtime/Engine/Public
        Source/Runtime/Platform/Public
        Source/Runtime/FileSystem/Public
        Source/Runtime/Input/Public
        Source/Runtime/JobSystem/Public
        Source/Runtime/Asset/Public
        Source/Runtime/Shader/Public
        Source/Runtime/RHI/Public
        Source/Runtime/Renderer/Public
        Source/Runtime/Scene/Public
        Source/Runtime/UI/Public
        Source/Runtime/Serialization/Public
    PRIVATE
        Source/Runtime/Engine/Private
        Source/Runtime/Platform/Private
        Source/Runtime/FileSystem/Private
        Source/Runtime/Input/Private
        Source/Runtime/JobSystem/Private
        Source/Runtime/Asset/Private
        Source/Runtime/Shader/Private
        Source/Runtime/RHI/Private
        Source/Runtime/Renderer/Private
        Source/Runtime/Scene/Private
        Source/Runtime/UI/Private
        Source/Runtime/Serialization/Private
)

target_link_libraries(XEngineRuntime PUBLIC XEngineFoundation)
target_compile_features(XEngineRuntime PUBLIC cxx_std_20)

if(XENGINE_ENABLE_EDITOR)
    file(GLOB_RECURSE XENGINE_EDITOR_SOURCES CONFIGURE_DEPENDS
        Source/Editor/*.h
        Source/Editor/*.cpp
    )

    add_library(XEngineEditor ${XENGINE_EDITOR_SOURCES})

    target_include_directories(XEngineEditor
        PUBLIC
            Source/Editor/Public
        PRIVATE
            Source/Editor/Private
    )

    target_link_libraries(XEngineEditor PUBLIC XEngineRuntime)
    target_compile_features(XEngineEditor PUBLIC cxx_std_20)
endif()
