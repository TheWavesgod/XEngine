// RHIDevice — NVI wrapper implementations.

#include <XEngine/RHI/RHIDevice.h>
#include <XEngine/RHI/RHIValidation.h>
#include <XEngine/Logging/Log.h>

namespace XEngine
{
    // M4: buffer.
    RHIBuffer* RHIDevice::CreateBuffer(const RHIBufferDesc& desc)
    {
        if (auto r = ValidateBufferDesc(desc); !r)
        {
            XENGINE_LOG_ERROR(r.Message);
            return nullptr;
        }
        return CreateBufferImpl(desc);
    }

    // M5: texture.
    RHITexture* RHIDevice::CreateTexture(const RHITextureDesc& desc)
    {
        if (auto r = ValidateTextureDesc(desc); !r)
        {
            XENGINE_LOG_ERROR(r.Message);
            return nullptr;
        }
        return CreateTextureImpl(desc);
    }

    // M5: texture view.
    RHITextureView* RHIDevice::CreateTextureView(const RHITextureViewDesc& desc)
    {
        if (auto r = ValidateTextureViewDesc(desc); !r)
        {
            XENGINE_LOG_ERROR(r.Message);
            return nullptr;
        }
        return CreateTextureViewImpl(desc);
    }

    // M5: sampler.
    RHISampler* RHIDevice::CreateSampler(const RHISamplerDesc& desc)
    {
        if (auto r = ValidateSamplerDesc(desc); !r)
        {
            XENGINE_LOG_ERROR(r.Message);
            return nullptr;
        }
        return CreateSamplerImpl(desc);
    }

    // M6: fence.
    RHIFence* RHIDevice::CreateFence(const RHIFenceDesc& desc)
    {
        if (auto r = ValidateFenceDesc(desc); !r)
        {
            XENGINE_LOG_ERROR(r.Message);
            return nullptr;
        }
        return CreateFenceImpl(desc);
    }

    // M6: semaphore.
    RHISemaphore* RHIDevice::CreateSemaphore(const RHISemaphoreDesc& desc)
    {
        if (auto r = ValidateSemaphoreDesc(desc); !r)
        {
            XENGINE_LOG_ERROR(r.Message);
            return nullptr;
        }
        return CreateSemaphoreImpl(desc);
    }

    // M6: command list.
    RHICommandList* RHIDevice::CreateCommandList(const RHICommandListDesc& desc)
    {
        if (auto r = ValidateCommandListDesc(desc); !r)
        {
            XENGINE_LOG_ERROR(r.Message);
            return nullptr;
        }
        return CreateCommandListImpl(desc);
    }
}
