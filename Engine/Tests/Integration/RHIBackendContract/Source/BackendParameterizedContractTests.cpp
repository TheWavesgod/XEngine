// Backend-parameterized RHI contract tests.
//
// Goal: one TEST_P body, runs against every backend linked into this binary.
// To verify "backend X implements the contract correctly":
//   1. Build with that backend's target enabled
//   2. Run this binary on a machine with the relevant GPU
//   3. Observe the parameterized case for X (CTest name: "AllBackends/BackendContract.<Test>/<X>")
//
// On a host without the GPU (headless CI, missing driver), the fixture
// GTEST_SKIP()s — so the binary is always safe to run, but missing GPUs
// won't be silently passed. The skip message tells you exactly why.
//
// Capability tiers (M3 → M6+):
//   Tier 1 — RHIInstance creation succeeds (CreateInstance non-null).
//   Tier 2 — RHIAdapter enumeration succeeds (EnumerateAdapters ≥ 1).
//   Tier 3 — RHIDevice creation succeeds (CreateDevice non-null) and
//            its GetBackend / GetCapabilities / GetMaxFramesInFlight
//            read accessors return non-default values.
//   Tier 4 — Queue family retrieval succeeds (Graphics/Compute/Transfer
//            all non-null). WaitIdle is idempotent.
//   Tier 5 — Resource factories (Buffer/Texture/Sampler/Fence/Semaphore/
//            CommandList) return non-null for valid descs.
//
// As of the M3 skeleton, Vulkan reaches Tier 4 (queues), D3D12 reaches
// Tier 3 (D3D12Device::GetQueue is a null stub), Metal is Tier 0.
// Tests skip cleanly against lower-tier backends rather than failing.
//
// Known bug noted for follow-up:
//   RHIObject::GetBackend() is non-virtual while RHIDevice::GetBackend() is
//   pure virtual. Calling GetBackend() through an RHIObject& reference to
//   a device returns RHIObject::m_Backend (None for VulkanDevice/D3D12Device
//   — neither sets it), not the override's hardcoded value. We deliberately
//   avoid XEngine::CheckedCast on devices here and use the typed pointer
//   to call GetBackend() so virtual dispatch fires.

#include <gtest/gtest.h>

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/Base.h>
#include <XEngine/RHI/RHIInstance.h>
#include <XEngine/RHI/RHIAdapter.h>
#include <XEngine/RHI/RHIDevice.h>
#include <XEngine/RHI/RHIQueue.h>
#include <XEngine/RHI/RHIFence.h>
#include <XEngine/RHI/RHIRuntime.h>
#include <XEngine/RHI/RHIDescriptors.h>
#include <XEngine/RHI/RHIValidation.h>
#include <XEngine/RHI/RHIFlags.h>
#include <XEngine/Test/TestSupport.h>

#ifdef XENGINE_HAS_VULKAN_BACKEND
    #include <XEngine/VulkanRHI/Backend.h>
#endif
#ifdef XENGINE_HAS_D3D12_BACKEND
    #include <XEngine/D3D12RHI/Backend.h>
#endif

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace
{
    // Build a list of backends we want to validate. Macros come from
    // CMakeLists.txt — set only when the matching backend target exists.
    constexpr XEngine::RHIBackend kHardcodedBackends[] = {
#ifdef XENGINE_HAS_VULKAN_BACKEND
        XEngine::RHIBackend::Vulkan,
#endif
#ifdef XENGINE_HAS_D3D12_BACKEND
        XEngine::RHIBackend::D3D12,
#endif
        // Metal: not built. Listed unconditionally so the test surface
        // stays stable when the Metal target lands — SetUp() will skip it.
        XEngine::RHIBackend::Metal,
    };

    // Build a typed RHIInstanceDesc that's recognisable across backends.
    XEngine::RHIInstanceDesc MakeInstanceDesc()
    {
        XEngine::RHIInstanceDesc desc{};
        desc.ApplicationName     = "XEngineContractTest";
        desc.ApplicationVersion  = 0;
        desc.EnableValidation    = false;
        desc.EnableDebugMarkers  = true;
        return desc;
    }

    // True iff this binary actually has a registered factory for backend b.
    bool BackendIsRegistered(XEngine::RHIBackend b)
    {
        for (const auto& entry : XEngine::RHIRuntime::EnumerateBackends())
        {
            if (entry.Backend == b) return true;
        }
        return false;
    }

    // Try to instantiate via RHIRuntime with explicit preference. Returns
    // nullptr if the factory returned nullptr (no GPU, no driver, etc.).
    std::unique_ptr<XEngine::RHIInstance> TryCreate(XEngine::RHIBackend b)
    {
        return XEngine::RHIRuntime::CreateInstance(MakeInstanceDesc(), b);
    }

    // Returns a human-readable tier label for skip messages.
    const char* TierLabel(int tier)
    {
        switch (tier)
        {
        case 1: return "Instance";
        case 2: return "Adapter";
        case 3: return "Device";
        case 4: return "Queue";
        case 5: return "Resource";
        default: return "None";
        }
    }

    std::string BackendName(XEngine::RHIBackend b)
    {
        const auto n = XEngine::RHIRuntime::GetBackendName(b);
        return std::string(n.data(), n.size());
    }

    // Auto-register every backend that's actually linked into this binary
    // at static-init time. RHIRuntime is a process-wide global registry,
    // so this is the only way a standalone contract test run gets any
    // backend registered — without this, EnumerateBackends() is empty and
    // every parameterized case SKIPs.
    struct BackendAutoRegisterForTests
    {
        BackendAutoRegisterForTests()
        {
#ifdef XENGINE_HAS_VULKAN_BACKEND
            XEngine::VulkanRHI::Register();
#endif
#ifdef XENGINE_HAS_D3D12_BACKEND
            XEngine::D3D12RHI::Register();
#endif
        }
    };
    static BackendAutoRegisterForTests g_BackendAutoRegister;
}

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

