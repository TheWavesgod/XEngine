function(xengine_copy_sdl_runtime target)
    if(XENGINE_ENABLE_SDL AND XENGINE_SDL_LINK_SHARED)
        if(WIN32)
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_FILE:SDL3::SDL3-shared>
                    $<TARGET_FILE_DIR:${target}>
            )
        endif()
    endif()
endfunction()

add_subdirectory(Sandbox)

if(XENGINE_ENABLE_EDITOR)
    add_subdirectory(EditorApp)
endif()
