file(GLOB_RECURSE XENGINE_ASSET_SOURCES CONFIGURE_DEPENDS
    Public/*.h
    Private/*.h
    Private/*.cpp
)

add_library(XEngineAsset ${XENGINE_ASSET_SOURCES})

target_include_directories(XEngineAsset
    PUBLIC
        Public
    PRIVATE
        Private
        ${XENGINE_STB_INCLUDE_DIR}
)

target_compile_features(XEngineAsset PUBLIC cxx_std_20)
target_link_libraries(XEngineAsset
    PUBLIC
        XEngineFoundation
        XEngineCoreRuntime
)

if(TARGET fastgltf::fastgltf)
    target_link_libraries(XEngineAsset PRIVATE fastgltf::fastgltf)
elseif(TARGET fastgltf)
    target_link_libraries(XEngineAsset PRIVATE fastgltf)
else()
    message(FATAL_ERROR "fastgltf target not found. Please check ThirdParty/fastgltf integration.")
endif()
