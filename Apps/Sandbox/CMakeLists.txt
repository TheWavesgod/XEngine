add_executable(XEngineSandbox Source/main.cpp)
target_link_libraries(XEngineSandbox PRIVATE XEngineRuntime)
target_compile_features(XEngineSandbox PRIVATE cxx_std_20)

if(TARGET XEngineSandbox)
    xengine_copy_sdl_runtime(XEngineSandbox)
endif()
