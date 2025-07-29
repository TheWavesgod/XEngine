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
        ~Sampler();
        
        //Getter
        DefineHandleTypeOperator;
        DefineAddressFunction;
        
        //Non-const Function
        result_t Create(VkSamplerCreateInfo& createInfo);
    };
}