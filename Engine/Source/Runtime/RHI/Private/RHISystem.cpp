#include <XEngine/RHI/RHISystem.h>

#include <XEngine/Engine/EngineConfig.h>
#include <XEngine/Logging/Log.h>
#include <XEngine/RHI/RHIDevice.h>

#if defined(XENGINE_ENABLE_VULKAN)
    #include "Vulkan/VulkanDevice.h"
#endif

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

        const GraphicsBackend backend = context.Config != nullptr ? context.Config->Backend : GraphicsBackend::None;

        if (backend == GraphicsBackend::None)
        {
            XENGINE_LOG_WARN("Graphics backend is None. No RHI device will be created.");
            return;
        }

        if (backend == GraphicsBackend::Vulkan)
        {
#if defined(XENGINE_ENABLE_VULKAN)
            m_Device = std::make_unique<VulkanDevice>();
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
    }

    void RHISystem::OnUpdate(float deltaTime)
    {
        // TODO(Stage 2B): Begin per-frame RHI work once Vulkan initialization exists.
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
