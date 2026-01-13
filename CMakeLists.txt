cmake_minimum_required(VERSION 3.10)

project(DX12RayTracing)

set(CMAKE_CXX_STANDARD 17)

# Add source files
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h")

# Create executable
add_executable(DX12RayTracing WIN32 ${SOURCES})

# Link libraries
target_link_libraries(DX12RayTracing d3d12.lib dxgi.lib d3dcompiler.lib)

# Copy shaders to output directory
file(GLOB CONFIGURE_DEPENDS SHADERS "${CMAKE_SOURCE_DIR}/shaders/*.hlsl")
add_custom_command(TARGET DX12RayTracing POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:DX12RayTracing>/shaders"
)
foreach(SHADER ${SHADERS})
    get_filename_component(SHADER_NAME "${SHADER}" NAME)

    add_custom_command(
        TARGET DX12RayTracing POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${SHADER}"
                "$<TARGET_FILE_DIR:DX12RayTracing>/shaders/${SHADER_NAME}"
        VERBATIM
    )
endforeach()
