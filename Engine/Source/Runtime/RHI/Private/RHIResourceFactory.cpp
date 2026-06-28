#include "XEngine/RHI/RHIResourceFactory.h"

#include "XEngine/RHI/RHIDevice.h"
#include "XEngine/RHI/RHIUtils.h"
#include "XEngine/Core/Assert.h"
#include "XEngine/Logging/Log.h"

namespace XEngine
{
    namespace
    {
        const RHIBindGroupLayoutEntry* FindLayoutEntry(
            const RHIBindGroupLayoutDesc& layout,
            u32 binding)
        {
            for (const RHIBindGroupLayoutEntry& entry : layout.Entries)
            {
                if (entry.Binding == binding)
                {
                    return &entry;
                }
            }
            return nullptr;
        }
    }

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
        const RHIBufferDesc& desc)
    {
        if (desc.Size == 0 || desc.Usage == RHIBufferUsage::None)
        {
            XENGINE_LOG_ERROR("Cannot create RHI buffer with zero size or no usage flags");
            return nullptr;
        }
        RHIBufferDesc normalized = desc;
        if (normalized.DebugName == nullptr)
        {
            normalized.DebugName = "RHIBuffer";
        }
        return CreateBufferImpl(normalized);
    }

    // ---- Texture ----

    std::shared_ptr<RHITexture> RHIResourceFactory::CreateTexture(
        const RHITextureDesc& desc)
    {
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
        if (!ValidateTextureDesc(normalized, GetDevice().GetCapabilities()))
        {
            XENGINE_LOG_ERROR("Invalid RHI texture descriptor");
            return nullptr;
        }
        return CreateTextureImpl(normalized);
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
        if (&desc.Texture->GetOwnerDevice() != &GetDevice())
        {
            XENGINE_LOG_ERROR("RHI texture view source belongs to another device");
            return nullptr;
        }
        const RHITextureDesc& texDesc = desc.Texture->GetDesc();

        RHITextureViewDesc normalized;
        if (!NormalizeTextureViewDesc(texDesc, desc, normalized) ||
            !ValidateTextureViewDesc(texDesc, normalized, GetDevice().GetCapabilities()))
        {
            XENGINE_LOG_ERROR("Invalid RHI texture view descriptor");
            return nullptr;
        }
        if (normalized.DebugName == nullptr)
        {
            normalized.DebugName = "RHITextureView";
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
        for (std::size_t i = 0; i < desc.Entries.size(); ++i)
        {
            const RHIBindGroupLayoutEntry& entry = desc.Entries[i];
            if (entry.Type == RHIBindingType::Unknown || entry.Count != 1 ||
                entry.Visibility == RHIShaderStageFlags::None)
            {
                XENGINE_LOG_ERROR("Invalid or unsupported RHI bind group layout entry");
                return nullptr;
            }
            for (std::size_t j = i + 1; j < desc.Entries.size(); ++j)
            {
                if (entry.Binding == desc.Entries[j].Binding)
                {
                    XENGINE_LOG_ERROR("Duplicate RHI bind group layout binding");
                    return nullptr;
                }
            }
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
        if (&desc.Layout->GetOwnerDevice() != &GetDevice())
        {
            XENGINE_LOG_ERROR("RHI bind group layout belongs to another device");
            return nullptr;
        }
        const RHIBindGroupLayoutDesc& layoutDesc = desc.Layout->GetDesc();
        if (desc.Resources.size() != layoutDesc.Entries.size())
        {
            XENGINE_LOG_ERROR("RHI bind group resources do not match its layout");
            return nullptr;
        }
        for (std::size_t i = 0; i < desc.Resources.size(); ++i)
        {
            const RHIBindingResource& resource = desc.Resources[i];
            const RHIBindGroupLayoutEntry* entry = FindLayoutEntry(layoutDesc, resource.Binding);
            if (entry == nullptr || entry->Type != resource.Type)
            {
                XENGINE_LOG_ERROR("RHI bind group binding does not match its layout");
                return nullptr;
            }
            for (std::size_t j = i + 1; j < desc.Resources.size(); ++j)
            {
                if (resource.Binding == desc.Resources[j].Binding)
                {
                    XENGINE_LOG_ERROR("Duplicate RHI bind group resource binding");
                    return nullptr;
                }
            }
            if ((resource.Type == RHIBindingType::CombinedImageSampler &&
                 (resource.TextureView == nullptr || resource.Sampler == nullptr)) ||
                (resource.Type == RHIBindingType::SampledTexture &&
                 resource.TextureView == nullptr) ||
                (resource.Type == RHIBindingType::Sampler &&
                 resource.Sampler == nullptr) ||
                ((resource.Type == RHIBindingType::UniformBuffer ||
                  resource.Type == RHIBindingType::StorageBuffer) &&
                 resource.Buffer == nullptr))
            {
                XENGINE_LOG_ERROR("RHI bind group resource is incomplete");
                return nullptr;
            }
            if ((resource.TextureView != nullptr &&
                 &resource.TextureView->GetOwnerDevice() != &GetDevice()) ||
                (resource.Sampler != nullptr &&
                 &resource.Sampler->GetOwnerDevice() != &GetDevice()) ||
                (resource.Buffer != nullptr &&
                 &resource.Buffer->GetOwnerDevice() != &GetDevice()))
            {
                XENGINE_LOG_ERROR("RHI bind group resource belongs to another device");
                return nullptr;
            }
            if (resource.Buffer != nullptr)
            {
                const u64 bufferSize = static_cast<u64>(resource.Buffer->GetSize());
                if (resource.BufferOffset > bufferSize)
                {
                    XENGINE_LOG_ERROR("RHI bind group buffer offset is invalid");
                    return nullptr;
                }
                const u64 rangeSize = resource.BufferSize == 0
                    ? bufferSize - resource.BufferOffset
                    : resource.BufferSize;
                if (rangeSize == 0 || rangeSize > bufferSize - resource.BufferOffset)
                {
                    XENGINE_LOG_ERROR("RHI bind group buffer range is invalid");
                    return nullptr;
                }
            }
        }
        return CreateBindGroupImpl(desc);
    }

    // ---- GraphicsPipeline ----

    std::shared_ptr<RHIPipeline> RHIResourceFactory::CreateGraphicsPipeline(
        const RHIGraphicsPipelineDesc& desc)
    {
        if (desc.VertexShader == nullptr)
        {
            XENGINE_LOG_ERROR("RHI graphics pipeline requires vertex shaders");
            return nullptr;
        }
        if (&desc.VertexShader->GetOwnerDevice() != &GetDevice() ||
            (desc.FragmentShader != nullptr &&
             &desc.FragmentShader->GetOwnerDevice() != &GetDevice()))
        {
            XENGINE_LOG_ERROR("RHI graphics pipeline shader belongs to another device");
            return nullptr;
        }

        if (desc.HasColorAttachment)
        {
            if (desc.FragmentShader == nullptr ||
                desc.ColorFormat == RHIFormat::Undefined)
            {
                XENGINE_LOG_ERROR("RHI graphics pipeline with color attachment requires a color format and fragment shaders");
                return nullptr;
            }
        }

        if (!desc.HasColorAttachment && desc.DepthFormat == RHIFormat::Undefined)
        {
            XENGINE_LOG_ERROR("With no color and no depth, this stage has no usable attachment.");
            return nullptr;
        }

        if ((desc.EnableDepthTest || desc.EnableDepthWrite) &&
            desc.DepthFormat == RHIFormat::Undefined)
        {
            return nullptr;
        }
        const RHICapabilities& caps = GetDevice().GetCapabilities();
        if ((caps.MaxPushConstantSize > 0 &&
             desc.PushConstantSize > caps.MaxPushConstantSize) ||
            (caps.MaxBoundDescriptorSets > 0 &&
             desc.BindGroupLayouts.size() > caps.MaxBoundDescriptorSets))
        {
            XENGINE_LOG_ERROR("RHI graphics pipeline exceeds device capabilities");
            return nullptr;
        }
        for (RHIBindGroupLayout* layout : desc.BindGroupLayouts)
        {
            if (layout == nullptr || &layout->GetOwnerDevice() != &GetDevice())
            {
                XENGINE_LOG_ERROR("RHI graphics pipeline has an invalid bind group layout");
                return nullptr;
            }
        }
        return CreateGraphicsPipelineImpl(desc);
    }

}
