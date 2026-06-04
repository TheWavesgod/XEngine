file(GLOB_RECURSE XENGINE_CORE_RUNTIME_SOURCES CONFIGURE_DEPENDS
    Public/*.h
    Private/*.h
    Private/*.cpp
)

add_library(XEngineCoreRuntime ${XENGINE_CORE_RUNTIME_SOURCES})

target_include_directories(XEngineCoreRuntime
    PUBLIC
        Public
    PRIVATE
        Private
        ../Platform/Public
        ../Shader/Public
        ../RHI/Public
        ../Renderer/Public
)

target_compile_features(XEngineCoreRuntime PUBLIC cxx_std_20)
target_link_libraries(XEngineCoreRuntime PUBLIC XEngineFoundation)