class BackendContract : public ::testing::TestWithParam<XEngine::RHIBackend>
{
protected:
    void SetUp() override
    {
        const auto backend = GetParam();

        if (!BackendIsRegistered(backend))
        {
            GTEST_SKIP() << BackendName(backend)
                         << " backend target is not linked into this test binary "
                         << "(CMake option not enabled).";
        }

        m_Instance = TryCreate(backend);
        if (!m_Instance)
        {
            GTEST_SKIP() << BackendName(backend)
                         << " factory returned nullptr — likely no "
                         << BackendName(backend) << " GPU or driver available.";
        }
    }

    void TearDown() override
    {
        // The RHIInstance owns the RHIDevice (single-device rule). Our
        // m_Device is a non-owning pointer; clearing it before destroying
        // the instance avoids any confusion in destruction order.
        m_Device = nullptr;
        m_Instance.reset();  // device is destroyed here as part of m_Instance.
    }

    // Lazy device creation: only when a test actually needs Tier ≥ 3.
    // SKIPs (via GTEST_SKIP) when the backend cannot provide a usable
    // device. Callers should immediately follow with an m_Device null
    // check — GTEST_SKIP expands to `return void` so it can only be
    // called from void-returning test bodies (gtest 1.15.2 behaviour).
    //
    // NB: RHIDevice is owned by the RHIInstance (single-device rule), so
    // we keep a non-owning raw pointer here. Calling CreateDevice a second
    // time on the same instance returns nullptr by design.
    void EnsureDevice()
    {
        if (m_Device)
        {
            return;
        }

        const auto backend = GetParam();
        auto adapters = m_Instance->EnumerateAdapters();
        if (adapters.empty())
        {
            GTEST_SKIP() << BackendName(backend)
                         << " backend has no adapters (headless? no display?).";
            return;
        }

        m_Device = m_Instance->CreateDevice(*adapters.front(), {});
        if (!m_Device)
        {
            GTEST_SKIP() << BackendName(backend)
                         << " CreateDevice returned nullptr — backend cannot "
                         << "build a real device on this host (M3 skeleton?).";
            return;
        }
    }

    XEngine::RHIBackend CurrentBackend() const noexcept { return GetParam(); }

    XEngine::RHIInstance* Instance() noexcept
    {
        return m_Instance.get();
    }

    XEngine::RHIDevice* Device() noexcept
    {
        return m_Device;
    }

private:
    std::unique_ptr<XEngine::RHIInstance> m_Instance;
    XEngine::RHIDevice*                   m_Device = nullptr;  // non-owning
};

// ---------------------------------------------------------------------------
// Smoke (Tier 1-2)
// ---------------------------------------------------------------------------

TEST_P(BackendContract, Smoke_CreateInstanceSucceeds)
{
    // SetUp() guarantees m_Instance is non-null for backends that support
    // instantiation; the fixture already SKIPped otherwise.
    ASSERT_NE(Instance(), nullptr);
}

TEST_P(BackendContract, Smoke_InstanceBackendTagMatchesRequested)
{
    // m_Backend on RHIInstance is set by the backend constructor
    // (RHIInstance(desc, RHIBackend::X)), so this works correctly through
    // the non-virtual RHIObject::GetBackend.
    EXPECT_EQ(Instance()->GetBackend(), CurrentBackend());
}

