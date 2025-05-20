#include "Sampler.h"

namespace VK
{
    Sampler::~Sampler()
    {
        DestroyHandleBy(vkDestroySampler);
    }

    result_t Sampler::Create(VkSamplerCreateInfo& createInfo)
    {
        createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        VkResult result = vkCreateSampler(VkBase::Base().Device(), &createInfo, nullptr, &handle);
        if (result)
        {
            outStream << std::format("[ sampler ] ERROR\nFailed to create a sampler!\nError code: {}\n", int32_t(result));
        }
        return result;
    }
}