#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/RHITypes.h>

#include <vector>

namespace XEngine
{
    class RHIBuffer;
    class RHITexture;
    class RHISampler;

    struct RHIBindGroupLayoutEntry
    {
        u32 Binding = 0;
        RHIBindingType Type = RHIBindingType::Unknown;
        RHIShaderStageFlags Visibility = RHIShaderStageFlags::Fragment;
        u32 Count = 1;
    };

    struct RHIBindGroupLayoutDesc
    {
        std::vector<RHIBindGroupLayoutEntry> Entries;
        const char* DebugName = nullptr;
    };

    class RHIBindGroupLayout
    {
    public:
        virtual ~RHIBindGroupLayout() = default;

        virtual const RHIBindGroupLayoutDesc& GetDesc() const = 0;
    };

    struct RHIBindingResource
    {
        u32 Binding = 0;
        RHIBindingType Type = RHIBindingType::Unknown;

        RHITexture* Texture = nullptr;
        RHISampler* Sampler = nullptr;
        RHIBuffer* Buffer = nullptr;
    };

    struct RHIBindGroupDesc
    {
        RHIBindGroupLayout* Layout = nullptr;
        std::vector<RHIBindingResource> Resources;
        const char* DebugName = nullptr;
    };

    class RHIBindGroup
    {
    public:
        virtual ~RHIBindGroup() = default;

        virtual const RHIBindGroupDesc& GetDesc() const = 0;
    };
}