TEST_P(BackendContract, Smoke_InstanceDescRoundTrips)
{
    const auto& desc = Instance()->GetDesc();
    // MakeInstanceDesc sets ApplicationName = "XEngineContractTest" and
    // EnableDebugMarkers = true.
    EXPECT_EQ(std::string(desc.ApplicationName.data(),
                           desc.ApplicationName.size()),
              std::string("XEngineContractTest"));
    EXPECT_TRUE(desc.EnableDebugMarkers);
    EXPECT_FALSE(desc.EnableValidation);
}

// ---------------------------------------------------------------------------
// Smoke (Tier 2: adapters)
// ---------------------------------------------------------------------------

TEST_P(BackendContract, Smoke_EnumerateAdaptersReturnsAtLeastOne)
{
    auto adapters = Instance()->EnumerateAdapters();
    if (adapters.empty())
    {
        GTEST_SKIP() << BackendName(CurrentBackend())
                     << " backend reported zero adapters (no GPU on host?).";
        return;
    }
    EXPECT_GE(adapters.size(), 1u);
    for (auto& a : adapters)
    {
        const auto info = a->GetInfo();
        EXPECT_NE(info.Type, XEngine::RHIAdapterType::Unknown);
    }
}

TEST_P(BackendContract, Smoke_RequestAdapterPicksValidOne)
{
    auto adapter = Instance()->RequestAdapter(
        XEngine::RHIAdapterPreference::HighPerformance,
        XEngine::RHICapabilities{});
    if (!adapter)
    {
        GTEST_SKIP() << BackendName(CurrentBackend())
                     << " RequestAdapter returned nullptr.";
        return;
    }
    const auto info = adapter->GetInfo();
    EXPECT_NE(info.Type, XEngine::RHIAdapterType::Unknown);
}

// ---------------------------------------------------------------------------
// Contract (Tier 1: NVI rejects invalid descs before backend Impl runs)
// ---------------------------------------------------------------------------

TEST_P(BackendContract, Contract_CreateDeviceRejectsZeroMaxFramesInFlight)
{
    auto adapters = Instance()->EnumerateAdapters();
    if (adapters.empty())
    {
        GTEST_SKIP() << "no adapters";
        return;
    }
    XEngine::RHIDeviceDesc bad{};
    bad.MaxFramesInFlight = 0;
    auto* device = Instance()->CreateDevice(*adapters.front(), bad);
    EXPECT_EQ(device, nullptr)
        << "CreateDevice must reject MaxFramesInFlight=0 (ValidateDeviceDesc).";
}

TEST_P(BackendContract, Contract_CreateBufferRejectsInvalidDesc)
{
    EnsureDevice();
    if (!Device()) return;

    XEngine::RHIBufferDesc zeroSize{};
    EXPECT_EQ(Device()->CreateBuffer(zeroSize), nullptr);

    XEngine::RHIBufferDesc noUsage{};
    noUsage.Size = 64;
    noUsage.Usage = XEngine::RHIBufferUsage::None;
    EXPECT_EQ(Device()->CreateBuffer(noUsage), nullptr);
}

TEST_P(BackendContract, Contract_CreateTextureRejectsInvalidDesc)
{
    EnsureDevice();
    if (!Device()) return;

    XEngine::RHITextureDesc noFormat{};
    noFormat.Width = 64;
    noFormat.Height = 64;
    EXPECT_EQ(Device()->CreateTexture(noFormat), nullptr);

    XEngine::RHITextureDesc noUsage{};
    noUsage.Width = 64;
    noUsage.Height = 64;
    noUsage.Format = XEngine::RHIFormat::R8G8B8A8_UNORM;
    noUsage.Usage = XEngine::RHITextureUsage::None;
    EXPECT_EQ(Device()->CreateTexture(noUsage), nullptr);
}

// ---------------------------------------------------------------------------
// Contract (Tier 3: device read accessors)
// ---------------------------------------------------------------------------

TEST_P(BackendContract, Contract_DeviceBackendTagMatchesInstance)
{
    EnsureDevice();
    if (!Device()) return;
    // device->GetBackend() dispatches through the virtual override
    // (RHIDevice::GetBackend) and returns the hardcoded backend tag, which
    // matches the instance. (Avoids the F4 issue with non-virtual
    // RHIObject::GetBackend on devices.)
    EXPECT_EQ(Device()->GetBackend(), Instance()->GetBackend());
}

