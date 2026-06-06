file(GLOB_RECURSE XENGINE_RENDERER_SOURCES CONFIGURE_DEPENDS
    Public/*.h
    Private/Passes/*.h
    Private/Passes/*.cpp
    Private/RenderGraph/*.h
    Private/RenderGraph/*.cpp
    Private/Mesh/*.h
    Private/Mesh/*.cpp
    Private/Materials/*.h
    Private/Materials/*.cpp
    Private/Resources/*.h
    Private/Resources/*.cpp
    Private/RenderSystem.cpp
)

add_library(XEngineRenderer ${XENGINE_RENDERER_SOURCES})

target_include_directories(XEngineRenderer
    PUBLIC
        Public
    PRIVATE
        Private
        ${XENGINE_STB_INCLUDE_DIR}
)

target_compile_features(XEngineRenderer PUBLIC cxx_std_20)
target_link_libraries(XEngineRenderer
    PUBLIC
        XEngineFoundation
        XEngineCoreRuntime
        XEngineShader
        XEngineRHI
)
