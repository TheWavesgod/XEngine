#pragma once

#include "Memory.h"

namespace VK
{
    class DescriptorSetLayout
    {
        VkDescriptorSetLayout handle = VK_NULL_HANDLE;

    public:
        DescriptorSetLayout() = default;
        DescriptorSetLayout(VkDescriptorSetLayoutCreateInfo& createInfo) { Create(createInfo); }

        DescriptorSetLayout(DescriptorSetLayout&& other) noexcept { MoveHandle; }
        ~DescriptorSetLayout();

        //Getter
        DefineHandleTypeOperator;
        DefineAddressFunction;

        //Non-const Function
        result_t Create(VkDescriptorSetLayoutCreateInfo& createInfo);
    };

    class DescriptorSet 
    {
        VkDescriptorSet handle = VK_NULL_HANDLE;
        
        friend class DescriptorPool;

    public:
        DescriptorSet() = default;
        DescriptorSet(DescriptorSet&& other) noexcept { MoveHandle; }

        //Getter
        DefineHandleTypeOperator;
        DefineAddressFunction;

        //Const Function
        void Write(arrayRef<const VkDescriptorImageInfo> descriptorInfos, VkDescriptorType descriptorType, uint32_t dstBinding = 0, uint32_t dstArrayElement = 0) const;        
        void Write(arrayRef<const VkDescriptorBufferInfo> descriptorInfos, VkDescriptorType descriptorType, uint32_t dstBinding = 0, uint32_t dstArrayElement = 0) const;
        void Write(arrayRef<const VkBufferView> descriptorInfos, VkDescriptorType descriptorType, uint32_t dstBinding = 0, uint32_t dstArrayElement = 0) const;
        void Write(arrayRef<const BufferView> descriptorInfos, VkDescriptorType descriptorType, uint32_t dstBinding = 0, uint32_t dstArrayElement = 0) const;

        //Static Function
        static void Update(arrayRef<VkWriteDescriptorSet> writes, arrayRef<VkCopyDescriptorSet> copies = {});
    };

    class DescriptorPool 
    {
        VkDescriptorPool handle = VK_NULL_HANDLE;

    public:
        DescriptorPool() = default;
        DescriptorPool(VkDescriptorPoolCreateInfo& createInfo) { Create(createInfo); }

        DescriptorPool(uint32_t maxSetCount, arrayRef<const VkDescriptorPoolSize> poolSizes, VkDescriptorPoolCreateFlags flags = 0) { Create(maxSetCount, poolSizes, flags); }
       
        DescriptorPool(DescriptorPool&& other) noexcept { MoveHandle; }
        ~DescriptorPool();

        // Getter
        DefineHandleTypeOperator;
        DefineAddressFunction;
        
        // Const Function
        result_t AllocateSets(arrayRef<VkDescriptorSet> sets, arrayRef<const VkDescriptorSetLayout> setLayouts) const;
        result_t AllocateSets(arrayRef<VkDescriptorSet> sets, arrayRef<const DescriptorSetLayout> setLayouts) const;
        result_t AllocateSets(arrayRef<DescriptorSet> sets, arrayRef<const VkDescriptorSetLayout> setLayouts) const;
        result_t AllocateSets(arrayRef<DescriptorSet> sets, arrayRef<const DescriptorSetLayout> setLayouts) const;

        result_t FreeSets(arrayRef<VkDescriptorSet> sets) const;
        result_t FreeSets(arrayRef<DescriptorSet> sets) const;

        //Non-const Function
        result_t Create(VkDescriptorPoolCreateInfo& createInfo);
        result_t Create(uint32_t maxSetCount, arrayRef<const VkDescriptorPoolSize> poolSizes, VkDescriptorPoolCreateFlags flags = 0);
    };
}
