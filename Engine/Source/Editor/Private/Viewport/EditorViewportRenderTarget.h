#pragma once

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/RHITypes.h>
#include <XEngine/RHI/Resources/RHISampler.h>
#include <XEngine/RHI/Resources/RHITexture.h>

#include <memory>

namespace XEngine
{
    class RHIDevice;

    class EditorViewportRenderTarget
    {
    public:
        bool Resize(RHIDevice& device, u32 width, u32 height);
        void Reset();

        RHIRenderOutputDesc BuildRenderOutput() const;

        RHITexture* GetColorTexture() const { return m_ColorTexture.get(); }
        RHISampler* GetSampler() const { return m_Sampler.get(); }
        u32 GetWidth() const { return m_Width; }
        u32 GetHeight() const { return m_Height; }
        bool IsValid() const { return m_ColorTexture != nullptr && m_DepthTexture != nullptr; }

    private:
        std::shared_ptr<RHITexture> m_ColorTexture;
        std::shared_ptr<RHITexture> m_DepthTexture;
        std::shared_ptr<RHISampler> m_Sampler;
        u32 m_Width = 0;
        u32 m_Height = 0;
    };
}
