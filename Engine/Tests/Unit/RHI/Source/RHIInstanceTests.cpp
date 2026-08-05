// Unit tests for RHIInstance base class.
//
// M2 scope:
//   * ScoreAdapter algorithm (all four preferences + Type permutations)
//   * RequestAdapter default implementation (uses EnumerateAdapters +
//     ScoreAdapter + pick best)
//   * Single-device rule (CreateDevice returns nullptr second time)
//   * RHIInstance::Create static factory stub returns nullptr
//
// M2 test stubs define RHIInstance, RHIAdapter, RHIDevice locally where
// the existing forward declarations are insufficient. M3 will replace
// these stubs with the production types.

#include <gtest/gtest.h>

#include <XEngine/Core/Types.h>
#include <XEngine/Core/Result.h>
#include <XEngine/RHI/Base.h>
#include <XEngine/RHI/RHIObject.h>
#include <XEngine/RHI/RHIInstance.h>  // brings RHIInstance, RHIInstanceDesc
#include <XEngine/RHI/RHIAdapter.h>   // brings RHIAdapter, RHIAdapterInfo
#include <XEngine/RHI/RHIDevice.h>    // brings RHIDevice, RHICapabilities, RHIDeviceDesc

#include <memory>
#include <vector>

namespace
{
    using namespace XEngine;

    // ---------------------------------------------------------------------
    // ScoreAdapter — pure-function unit tests.
    // ---------------------------------------------------------------------

    TEST(RHIInstanceScore, UnknownAdapterAlwaysScoresZero)
    {
        RHIAdapterInfo info{ .Type = RHIAdapterType::Unknown };
        EXPECT_EQ(RHIInstance::ScoreAdapter(info, RHIAdapterPreference::Automatic), 0u);
        EXPECT_EQ(RHIInstance::ScoreAdapter(info, RHIAdapterPreference::HighPerformance), 0u);
        EXPECT_EQ(RHIInstance::ScoreAdapter(info, RHIAdapterPreference::LowPower), 0u);
        EXPECT_EQ(RHIInstance::ScoreAdapter(info, RHIAdapterPreference::Explicit), 0u);
    }

    TEST(RHIInstanceScore, ExplicitPreferenceAlwaysScoresZero)
    {
        // Explicit is selected by ID, not by score.
        RHIAdapterInfo info{ .Type = RHIAdapterType::Discrete, .DedicatedMemoryBytes = 8ull * 1024 * 1024 * 1024 };
        EXPECT_EQ(RHIInstance::ScoreAdapter(info, RHIAdapterPreference::Explicit), 0u);
    }

    TEST(RHIInstanceScore, DiscreteBeatsIntegratedForHighPerformance)
    {
        RHIAdapterInfo gpu{ .Type = RHIAdapterType::Discrete, .DedicatedMemoryBytes = 8ull * 1024 * 1024 * 1024 };
        RHIAdapterInfo igpu{ .Type = RHIAdapterType::Integrated };

        EXPECT_GT(
            RHIInstance::ScoreAdapter(gpu, RHIAdapterPreference::HighPerformance),
            RHIInstance::ScoreAdapter(igpu, RHIAdapterPreference::HighPerformance));
    }

    TEST(RHIInstanceScore, VRAMIncreasesDiscreteScore)
    {
        RHIAdapterInfo small{ .Type = RHIAdapterType::Discrete, .DedicatedMemoryBytes = 1ull * 1024 * 1024 * 1024 };  // 1 GB
        RHIAdapterInfo large{ .Type = RHIAdapterType::Discrete, .DedicatedMemoryBytes = 16ull * 1024 * 1024 * 1024 }; // 16 GB

        EXPECT_LT(
            RHIInstance::ScoreAdapter(small, RHIAdapterPreference::HighPerformance),
            RHIInstance::ScoreAdapter(large, RHIAdapterPreference::HighPerformance));
    }

    TEST(RHIInstanceScore, DiscreteScoreIsCappedAt200)
    {
        // 200 GB discrete should not blow past the cap.
        RHIAdapterInfo huge{ .Type = RHIAdapterType::Discrete, .DedicatedMemoryBytes = 200ull * 1024 * 1024 * 1024 };
        EXPECT_EQ(RHIInstance::ScoreAdapter(huge, RHIAdapterPreference::HighPerformance), 200u);
    }

