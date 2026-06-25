#include "XEngine/RHI/Resources/RHITextureView.h"

#include "XEngine/RHI/Resources/RHITexture.h"

namespace XEngine
{
    RHITextureView::RHITextureView(RHIDevice& ownerDevice)
        : RHIResource(ownerDevice)
    {
    }

    const RHITexture* RHITextureView::GetTexture() const
    {
        return GetDesc().Texture;
    }
}