###########################################################################
#
###########################################################################

set(PROJECT_NAME VulkanImpl)

###########################################################################
# Source 
###########################################################################
file(GLOB VULKAN_IMPL_PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

file(GLOB VULKAN_IMPL_PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
)

set(ALL_FILES 
    ${VULKAN_IMPL_PRIVATE}
)

set_source_files_properties(${ALL_FILES} PROPERTIES LANGUAGE CXX)

###########################################################################
# Target
###########################################################################
add_library(${PROJECT_NAME} STATIC ${ALL_FILES})

target_precompile_headers(${PROJECT_NAME}
    PUBLIC
    ${VULKAN_IMPL_PUBLIC}
)

###########################################################################
# Dependencies
###########################################################################
find_library(VULKAN_LIB vulkan-1 ${CMAKE_CURRENT_SOURCE_DIR}/../../ThirdParty/Vulkan/Lib)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../ThirdParty/Vulkan/Include)