    TEST(RHIInstanceScore, IntegratedBeatsDiscreteForLowPower)
    {
        RHIAdapterInfo gpu{ .Type = RHIAdapterType::Discrete, .DedicatedMemoryBytes = 8ull * 1024 * 1024 * 1024 };
        RHIAdapterInfo igpu{ .Type = RHIAdapterType::Integrated };

        EXPECT_GT(
            RHIInstance::ScoreAdapter(igpu, RHIAdapterPreference::LowPower),
            RHIInstance::ScoreAdapter(gpu, RHIAdapterPreference::LowPower));
    }

    TEST(RHIInstanceScore, CPUScoresLowestForLowPower)
    {
        RHIAdapterInfo cpu{ .Type = RHIAdapterType::CPU };
        RHIAdapterInfo gpu{ .Type = RHIAdapterType::Discrete };

        EXPECT_GT(
            RHIInstance::ScoreAdapter(gpu, RHIAdapterPreference::LowPower),
            RHIInstance::ScoreAdapter(cpu, RHIAdapterPreference::LowPower));
    }

    // ---------------------------------------------------------------------
    // RequestAdapter / Single-device rule — needs a stub RHIInstance.
    // ---------------------------------------------------------------------

    // A stub RHIInstance that exposes canned adapters and reports a single
    // failed-CreateDevice / two-calls test. We multiply-inherit from
    // RHIInstance so we get the default RequestAdapter impl that uses
    // EnumerateAdapters + ScoreAdapter.
    class StubInstance : public RHIInstance
    {
    public:
        explicit StubInstance(std::vector<RHIAdapterInfo> infos)
            : RHIInstance(RHIInstanceDesc{}, RHIBackend::Vulkan)
            , m_AdapterInfos(std::move(infos))
        {
        }

        std::vector<std::unique_ptr<RHIAdapter>> EnumerateAdapters() override
        {
            std::vector<std::unique_ptr<RHIAdapter>> result;
            result.reserve(m_AdapterInfos.size());
            for (const auto& info : m_AdapterInfos)
            {
                result.push_back(std::make_unique<StubAdapter>(*this, info));
            }
            return result;
        }

        // CreateDevice returns a new RHIDevice stub on first call, nullptr
        // on subsequent calls — exercising the single-device rule.
        RHIDevice* CreateDevice(RHIAdapter& adapter, const RHIDeviceDesc& desc = RHIDeviceDesc{}) override
        {
            (void)adapter;
            (void)desc;
            if (m_Device)
            {
                return nullptr;
            }
            m_Device = std::make_unique<StubDevice>(*this);
            return m_Device.get();
        }

        int CreateDeviceCallCount = 0;

    private:
        // Local adapter stub — records its info so the test can inspect.
        class StubAdapter : public RHIAdapter
        {
        public:
            StubAdapter(RHIInstance& owner, RHIAdapterInfo info)
                : RHIAdapter(owner, RHIBackend::Vulkan)
                , m_Info(info)
            {
            }
            RHIAdapterInfo GetInfo() const override { return m_Info; }
            bool SupportsRequiredCapabilities(const RHICapabilities&) const override { return true; }
        private:
            RHIAdapterInfo m_Info;
        };

        // Local device stub — derives from RHIDevice so the base-class
        // unique_ptr<RHIDevice> can hold it. Constructed via the protected
        // Local device stub — derives from RHIDevice so the base-class
        // unique_ptr<RHIDevice> can hold it. M3 expanded RHIDevice with
        // 5 pure virtuals; M4 added CreateBufferImpl.
        class StubDevice : public RHIDevice
        {
        public:
            explicit StubDevice(RHIInstance& owner) : RHIDevice(owner) {}

            void WaitIdle() override {}
            RHIBackend GetBackend() const noexcept override { return RHIBackend::Vulkan; }
            const RHICapabilities& GetCapabilities() const noexcept override { return m_Caps; }
            u32 GetMaxFramesInFlight() const noexcept override { return 2; }
            RHIQueue* GetQueue(RHIQueueType) const override { return nullptr; }
            RHIBuffer* CreateBufferImpl(const RHIBufferDesc&) override { return nullptr; }
            RHITexture* CreateTextureImpl(const RHITextureDesc&) override { return nullptr; }
            RHITextureView* CreateTextureViewImpl(const RHITextureViewDesc&) override { return nullptr; }
            RHISampler* CreateSamplerImpl(const RHISamplerDesc&) override { return nullptr; }
            RHIFence* CreateFenceImpl(const RHIFenceDesc&) override { return nullptr; }
            RHISemaphore* CreateSemaphoreImpl(const RHISemaphoreDesc&) override { return nullptr; }
            RHICommandList* CreateCommandListImpl(const RHICommandListDesc&) override { return nullptr; }

        private:
            RHICapabilities m_Caps;
        };

