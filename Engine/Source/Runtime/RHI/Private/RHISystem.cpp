#include <XEngine/RHI/RHISystem.h>

#include <XEngine/Core/Assert.h>
#include <XEngine/Engine/Engine.h>
#include <XEngine/Engine/EngineConfig.h>
#include <XEngine/Engine/SubsystemManager.h>
#include <XEngine/Logging/Log.h>
#include <XEngine/Platform/PlatformSystem.h>
#include <XEngine/Platform/Window.h>
#include <XEngine/RHI/RHIDevice.h>

#if defined(XENGINE_ENABLE_VULKAN)
    #include "Vulkan/VulkanDevice.h"
#endif

#include <string>

namespace XEngine
{
    RHISystem::RHISystem() = default;

    RHISystem::~RHISystem()
    {
        OnDestroy();
    }

    void RHISystem::OnCreate(const SubsystemContext& context)
    {
        XENGINE_LOG_INFO("Creating RHI system");
        m_Engine = context.Engine;

        const GraphicsBackend backend = context.Config != nullptr ? context.Config->Backend : GraphicsBackend::None;

        if (backend == GraphicsBackend::None)
        {
            XENGINE_LOG_WARN("Graphics backend is None. No RHI device will be created.");
            return;
        }

        if (backend == GraphicsBackend::Vulkan)
        {
#if defined(XENGINE_ENABLE_VULKAN)
            XENGINE_ASSERT(m_Engine != nullptr, "RHISystem requires a valid Engine");

            PlatformSystem* platformSystem = m_Engine->GetSubsystemManager().GetSubsystem<PlatformSystem>();
            XENGINE_ASSERT(platformSystem != nullptr, "Vulkan backend requires PlatformSystem");
            if (platformSystem == nullptr)
            {
                XENGINE_LOG_ERROR("Vulkan backend requires PlatformSystem");
                return;
            }

            Window* mainWindow = platformSystem->GetMainWindow();
            XENGINE_ASSERT(mainWindow != nullptr, "Vulkan backend requires a main window");
            if (mainWindow == nullptr)
            {
                XENGINE_LOG_ERROR("Vulkan backend requires a main window");
                return;
            }

            auto vulkanDevice = std::make_unique<VulkanDevice>();

            VulkanDeviceCreateInfo createInfo;
            createInfo.NativeWindow = mainWindow->GetNativeHandle();
            createInfo.EnableValidation = context.Config != nullptr ? context.Config->EnableValidation : true;
            createInfo.EnableVSync = context.Config != nullptr ? context.Config->EnableVSync : true;

            if (!vulkanDevice->Initialize(createInfo))
            {
                XENGINE_LOG_ERROR("Failed to initialize Vulkan backend");
                return;
            }

            m_Device = std::move(vulkanDevice);
            m_Initialized = true;
#else
            XENGINE_LOG_ERROR("Vulkan backend requested but XENGINE_ENABLE_VULKAN is disabled.");
#endif
            return;
        }

        XENGINE_LOG_ERROR("Unsupported graphics backend requested.");
    }

    void RHISystem::OnDestroy()
    {
        if (!m_Initialized && !m_Device)
        {
            return;
        }

        XENGINE_LOG_INFO("Destroying RHI system");

        if (m_Device)
        {
            m_Device->WaitIdle();
            m_Device.reset();
        }

        m_Initialized = false;
        m_Engine = nullptr;
        m_PendingResize = false;
        m_PendingResizeWidth = 0;
        m_PendingResizeHeight = 0;
    }

    void RHISystem::OnUpdate(float deltaTime)
    {
        if (m_Engine == nullptr)
        {
            return;
        }

        PlatformSystem* platformSystem = m_Engine->GetSubsystemManager().GetSubsystem<PlatformSystem>();
        if (platformSystem == nullptr)
        {
            return;
        }

        for (const PlatformEvent& event : platformSystem->GetEvents())
        {
            if (event.Type == PlatformEventType::WindowResize)
            {
                m_PendingResize = true;
                m_PendingResizeWidth = event.Width;
                m_PendingResizeHeight = event.Height;

                std::string message = "Window resized to ";
                message += std::to_string(event.Width);
                message += "x";
                message += std::to_string(event.Height);
                message += ". Swapchain recreation is TODO for Stage 2B-2.";
                XENGINE_LOG_INFO(message);
            }
        }
    }

    RHIDevice* RHISystem::GetDevice()
    {
        return m_Device.get();
    }

    const RHIDevice* RHISystem::GetDevice() const
    {
        return m_Device.get();
    }
}
