# TestSupport
#
# Shared code for multiple test targets. This is the ONLY target other test
# targets may link against for cross-cutting helpers. Runtime modules must not
# depend on TestSupport.

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
        XEngineCoreRuntime
)