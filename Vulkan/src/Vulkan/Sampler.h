#pragma once

#include "VkBase.h"

namespace VK
{
    class Sampler 
    {
        VkSampler handle = VK_NULL_HANDLE;
    
    public:
        Sampler() = default;
        Sampler(VkSamplerCreateInfo& createInfo) 
        {
            Create(createInfo);
        }

        Sampler(Sampler&& other) noexcept { MoveHandle; }
        ~Sampler() { DestroyHandleBy(vkDestroySampler); }
        
        //Getter
        DefineHandleTypeOperator;
        DefineAddressFunction;
        
        //Non-const Function
        result_t Create(VkSamplerCreateInfo& createInfo) 
        {
            createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            VkResult result = vkCreateSampler(VkBase::Base().Device(), &createInfo, nullptr, &handle);
            if (result)
            {
                outStream << std::format("[ sampler ] ERROR\nFailed to create a sampler!\nError code: {}\n", int32_t(result));
            }
            return result;
        }
    };
}