TEST_P(BackendContract, Contract_DeviceCapabilitiesNonEmpty)
{
    EnsureDevice();
    if (!Device()) return;
    const auto& caps = Device()->GetCapabilities();
    // MaxTextureSize2D / MaxSamplerAnisotropy are 0 in POD default. A real
    // device (even a M3 skeleton device that successfully created a queue
    // family) must populate at least the texture-size cap.
    EXPECT_GT(caps.MaxTextureSize2D, 0u);
}

TEST_P(BackendContract, Contract_DeviceMaxFramesInFlightSensible)
{
    EnsureDevice();
    if (!Device()) return;
    EXPECT_GE(Device()->GetMaxFramesInFlight(), 1u);
}

// ---------------------------------------------------------------------------
// Contract (Tier 4: queues + WaitIdle)
// ---------------------------------------------------------------------------

TEST_P(BackendContract, Contract_GetQueueReturnsGraphicsFamily)
{
    EnsureDevice();
    if (!Device()) return;

    auto* gfx = Device()->GetQueue(XEngine::RHIQueueType::Graphics);
    if (!gfx)
    {
        GTEST_SKIP() << BackendName(CurrentBackend())
                     << " backend does not yet expose a Graphics queue "
                     << "(M3 skeleton?)";
        return;
    }
    EXPECT_EQ(gfx->GetType(), XEngine::RHIQueueType::Graphics);
}

TEST_P(BackendContract, Contract_GetQueueReturnsComputeAndTransferFamilies)
{
    EnsureDevice();
    if (!Device()) return;

    auto* gfx = Device()->GetQueue(XEngine::RHIQueueType::Graphics);
    if (!gfx)
    {
        GTEST_SKIP() << "no Graphics queue → Compute/Transfer tests meaningless.";
        return;
    }
    auto* cmp = Device()->GetQueue(XEngine::RHIQueueType::Compute);
    auto* xfr = Device()->GetQueue(XEngine::RHIQueueType::Transfer);
    // On backends with a single combined queue family, Compute and
    // Transfer may alias the same queue — the contract is that they
    // at least return non-null and report the requested type.
    EXPECT_NE(cmp, nullptr);
    EXPECT_NE(xfr, nullptr);
    if (cmp) EXPECT_EQ(cmp->GetType(), XEngine::RHIQueueType::Compute);
    if (xfr) EXPECT_EQ(xfr->GetType(), XEngine::RHIQueueType::Transfer);
}

TEST_P(BackendContract, Contract_WaitIdleIsIdempotent)
{
    EnsureDevice();
    if (!Device()) return;
    Device()->WaitIdle();
    Device()->WaitIdle();  // must not throw / abort
    SUCCEED();
}

// ---------------------------------------------------------------------------
// Contract (Tier 5: resource factories — only run on backends that implement them)
// ---------------------------------------------------------------------------

TEST_P(BackendContract, Contract_CreateBufferWithValidDesc)
{
    EnsureDevice();
    if (!Device()) return;

    XEngine::RHIBufferDesc desc{};
    desc.Size = 256;
    desc.Usage = XEngine::RHIBufferUsage::Vertex;
    auto* buffer = Device()->CreateBuffer(desc);
    if (!buffer)
    {
        GTEST_SKIP() << BackendName(CurrentBackend())
                     << " CreateBufferImpl is still a nullptr stub (M4+).";
        return;
    }
    EXPECT_EQ(buffer->GetSize(), 256u);
    EXPECT_TRUE(XEngine::HasFlag(buffer->GetUsage(),
                                 XEngine::RHIBufferUsage::Vertex));
    EXPECT_EQ(buffer->GetOwnerDevice(), Device());
    // RHIDevice is the single-device rule owner; on dtor it destroys
    // its device which transitively tears down the VMA allocator /
    // ID3D12Device. To keep that path leak-clean (VMA asserts in
    // debug on leaked allocations), explicitly free the buffer here.
    delete buffer;
}

TEST_P(BackendContract, Contract_CreateFenceRoundTrips)
{
    EnsureDevice();
    if (!Device()) return;

    XEngine::RHIFenceDesc desc{};
    desc.InitialSignaled = false;
    auto* fence = Device()->CreateFence(desc);
    if (!fence)
    {
        GTEST_SKIP() << BackendName(CurrentBackend())
                     << " CreateFenceImpl is still a nullptr stub (M6+).";
        return;
    }
    EXPECT_FALSE(fence->IsSignaled());
}

// ---------------------------------------------------------------------------
// Parameterisation
// ---------------------------------------------------------------------------

INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    BackendContract,
    ::testing::ValuesIn(kHardcodedBackends),
    [](const ::testing::TestParamInfo<XEngine::RHIBackend>& info) {
        return BackendName(info.param);
    });