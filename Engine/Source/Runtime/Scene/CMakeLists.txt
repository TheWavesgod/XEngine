file(GLOB_RECURSE XENGINE_SCENE_SOURCES CONFIGURE_DEPENDS
    Public/*.h
    Private/*.h
    Private/*.cpp
)

add_library(XEngineScene ${XENGINE_SCENE_SOURCES})

target_include_directories(XEngineScene
    PUBLIC
        Public
    PRIVATE
        Private
)

target_compile_features(XEngineScene PUBLIC cxx_std_20)
target_link_libraries(XEngineScene
    PUBLIC
        XEngineFoundation
        XEngineCoreRuntime
        XEngineAsset
        XEngineInput
)
