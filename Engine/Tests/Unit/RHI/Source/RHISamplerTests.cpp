// Unit tests for RHISampler abstract interface.
//
// M5 surface: GetDesc() returns the cached RHISamplerDesc. Tests verify
// the audit 3.1 contract — all 7 fields (AddressMode U/V/W, Mag/Min filter,
// LodBias, MaxAnisotropy, CompareEnable, CompareOp, MinLod, MaxLod,
// BorderColor) are tracked.

#include <gtest/gtest.h>

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/Base.h>
#include <XEngine/RHI/RHIObject.h>
#include <XEngine/RHI/RHISampler.h>
#include <XEngine/RHI/RHIDevice.h>
#include <XEngine/RHI/RHIInstance.h>
#include <XEngine/RHI/RHIDescriptors.h>
#include <XEngine/RHI/RHIEnums.h>

#include "RHITestStubs.h"

#include <memory>
#include <vector>

namespace XEngine
{
    // Stub sampler — captures desc at creation, returns it via GetDesc()
    // (audit 3.1). Local because it carries test-specific stored desc state.
    class StubSampler : public RHISampler
    {
    public:
        StubSampler(RHIDevice& owner, RHISamplerDesc desc)
            : RHISampler(owner, owner.GetBackend())
            , m_Desc(desc)
        {
        }

        RHISamplerDesc GetDesc() const noexcept override { return m_Desc; }

    private:
        RHISamplerDesc m_Desc;
    };

    // SamplerTestDevice — derives from the shared Test::StubDevice and
    // only overrides the sampler factory hook.
    class SamplerTestDevice final : public Test::StubDevice
    {
    public:
        explicit SamplerTestDevice(RHIInstance& owner)
            : Test::StubDevice(owner)
        {
        }

        RHISampler* CreateSamplerImpl(const RHISamplerDesc& desc) override
        {
            m_LastSampler = std::make_unique<StubSampler>(*this, desc);
            return m_LastSampler.get();
        }

        StubSampler* GetLastSampler() noexcept { return m_LastSampler.get(); }

    private:
        std::unique_ptr<StubSampler> m_LastSampler;
    };
}

namespace
{
    using namespace XEngine;
    using StubInstance = Test::StubInstance;

    static_assert(std::is_polymorphic_v<RHISampler>,
                  "RHISampler must be polymorphic");

    TEST(RHISampler, GetDescReturnsConstructedDesc)
    {
        StubInstance instance;
        SamplerTestDevice device(instance);

        RHISamplerDesc desc{
            .AddressModeU = RHIAddressMode::ClampToBorder,
            .AddressModeV = RHIAddressMode::ClampToBorder,
            .AddressModeW = RHIAddressMode::ClampToEdge,
            .MagFilter = RHIFilterMode::Linear,
            .MinFilter = RHIFilterMode::Linear,
            .MipFilter = RHIFilterMode::Linear,
            .LodBias = 0.5f,
            .MaxAnisotropy = 16,
            .CompareEnable = true,
            .CompareOp = RHICompareOp::LessEqual,
            .MinLod = 0.0f,
            .MaxLod = 4.0f,
            .BorderColor = RHIBorderColor::White,
        };

        RHISampler* sampler = device.CreateSampler(desc);
        ASSERT_NE(sampler, nullptr);

        RHISamplerDesc got = sampler->GetDesc();
        EXPECT_EQ(got.AddressModeU, RHIAddressMode::ClampToBorder);
        EXPECT_EQ(got.AddressModeV, RHIAddressMode::ClampToBorder);
        EXPECT_EQ(got.AddressModeW, RHIAddressMode::ClampToEdge);
        EXPECT_EQ(got.MagFilter, RHIFilterMode::Linear);
        EXPECT_EQ(got.MinFilter, RHIFilterMode::Linear);
        EXPECT_EQ(got.MipFilter, RHIFilterMode::Linear);
        EXPECT_FLOAT_EQ(got.LodBias, 0.5f);
        EXPECT_EQ(got.MaxAnisotropy, 16u);
        EXPECT_TRUE(got.CompareEnable);
        EXPECT_EQ(got.CompareOp, RHICompareOp::LessEqual);
        EXPECT_FLOAT_EQ(got.MinLod, 0.0f);
        EXPECT_FLOAT_EQ(got.MaxLod, 4.0f);
        EXPECT_EQ(got.BorderColor, RHIBorderColor::White);
    }

    TEST(RHISampler, DefaultDescMatchesRHISamplerDescDefaults)
    {
        StubInstance instance;
        SamplerTestDevice device(instance);

        RHISampler* sampler = device.CreateSampler(RHISamplerDesc{});
        ASSERT_NE(sampler, nullptr);

        RHISamplerDesc got = sampler->GetDesc();
        EXPECT_EQ(got.AddressModeU, RHIAddressMode::Repeat);
        EXPECT_EQ(got.MagFilter, RHIFilterMode::Nearest);
        EXPECT_EQ(got.CompareEnable, false);
        EXPECT_EQ(got.BorderColor, RHIBorderColor::Black);
    }

    TEST(RHISampler, OwnerDeviceAndBackend)
    {
        StubInstance instance;
        SamplerTestDevice device(instance);

        RHISampler* sampler = device.CreateSampler({});
        ASSERT_NE(sampler, nullptr);

        EXPECT_EQ(sampler->GetOwnerDevice(), &device);
        EXPECT_EQ(sampler->GetBackend(), RHIBackend::Vulkan);
    }

    TEST(RHISampler, IsPolymorphicThroughBasePointer)
    {
        StubInstance instance;
        SamplerTestDevice device(instance);

        RHISampler* sampler = device.CreateSampler({});
        ASSERT_NE(sampler, nullptr);

        RHISampler* base = sampler;
        EXPECT_EQ(base->GetDesc().AddressModeU, RHIAddressMode::Repeat);
    }
}
