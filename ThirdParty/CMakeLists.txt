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

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/fastgltf")
    message(WARNING "fastgltf 0.9 is expected for Stage 7E. Please place it under ThirdParty/fastgltf.")
elseif(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/fastgltf/include")
    message(WARNING "ThirdParty/fastgltf exists but its include directory was not found. Stage 7E GltfImporter integration will need attention.")
else()
    message(STATUS "fastgltf source detected for future Stage 7E importer integration.")
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
