set(PROJECT_NAME XEngine)

# file(GLOB ENGINE_PRIVATE 
#     "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
# )

file(GLOB ENGINE_PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

set(ALL_FILES

    ${ENGINE_PRIVATE}
    ${ENGINE_PUBLIC}
)

set_source_files_properties(${ALL_FILES} PROPERTIES LANGUAGE CXX)

###########################################################################
# Target
###########################################################################
add_executable(${PROJECT_NAME} main.cpp ${ALL_FILES})

###########################################################################
# Dependencies
###########################################################################
target_link_libraries(${PROJECT_NAME} PRIVATE Renderer VulkanImpl)