# Smoke tests layer
#
# Smoke tests are minimal executables that verify the full GPU path is alive
# end-to-end:
#   Platform initialization
#   Window creation
#   Vulkan surface
#   Swapchain
#   Command recording
#   Queue submission
#   Present
#   Resize
#   Shutdown
#
# Smoke tests are NOT required to use GoogleTest. They can be registered as
# standalone executables in CTest.

add_subdirectory(VulkanRHI)