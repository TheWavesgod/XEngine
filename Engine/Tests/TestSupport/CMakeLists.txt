# TestSupport
#
# Shared code for multiple test targets. This is the ONLY target other test
# targets may link against for cross-cutting helpers. Runtime modules must not
# depend on TestSupport.
#
# Per Docs/AI helper/prompts.md the test tree may only depend on:
#   XEngineFoundation
#   XEngineRHI / XEngineVulkanRHI / XEngineTestSupport
#   GoogleTest
# It must NOT depend on XEngineCoreRuntime, Renderer, Editor, Asset, Scene,
# Input, Serialization, Shader, Platform, or SDL3 (except for the Smoke test,
# which may use a minimal Platform/Window dependency to create a window).

file(GLOB_RECURSE XENGINE_TESTSUPPORT_SOURCES CONFIGURE_DEPENDS
    Public/*.h
    Private/*.h
    Private/*.cpp
)

add_library(XEngineTestSupport ${XENGINE_TESTSUPPORT_SOURCES})

target_include_directories(XEngineTestSupport
    PUBLIC
        Public
    PRIVATE
        Private
)

target_compile_features(XEngineTestSupport PUBLIC cxx_std_20)
target_link_libraries(XEngineTestSupport
    PUBLIC
        XEngineFoundation
)
