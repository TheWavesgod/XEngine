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
            createInfo.Width = mainWindow->GetWidth();
            createInfo.Height = mainWindow->GetHeight();
            createInfo.EnableValidation = context.Config != nullptr ? context.Config->EnableValidation : true;
            createInfo.EnableVSync = context.Config != nullptr ? context.Config->EnableVSync : true;

            if (!vulkanDevice->Initialize(createInfo))
            {
                XENGINE_LOG_ERROR("Failed to initialize Vulkan backend");
                return;
            }

            m_Device = std::move(vulkanDevice);
            CreateDefaultTextureValidationResources();
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
            m_DefaultLinearRepeatSampler.reset();
            m_DefaultNormalTexture.reset();
            m_DefaultWhiteTexture.reset();
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
                XENGINE_LOG_INFO(message);

                if (m_Device)
                {
                    m_Device->RequestResize(event.Width, event.Height);
                }
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

    void RHISystem::CreateDefaultTextureValidationResources()
    {
        if (!m_Device || !m_Device->IsValid())
        {
            return;
        }

        RHITextureDesc textureDesc;
        textureDesc.Width = 1;
        textureDesc.Height = 1;
        textureDesc.MipLevels = 1;
        textureDesc.ArrayLayers = 1;
        textureDesc.Format = RHIFormat::RGBA8Unorm;
        textureDesc.Dimension = RHITextureDimension::Texture2D;
        textureDesc.Usage = RHITextureUsageFlags::Sampled | RHITextureUsageFlags::TransferDst;
        textureDesc.DebugName = "DefaultWhiteTexture";

        const u8 whitePixel[] = { 255, 255, 255, 255 };
        m_DefaultWhiteTexture = m_Device->CreateTexture(textureDesc, whitePixel, sizeof(whitePixel));
        if (!m_DefaultWhiteTexture)
        {
            XENGINE_LOG_ERROR("Failed to create DefaultWhiteTexture validation resource");
            return;
        }

        textureDesc.DebugName = "DefaultNormalTexture";
        const u8 normalPixel[] = { 128, 128, 255, 255 };
        m_DefaultNormalTexture = m_Device->CreateTexture(textureDesc, normalPixel, sizeof(normalPixel));
        if (!m_DefaultNormalTexture)
        {
            XENGINE_LOG_ERROR("Failed to create DefaultNormalTexture validation resource");
            return;
        }

        RHISamplerDesc samplerDesc;
        samplerDesc.MinFilter = RHIFilter::Linear;
        samplerDesc.MagFilter = RHIFilter::Linear;
        samplerDesc.AddressU = RHIAddressMode::Repeat;
        samplerDesc.AddressV = RHIAddressMode::Repeat;
        samplerDesc.AddressW = RHIAddressMode::Repeat;
        samplerDesc.DebugName = "DefaultLinearRepeatSampler";

        m_DefaultLinearRepeatSampler = m_Device->CreateSampler(samplerDesc);
        if (!m_DefaultLinearRepeatSampler)
        {
            XENGINE_LOG_ERROR("Failed to create DefaultLinearRepeatSampler validation resource");
            return;
        }

        XENGINE_LOG_INFO("Created Stage 6A default white/normal textures and linear repeat sampler");
    }
}
