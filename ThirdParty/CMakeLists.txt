if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/spdlog/CMakeLists.txt")
    add_subdirectory(spdlog)
else()
    message(FATAL_ERROR "spdlog is required for Stage 0. Please place spdlog under ThirdParty/spdlog.")
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
