###########################################################################
#
###########################################################################

set(PROJECT_NAME Renderer)

###########################################################################
# Source 
###########################################################################
file(GLOB RENDERER_PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

file(GLOB RENDERER_PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

set(ALL_FILES 
    ${RENDERER_PRIVATE}
    ${RENDERER_PUBLIC}
)

set_source_files_properties(${ALL_FILES} PROPERTIES LANGUAGE CXX)

###########################################################################
# Target
###########################################################################
add_library(${PROJECT_NAME} STATIC ${ALL_FILES})



###########################################################################
# Dependencies
###########################################################################
find_library(VULKAN_LIB vulkan-1 ${CMAKE_CURRENT_SOURCE_DIR}/../../ThirdParty/Vulkan/Lib)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../ThirdParty/Vulkan/Include)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../Vulkan)
target_link_libraries(${PROJECT_NAME} PUBLIC VulkanImpl)