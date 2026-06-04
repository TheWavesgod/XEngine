file(GLOB_RECURSE XENGINE_SHADER_SOURCES CONFIGURE_DEPENDS
    Public/*.h
    Private/*.h
    Private/*.cpp
)

add_library(XEngineShader ${XENGINE_SHADER_SOURCES})

target_include_directories(XEngineShader
    PUBLIC
        Public
    PRIVATE
        Private
)

target_compile_features(XEngineShader PUBLIC cxx_std_20)
target_link_libraries(XEngineShader
    PUBLIC
        XEngineFoundation
        XEngineCoreRuntime
)

target_compile_definitions(XEngineShader
    PRIVATE
        XENGINE_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

if(XENGINE_ENABLE_SHADER_COMPILER)
    target_compile_definitions(XEngineShader PRIVATE XENGINE_ENABLE_SHADER_COMPILER)
    target_include_directories(XEngineShader PRIVATE ${CMAKE_SOURCE_DIR}/ThirdParty/slang/include)

    if(XENGINE_SLANGC_EXECUTABLE)
        target_compile_definitions(XEngineShader
            PRIVATE
                XENGINE_SLANGC_EXECUTABLE="${XENGINE_SLANGC_EXECUTABLE}"
        )
    endif()

    if(TARGET slang)
        target_link_libraries(XEngineShader PRIVATE slang)
    elseif(TARGET slang::slang)
        target_link_libraries(XEngineShader PRIVATE slang::slang)
    elseif(TARGET Slang::slang)
        target_link_libraries(XEngineShader PRIVATE Slang::slang)
    else()
        message(STATUS "No Slang library target found. Using private slangc fallback for Stage 5.")
    endif()
endif()
