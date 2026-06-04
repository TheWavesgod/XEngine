file(GLOB_RECURSE XENGINE_FOUNDATION_SOURCES CONFIGURE_DEPENDS
    Core/*.h
    Core/*.cpp
    Diagnostics/*.h
    Diagnostics/*.cpp
    Logging/*.h
    Logging/*.cpp
    Math/*.h
    Math/*.cpp
)

add_library(XEngineFoundation ${XENGINE_FOUNDATION_SOURCES})

target_include_directories(XEngineFoundation
    PUBLIC
        Core/Public
        Diagnostics/Public
        Logging/Public
        Math/Public
    PRIVATE
        Core/Private
        Diagnostics/Private
        Logging/Private
        Math/Private
)

target_compile_features(XEngineFoundation PUBLIC cxx_std_20)
target_link_libraries(XEngineFoundation PRIVATE spdlog::spdlog)
