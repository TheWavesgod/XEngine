file(GLOB_RECURSE XENGINE_PLATFORM_SOURCES CONFIGURE_DEPENDS
    Public/*.h
    Private/*.h
    Private/*.cpp
)

if(NOT XENGINE_ENABLE_SDL)
    list(FILTER XENGINE_PLATFORM_SOURCES EXCLUDE REGEX "/SDL/")
endif()

add_library(XEnginePlatform ${XENGINE_PLATFORM_SOURCES})

target_include_directories(XEnginePlatform
    PUBLIC
        Public
        ../Input/Public
    PRIVATE
        Private
)

target_compile_features(XEnginePlatform PUBLIC cxx_std_20)
target_link_libraries(XEnginePlatform
    PUBLIC
        XEngineFoundation
        XEngineCoreRuntime
)

if(XENGINE_ENABLE_SDL)
    target_compile_definitions(XEnginePlatform PRIVATE XENGINE_ENABLE_SDL)

    if(XENGINE_SDL_LINK_SHARED)
        target_link_libraries(XEnginePlatform PRIVATE SDL3::SDL3-shared)
    else()
        target_link_libraries(XEnginePlatform PRIVATE SDL3::SDL3-static)
    endif()
endif()

function(xengine_copy_sdl_runtime target_name)
    if(XENGINE_ENABLE_SDL AND XENGINE_SDL_LINK_SHARED)
        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:SDL3::SDL3-shared>
                $<TARGET_FILE_DIR:${target_name}>
        )
    endif()
endfunction()
