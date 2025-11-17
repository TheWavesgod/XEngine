#pragma once

#include "Memory.h"

namespace VK
{
    class DescriptorSetLayout
    {
        VkDescriptorSetLayout handle = VK_NULL_HANDLE;

    public:
        DescriptorSetLayout() = default;
        DescriptorSetLayout(VkDescriptorSetLayout layout) : handle(layout) {}

        DescriptorSetLayout(DescriptorSetLayout&& other) noexcept { MoveHandle; }
        ~DescriptorSetLayout();

        //Getter
        DefineHandleTypeOperator;
        DefineAddressFunction;

    public:
        class Builder
        {
        public:
            explicit Builder(VkDevice device) : device_(device) {}

            Builder& SetFlags(VkDescriptorSetLayoutCreateFlags flags);

            Builder& AddBinding(
                uint32_t binding,
                VkDescriptorType descriptorType,
                VkShaderStageFlags stageFlags,
                uint32_t descriptorCount = 1,
                const VkSampler* pImmutableSamplers = nullptr);

            Builder& AddUniformBuffer(
                uint32_t binding,
                VkShaderStageFlags stageFlags,
                uint32_t descriptorCount = 1);

            Builder& AddUniformBufferDynamic(
                uint32_t binding,
                VkShaderStageFlags stageFlags,
                uint32_t descriptorCount = 1);

            Builder& AddCombinedImageSampler(
                uint32_t binding,
                VkShaderStageFlags stageFlags,
                uint32_t descriptorCount = 1,
                const VkSampler* pImmutableSamplers = nullptr);

            Builder& AddStorageBuffer(
                uint32_t binding,
                VkShaderStageFlags stageFlags,
                uint32_t descriptorCount = 1);

            DescriptorSetLayout Build();

        private:
            VkDevice device_ = VK_NULL_HANDLE;
            VkDescriptorSetLayoutCreateFlags flags_ = 0;
            std::vector<VkDescriptorSetLayoutBinding> bindings_;
        };
    };

    class DescriptorPool 
    {
        VkDescriptorPool handle = VK_NULL_HANDLE;
        VkDevice device = VK_NULL_HANDLE;

    public:
        DescriptorPool() = default;
        DescriptorPool(VkDescriptorPool pool, VkDevice device) : handle(pool), device(device) {}
       
        DescriptorPool(DescriptorPool&& other) noexcept { MoveHandle; }
        ~DescriptorPool();

        DescriptorPool(const DescriptorPool&) = delete;
        DescriptorPool& operator=(const DescriptorPool&) = delete;

        // Getter
        DefineHandleTypeOperator;
        DefineAddressFunction;

        void Reset() const;

    public:
        class Builder
        {
        public:
            explicit Builder(VkDevice device) : device_(device) {}

            Builder& SetFlags(VkDescriptorPoolCreateFlags flags);
            Builder& SetMaxSets(uint32_t maxSets);
            Builder& AddPoolSize(VkDescriptorType type, uint32_t count);

            DescriptorPool Build();

        private:
            VkDevice device_ = VK_NULL_HANDLE;

            VkDescriptorPoolCreateFlags flags_ = 0;
            uint32_t maxSets_ = 0;
            std::vector<VkDescriptorPoolSize> poolSizes_;
        };
    };

    class DescriptorSet
    {
        VkDescriptorSet handle = VK_NULL_HANDLE;

    public:
        DescriptorSet() = default;
        DescriptorSet(VkDescriptorSet set) : handle(set) {}

        DescriptorSet(DescriptorSet&& other) noexcept { MoveHandle; }

        //Getter
        DefineHandleTypeOperator;
        DefineAddressFunction;

        static DescriptorSet Allocate(VkDevice device, const VkDescriptorPool& pool,VkDescriptorSetLayout layout);

        DescriptorSet& WriteBuffer(
            uint32_t binding,
            VkDescriptorType type,
            VkBuffer buffer,
            VkDeviceSize offset,
            VkDeviceSize range,
            uint32_t arrayElement = 0);

        DescriptorSet& WriteBuffer(
            uint32_t binding,
            VkDescriptorType type,
            const VkDescriptorBufferInfo& info,
            uint32_t arrayElement = 0);

        DescriptorSet& WriteImage(
            uint32_t binding,
            VkDescriptorType type,
            VkImageView imageView,
            VkSampler sampler,
            VkImageLayout layout,
            uint32_t arrayElement = 0);

        DescriptorSet& WriteImage(
            uint32_t binding,
            VkDescriptorType type,
            const VkDescriptorImageInfo& info,
            uint32_t arrayElement = 0);

        DescriptorSet& WriteImageArray(
            uint32_t binding,
            VkDescriptorType type,
            const std::vector<VkDescriptorImageInfo>& infos,
            uint32_t firstArrayElement = 0);

        DescriptorSet& WriteCombinedImageSampler(
            uint32_t binding,
            VkImageView imageView,
            VkSampler sampler,
            VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            uint32_t arrayElement = 0);

		DescriptorSet& WriteUniformBuffer(
			uint32_t binding,
			VkBuffer buffer,
			VkDeviceSize offset,
			VkDeviceSize range,
			uint32_t arrayElement = 0);

        DescriptorSet& WriteStorageBuffer(
            uint32_t binding,
            VkBuffer buffer,
            VkDeviceSize offset,
            VkDeviceSize range,
            uint32_t arrayElement = 0);

        void Update(VkDevice device);

    private:
        struct BufferWrite {
            uint32_t binding;
            uint32_t arrayElement;
            VkDescriptorType type;
            VkDescriptorBufferInfo info;
        };

        struct ImageWrite {
            uint32_t binding;
            uint32_t arrayElement;
            VkDescriptorType type;
            VkDescriptorImageInfo info;
        };

        std::vector<BufferWrite> bufferWrites;
        std::vector<ImageWrite>  imageWrites;
    };
}
