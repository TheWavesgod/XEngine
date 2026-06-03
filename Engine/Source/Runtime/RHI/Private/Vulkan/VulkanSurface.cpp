#include "VulkanSurface.h"

#include <XEngine/Core/Types.h>
#include <XEngine/Logging/Log.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <string>

namespace XEngine
{
    std::vector<const char*> VulkanSurface::GetRequiredInstanceExtensions()
    {
        u32 extensionCount = 0;
        const char* const* extensions = SDL_Vulkan_GetInstanceExtensions(&extensionCount);
        if (extensions == nullptr || extensionCount == 0)
        {
            XENGINE_LOG_ERROR("SDL did not provide Vulkan instance extensions");
            return {};
        }

        std::vector<const char*> result;
        result.reserve(extensionCount);
        for (u32 index = 0; index < extensionCount; ++index)
        {
            result.push_back(extensions[index]);
        }

        return result;
    }

    VulkanSurface::~VulkanSurface()
    {
        Destroy();
    }

    bool VulkanSurface::Create(VkInstance instance, NativeWindowHandle nativeWindow)
    {
        if (m_Surface != VK_NULL_HANDLE)
        {
            return true;
        }

        m_Instance = instance;
        SDL_Window* sdlWindow = static_cast<SDL_Window*>(nativeWindow.Window);
        if (m_Instance == VK_NULL_HANDLE || sdlWindow == nullptr)
        {
            XENGINE_LOG_ERROR("Cannot create Vulkan surface without a valid instance and SDL window");
            return false;
        }

        if (!SDL_Vulkan_CreateSurface(sdlWindow, m_Instance, nullptr, &m_Surface))
        {
            std::string message = "Failed to create Vulkan surface: ";
            message += SDL_GetError();
            XENGINE_LOG_ERROR(message);
            return false;
        }

        XENGINE_LOG_INFO("Vulkan surface created");
        return true;
    }

    void VulkanSurface::Destroy()
    {
        if (m_Surface == VK_NULL_HANDLE)
        {
            return;
        }

        XENGINE_LOG_INFO("Destroying Vulkan surface");
        vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
        m_Surface = VK_NULL_HANDLE;
        m_Instance = VK_NULL_HANDLE;
    }

    VkSurfaceKHR VulkanSurface::GetHandle() const
    {
        return m_Surface;
    }
}
