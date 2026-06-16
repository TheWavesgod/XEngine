if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/spdlog/CMakeLists.txt")
    add_subdirectory(spdlog)
else()
    message(FATAL_ERROR "spdlog is required for Stage 0. Please place spdlog under ThirdParty/spdlog.")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/glm/CMakeLists.txt")
    add_subdirectory(glm)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/glm/glm/glm.hpp")
    add_library(glm INTERFACE)
    add_library(glm::glm ALIAS glm)
    target_include_directories(glm INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/glm")
else()
    message(FATAL_ERROR "glm is required for Stage 6A. Please place glm under ThirdParty/glm.")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/stb/stb_image.h")
    set(XENGINE_STB_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/stb" CACHE PATH "stb include directory" FORCE)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/stb_image/stb_image.h")
    set(XENGINE_STB_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/stb_image" CACHE PATH "stb_image include directory" FORCE)
else()
    message(FATAL_ERROR "stb_image is required for Stage 6B. Please place stb_image.h under ThirdParty/stb or ThirdParty/stb_image.")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/fastgltf/CMakeLists.txt")
    if(NOT TARGET simdjson::simdjson)
        add_library(simdjson::simdjson INTERFACE IMPORTED GLOBAL)
        set_target_properties(simdjson::simdjson PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/fastgltf/deps/simdjson"
        )
    endif()

    set(FASTGLTF_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
    set(FASTGLTF_ENABLE_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(FASTGLTF_ENABLE_DOCS OFF CACHE BOOL "" FORCE)
    set(FASTGLTF_ENABLE_GLTF_RS OFF CACHE BOOL "" FORCE)
    set(FASTGLTF_ENABLE_ASSIMP OFF CACHE BOOL "" FORCE)
    set(FASTGLTF_COMPILE_AS_CPP20 ON CACHE BOOL "" FORCE)
    add_subdirectory(fastgltf)
    target_include_directories(fastgltf PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/fastgltf/deps/simdjson")
    target_sources(fastgltf PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/fastgltf/deps/simdjson/simdjson.cpp")
else()
    message(FATAL_ERROR "fastgltf 0.9 is required for Stage 7E. Please place it under ThirdParty/fastgltf.")
endif()

if(XENGINE_ENABLE_SDL)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/SDL/CMakeLists.txt")
        set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
        set(SDL_INSTALL OFF CACHE BOOL "" FORCE)

        if(XENGINE_SDL_LINK_SHARED)
            set(SDL_SHARED ON CACHE BOOL "" FORCE)
            set(SDL_STATIC OFF CACHE BOOL "" FORCE)
        else()
            set(SDL_SHARED OFF CACHE BOOL "" FORCE)
            set(SDL_STATIC ON CACHE BOOL "" FORCE)
        endif()

        add_subdirectory(SDL)
    else()
        message(FATAL_ERROR "SDL3 is required when XENGINE_ENABLE_SDL=ON. Please place SDL3 source under ThirdParty/SDL.")
    endif()
endif()

if(XENGINE_ENABLE_VULKAN)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/volk/CMakeLists.txt")
        add_subdirectory(volk)
    else()
        message(FATAL_ERROR "volk is required when XENGINE_ENABLE_VULKAN=ON. Please place volk source under ThirdParty/volk.")
    endif()

    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/VulkanMemoryAllocator/include/vk_mem_alloc.h")
        message(FATAL_ERROR "VMA is required when XENGINE_ENABLE_VULKAN=ON. Please place VMA under ThirdParty/VulkanMemoryAllocator.")
    endif()
endif()

if(XENGINE_ENABLE_SHADER_COMPILER)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/slang/CMakeLists.txt")
        # The local Slang source snapshot may not contain all external submodules.
        # Stage 4A uses the private slangc fallback when no usable Slang CMake target is available.
        file(GLOB XENGINE_SLANGC_CANDIDATES
            "${CMAKE_CURRENT_SOURCE_DIR}/slang/bin/slangc.exe"
            "${CMAKE_CURRENT_SOURCE_DIR}/slang/bin/slangc"
            "${CMAKE_BINARY_DIR}/ThirdParty/slang/slang-*/bin/slangc.exe"
            "${CMAKE_BINARY_DIR}/ThirdParty/slang/slang-*/bin/slangc"
        )

        list(LENGTH XENGINE_SLANGC_CANDIDATES XENGINE_SLANGC_CANDIDATE_COUNT)
        if(XENGINE_SLANGC_CANDIDATE_COUNT GREATER 0)
            list(GET XENGINE_SLANGC_CANDIDATES 0 XENGINE_SLANGC_EXECUTABLE)
            set(XENGINE_SLANGC_EXECUTABLE "${XENGINE_SLANGC_EXECUTABLE}" CACHE FILEPATH "Path to slangc executable" FORCE)
        else()
            find_program(XENGINE_SLANGC_EXECUTABLE
                NAMES slangc slangc.exe
            )
        endif()

        if(XENGINE_SLANGC_EXECUTABLE)
            message(STATUS "Using slangc fallback: ${XENGINE_SLANGC_EXECUTABLE}")
        else()
            message(FATAL_ERROR "slangc is required for Stage 4A fallback. Please provide slangc under ThirdParty/slang/bin or on PATH.")
        endif()
    else()
        message(FATAL_ERROR "Slang is required when XENGINE_ENABLE_SHADER_COMPILER=ON. Please place Slang source under ThirdParty/slang.")
    endif()
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/json/include/nlohmann/json.hpp")
    add_library(ThirdParty_json INTERFACE)
    target_include_directories(ThirdParty_json INTERFACE
        "${CMAKE_CURRENT_SOURCE_DIR}/json/include"
    )
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/json/single_include/nlohmann/json.hpp")
    add_library(ThirdParty_json INTERFACE)
    target_include_directories(ThirdParty_json INTERFACE
        "${CMAKE_CURRENT_SOURCE_DIR}/json/single_include"
    )
else()
    message(FATAL_ERROR "nlohmann/json is required for Stage 8D. Please place json.hpp under ThirdParty/json/include or single_include.")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.cpp")
    # Dear ImGui is editor-only. Runtime modules and Sandbox must not link this target.
    add_library(ThirdParty_imgui STATIC
        imgui/imgui.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_widgets.cpp
        imgui/imgui_tables.cpp
        imgui/imgui_demo.cpp
        imgui/backends/imgui_impl_sdl3.cpp
        imgui/backends/imgui_impl_vulkan.cpp
        imgui/misc/cpp/imgui_stdlib.cpp
    )

    target_include_directories(ThirdParty_imgui PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui"
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends"
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui/misc/cpp"
    )

    target_compile_features(ThirdParty_imgui PUBLIC cxx_std_20)
    target_compile_definitions(ThirdParty_imgui PUBLIC
        VK_NO_PROTOTYPES
        IMGUI_IMPL_VULKAN_USE_VOLK
    )

    if(XENGINE_ENABLE_VULKAN)
        find_package(Vulkan REQUIRED)
        target_include_directories(ThirdParty_imgui PUBLIC ${Vulkan_INCLUDE_DIRS})
        if(TARGET volk::volk)
            target_link_libraries(ThirdParty_imgui PUBLIC volk::volk)
        elseif(TARGET volk)
            target_link_libraries(ThirdParty_imgui PUBLIC volk)
        endif()
    endif()

    if(XENGINE_ENABLE_SDL)
        if(XENGINE_SDL_LINK_SHARED)
            target_link_libraries(ThirdParty_imgui PUBLIC SDL3::SDL3-shared)
        else()
            target_link_libraries(ThirdParty_imgui PUBLIC SDL3::SDL3-static)
        endif()
    endif()
endif()
