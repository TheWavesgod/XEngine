file(GLOB_RECURSE XENGINE_SERIALIZATION_SOURCES CONFIGURE_DEPENDS
    Public/*.h
    Private/*.h
    Private/*.cpp
)

add_library(XEngineSerialization ${XENGINE_SERIALIZATION_SOURCES})

target_include_directories(XEngineSerialization
    PUBLIC
        Public
    PRIVATE
        Private
)

target_compile_features(XEngineSerialization PUBLIC cxx_std_20)
target_link_libraries(XEngineSerialization
    PUBLIC
        XEngineFoundation
        XEngineCoreRuntime
        ThirdParty_json
)
