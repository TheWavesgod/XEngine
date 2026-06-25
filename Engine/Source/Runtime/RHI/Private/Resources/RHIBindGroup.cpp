#include <XEngine/RHI/Resources/RHIBindGroup.h>

namespace XEngine
{
    RHIBindGroupLayout::RHIBindGroupLayout(RHIDevice& ownerDevice)
        : RHIResource(ownerDevice)
    {
    }

    RHIBindGroup::RHIBindGroup(RHIDevice& ownerDevice)
        : RHIResource(ownerDevice)
    {
    }
}
