file(GLOB_RECURSE XENGINE_INPUT_SOURCES CONFIGURE_DEPENDS
    Public/*.h
    Private/*.h
    Private/*.cpp
)

add_library(XEngineInput ${XENGINE_INPUT_SOURCES})

target_include_directories(XEngineInput
    PUBLIC
        Public
    PRIVATE
        Private
        ../Platform/Public
)

target_compile_features(XEngineInput PUBLIC cxx_std_20)
target_link_libraries(XEngineInput
    PUBLIC
        XEngineFoundation
        XEngineCoreRuntime
)
