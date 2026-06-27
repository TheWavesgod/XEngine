#include "XEngine/RHI/RHIResourceFactory.h"

#include "XEngine/RHI/RHIDevice.h"
#include "XEngine/Core/Assert.h"
#include "XEngine/Logging/Log.h"

namespace XEngine
{
    RHIResourceFactory::RHIResourceFactory(RHIDevice& ownerDevice)
        : m_Device(&ownerDevice)
    {
    }

    RHIDevice& RHIResourceFactory::GetDevice() const
    {
        XENGINE_ASSERT(m_Device != nullptr, "RHIResourceFactory doesn't have valid RHIDevice handle!");
        return *m_Device;
    }

    // ---- Buffer ----

    std::shared_ptr<RHIBuffer> RHIResourceFactory::CreateBuffer(
        const RHIBufferDesc& desc,
        const void* initialData,
        std::size_t initialDataSize)
    {
        if (desc.Size == 0)
        {
            XENGINE_LOG_ERROR("Cannot create RHI buffer with zero size");
            return nullptr;
        }
        RHIBufferDesc normalized = desc;
        if (normalized.DebugName == nullptr)
        {
            normalized.DebugName = "RHIBuffer";
        }
        return CreateBufferImpl(normalized, initialData, initialDataSize);
    }

    // ---- Texture ----

    std::shared_ptr<RHITexture> RHIResourceFactory::CreateTexture(
        const RHITextureDesc& desc,
        const void* initialData,
        std::size_t initialDataSize)
    {
        if (desc.Width == 0 || desc.Height == 0)
        {
            XENGINE_LOG_ERROR("Cannot create RHI texture with zero extent");
            return nullptr;
        }
        if (desc.Dimension == RHITextureDimension::TextureCube && desc.ArrayLayers != 6)
        {
            XENGINE_LOG_ERROR("TextureCube requires exactly 6 array layers");
            return nullptr;
        }
        RHITextureDesc normalized = desc;
        if (normalized.GenerateMips)
        {
            XENGINE_LOG_WARN("Texture mip generation not implemented; forcing MipLevels = 1");
            normalized.MipLevels = 1;
            normalized.GenerateMips = false;
        }
        if (normalized.DebugName == nullptr)
        {
            normalized.DebugName = "RHITexture";
        }
        return CreateTextureImpl(normalized, initialData, initialDataSize);
    }

    // ---- TextureView ----

    std::shared_ptr<RHITextureView> RHIResourceFactory::CreateTextureView(
        const RHITextureViewDesc& desc)
    {
        if (desc.Texture == nullptr)
        {
            XENGINE_LOG_ERROR("Cannot create RHI texture view with null texture");
            return nullptr;
        }
        const RHITextureDesc& texDesc = desc.Texture->GetDesc();

        RHITextureViewDesc normalized = desc;
        if (normalized.MipCount == 0)
        {
            normalized.MipCount = texDesc.MipLevels - normalized.BaseMipLevel;
        }
        if (normalized.ArrayLayerCount == 0)
        {
            normalized.ArrayLayerCount = texDesc.ArrayLayers - normalized.BaseArrayLayer;
        }
        if (normalized.BaseMipLevel + normalized.MipCount > texDesc.MipLevels)
        {
            XENGINE_LOG_ERROR("RHI texture view mip range exceeds source texture");
            return nullptr;
        }
        if (normalized.BaseArrayLayer + normalized.ArrayLayerCount > texDesc.ArrayLayers)
        {
            XENGINE_LOG_ERROR("RHI texture view array layer range exceeds source texture");
            return nullptr;
        }
        return CreateTextureViewImpl(normalized);
    }

    // ---- Sampler ----

    std::shared_ptr<RHISampler> RHIResourceFactory::CreateSampler(const RHISamplerDesc& desc)
    {
        return CreateSamplerImpl(desc);
    }

    // ---- Shader ----

    std::shared_ptr<RHIShader> RHIResourceFactory::CreateShader(const RHIShaderDesc& desc)
    {
        if (desc.Code == nullptr || desc.CodeSize == 0 || desc.EntryPoint.empty())
        {
            XENGINE_LOG_ERROR("RHI shader desc requires non-null code and non-empty entry point");
            return nullptr;
        }
        return CreateShaderImpl(desc);
    }

    // ---- BindGroupLayout ----

    std::shared_ptr<RHIBindGroupLayout> RHIResourceFactory::CreateBindGroupLayout(
        const RHIBindGroupLayoutDesc& desc)
    {
        if (desc.Entries.empty())
        {
            XENGINE_LOG_ERROR("RHI bind group layout desc has no entries");
            return nullptr;
        }
        return CreateBindGroupLayoutImpl(desc);
    }

    // ---- BindGroup ----

    std::shared_ptr<RHIBindGroup> RHIResourceFactory::CreateBindGroup(
        const RHIBindGroupDesc& desc)
    {
        if (desc.Layout == nullptr)
        {
            XENGINE_LOG_ERROR("RHI bind group desc requires a non-null layout");
            return nullptr;
        }
        return CreateBindGroupImpl(desc);
    }

    // ---- GraphicsPipeline ----

    std::shared_ptr<RHIPipeline> RHIResourceFactory::CreateGraphicsPipeline(
        const RHIGraphicsPipelineDesc& desc)
    {
        if (desc.VertexShader == nullptr || desc.FragmentShader == nullptr)
        {
            XENGINE_LOG_ERROR("RHI graphics pipeline requires both vertex and fragment shaders");
            return nullptr;
        }
        if (desc.ColorFormat == RHIFormat::Undefined)
        {
            XENGINE_LOG_ERROR("RHI graphics pipeline requires a color format (Stage 6 will lift this)");
            return nullptr;
        }
        return CreateGraphicsPipelineImpl(desc);
    }

}