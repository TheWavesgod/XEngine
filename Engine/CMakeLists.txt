add_subdirectory(Source/Foundation)
add_subdirectory(Source/Runtime/Engine)
add_subdirectory(Source/Runtime/Asset)
add_subdirectory(Source/Runtime/Platform)
add_subdirectory(Source/Runtime/Shader)
add_subdirectory(Source/Runtime/RHI)
add_subdirectory(Source/Runtime/Renderer)

add_library(XEngineRuntime INTERFACE)
target_link_libraries(XEngineRuntime
    INTERFACE
        XEngineFoundation
        XEngineCoreRuntime
        XEngineAsset
        XEnginePlatform
        XEngineShader
        XEngineRHI
        XEngineRenderer
)

if(XENGINE_ENABLE_EDITOR)
    file(GLOB_RECURSE XENGINE_EDITOR_SOURCES CONFIGURE_DEPENDS
        Source/Editor/*.h
        Source/Editor/*.cpp
    )

    if(XENGINE_EDITOR_SOURCES)
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
endif()