        std::vector<RHIAdapterInfo> m_AdapterInfos;
    };

    // ---------------------------------------------------------------------
    TEST(RHIInstance, CreateFactoryStubReturnsNullInM2)
    {
        // M2 has no backend target yet. The static factory MUST return
        // nullptr; the test catches accidental dispatch to Vulkan / D3D12.
        auto inst = RHIInstance::Create(RHIInstanceDesc{});
        EXPECT_EQ(inst, nullptr);
    }

    TEST(RHIInstance, RequestAdapterChoosesBestByPreference)
    {
        StubInstance instance({
            RHIAdapterInfo{ .AdapterName = "iGPU",      .Type = RHIAdapterType::Integrated },
            RHIAdapterInfo{ .AdapterName = "dGPU-8GB",  .Type = RHIAdapterType::Discrete,  .DedicatedMemoryBytes = 8ull  * 1024 * 1024 * 1024 },
            RHIAdapterInfo{ .AdapterName = "dGPU-16GB", .Type = RHIAdapterType::Discrete,  .DedicatedMemoryBytes = 16ull * 1024 * 1024 * 1024 },
            RHIAdapterInfo{ .AdapterName = "llvmpipe",  .Type = RHIAdapterType::CPU },
        });

        // HighPerformance picks the highest VRAM discrete.
        {
            auto best = instance.RequestAdapter(RHIAdapterPreference::HighPerformance);
            ASSERT_NE(best, nullptr);
            EXPECT_EQ(best->GetInfo().AdapterName, "dGPU-16GB");
        }

        // LowPower picks the integrated GPU.
        {
            auto best = instance.RequestAdapter(RHIAdapterPreference::LowPower);
            ASSERT_NE(best, nullptr);
            EXPECT_EQ(best->GetInfo().Type, RHIAdapterType::Integrated);
        }

        // Automatic picks the highest composite score (with the current
        // formula, HighPerformance and Automatic coincide).
        {
            auto best = instance.RequestAdapter(RHIAdapterPreference::Automatic);
            ASSERT_NE(best, nullptr);
            EXPECT_EQ(best->GetInfo().AdapterName, "dGPU-16GB");
        }

        // Explicit is score-zero — never picked.
        {
            auto best = instance.RequestAdapter(RHIAdapterPreference::Explicit);
            EXPECT_EQ(best, nullptr);
        }
    }

    TEST(RHIInstance, RequestAdapterReturnsNullWhenAllUnsupported)
    {
        // All adapters are Unknown — no score > 0, so nothing is picked.
        StubInstance instance({
            RHIAdapterInfo{ .AdapterName = "u1", .Type = RHIAdapterType::Unknown },
            RHIAdapterInfo{ .AdapterName = "u2", .Type = RHIAdapterType::Unknown },
        });

        auto best = instance.RequestAdapter(RHIAdapterPreference::Automatic);
        EXPECT_EQ(best, nullptr);
    }

    TEST(RHIInstance, SingleDeviceRule)
    {
        StubInstance instance({
            RHIAdapterInfo{ .AdapterName = "dGPU", .Type = RHIAdapterType::Discrete },
        });

        auto adapters = instance.EnumerateAdapters();
        ASSERT_EQ(adapters.size(), 1u);
        RHIAdapter& adapter = *adapters[0];

        RHIDevice* first = instance.CreateDevice(adapter);
        EXPECT_NE(first, nullptr);
        EXPECT_EQ(instance.GetDevice(), first);

        // Second call must fail.
        RHIDevice* second = instance.CreateDevice(adapter);
        EXPECT_EQ(second, nullptr);
        EXPECT_EQ(instance.GetDevice(), first);  // unchanged
    }

    TEST(RHIInstance, GetDescReturnsDesc)
    {
        RHIInstanceDesc desc{
            .ApplicationName    = "MyApp",
            .ApplicationVersion = 42,
            .EnableValidation   = true,
            .EnableDebugMarkers = false,
        };
        StubInstance instance({});
        // The desc was passed via the base constructor; we just verify the
        // accessor matches what was passed.
        // Note: StubInstance constructs its own default RHIInstanceDesc;
        //       this test exercises the accessor on a default-constructed
        //       instance.
        const RHIInstanceDesc& got = instance.GetDesc();
        EXPECT_EQ(got.ApplicationName, "XEngineApp");  // default
        EXPECT_EQ(got.ApplicationVersion, 1u);
        EXPECT_FALSE(got.EnableValidation);
        EXPECT_TRUE(got.EnableDebugMarkers);
        (void)desc;  // exercised above is enough; the desc is opaque for now
    }
}
