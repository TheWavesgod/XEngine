#include <XEngine/RHI/Resources/RHITexture.h>
#include <XEngine/RHI/Resources/RHITextureView.h>

namespace XEngine
{
    RHITexture::RHITexture(RHIDevice& ownerDevice)
        : RHIResource(ownerDevice)
    {
    }

    void* RHITexture::GetNativeDefaultView(RHIBackend backend) const
    {
        RHITextureView* view = GetDefaultView();
        return view != nullptr ? view->GetNativeView(backend) : nullptr;
    }
}
