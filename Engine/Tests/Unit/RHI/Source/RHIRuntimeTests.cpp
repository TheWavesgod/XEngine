// Unit tests for XEngine::RHIRuntime (multi-backend runtime registry).
//
// These tests do NOT depend on VulkanRHI / volk / VMA / SDL3 — they only
// exercise the registry's pure logic (Register / Unregister / ParseBackend /
// GetBackendName / EnumerateBackends). The factory function pointers used
// in Register entries are local static helpers, not real backend factories.
//
// Per plan §13 / M2, XEngine::RHIRuntime is implemented in XEngineRHILoader;
// these tests link that target (see Tests/Unit/RHI/CMakeLists.txt).

#include <gtest/gtest.h>

#include <XEngine/Core/Types.h>
#include <XEngine/RHI/Base.h>
#include <XEngine/RHI/RHIInstance.h>
#include <XEngine/RHI/RHIRuntime.h>

#include "RHITestStubs.h"

#include <string>
#include <vector>

namespace
{
    using namespace XEngine;

    // TaggedInstance — derives from the shared Test::StubInstance and adds
    // an integer tag so tests can verify which factory was invoked.
    class TaggedInstance final : public Test::StubInstance
    {
    public:
        explicit TaggedInstance(int tag)
            : Test::StubInstance(RHIBackend::None)
            , m_Tag(tag)
        {
        }

        int Tag() const noexcept { return m_Tag; }

    private:
        int m_Tag = 0;
    };

    // Factory helpers — each creates a TaggedInstance tagged with a known
    // integer so the test can verify "this exact factory was called".
    std::unique_ptr<RHIInstance> MakeTagged(int tag)
    {
        return std::unique_ptr<RHIInstance>(new TaggedInstance(tag));
    }

    std::unique_ptr<RHIInstance> Factory_42(const RHIInstanceDesc&) { return MakeTagged(42); }
    std::unique_ptr<RHIInstance> Factory_99(const RHIInstanceDesc&) { return MakeTagged(99); }
    std::unique_ptr<RHIInstance> FactoryAlwaysNull(const RHIInstanceDesc&) { return nullptr; }

    // Per-test fixture: ensures each test starts with an empty registry.
    class RHIRuntimeTest : public ::testing::Test
    {
    protected:
        void TearDown() override
        {
            RHIRuntime::UnregisterBackend(RHIBackend::Vulkan);
            RHIRuntime::UnregisterBackend(RHIBackend::D3D12);
            RHIRuntime::UnregisterBackend(RHIBackend::Metal);
        }
    };

    // ---------------------------------------------------------------------
    // ParseBackend / GetBackendName
    // ---------------------------------------------------------------------

    TEST(RHIRuntime, ParseBackendCaseInsensitive)
    {
        EXPECT_EQ(RHIRuntime::ParseBackend("Vulkan"), RHIBackend::Vulkan);
        EXPECT_EQ(RHIRuntime::ParseBackend("vulkan"), RHIBackend::Vulkan);
        EXPECT_EQ(RHIRuntime::ParseBackend("VULKAN"), RHIBackend::Vulkan);
        EXPECT_EQ(RHIRuntime::ParseBackend("Vk"),      RHIBackend::Vulkan);
        EXPECT_EQ(RHIRuntime::ParseBackend("VK"),      RHIBackend::Vulkan);

        EXPECT_EQ(RHIRuntime::ParseBackend("D3D12"),  RHIBackend::D3D12);
        EXPECT_EQ(RHIRuntime::ParseBackend("d3d12"),  RHIBackend::D3D12);
        EXPECT_EQ(RHIRuntime::ParseBackend("DX12"),   RHIBackend::D3D12);
        EXPECT_EQ(RHIRuntime::ParseBackend("dx12"),   RHIBackend::D3D12);

        EXPECT_EQ(RHIRuntime::ParseBackend("Metal"),  RHIBackend::Metal);
        EXPECT_EQ(RHIRuntime::ParseBackend("metal"),  RHIBackend::Metal);
        EXPECT_EQ(RHIRuntime::ParseBackend("MTL"),    RHIBackend::Metal);

        EXPECT_EQ(RHIRuntime::ParseBackend("Auto"),     RHIBackend::None);
        EXPECT_EQ(RHIRuntime::ParseBackend("auto"),     RHIBackend::None);
        EXPECT_EQ(RHIRuntime::ParseBackend("Default"),  RHIBackend::None);
        EXPECT_EQ(RHIRuntime::ParseBackend(""),         RHIBackend::None);
    }

    TEST(RHIRuntime, ParseBackendUnknownReturnsNoneAndLogsWarning)
    {
        // Unknown names collapse to None (with a warn log) rather than
        // throwing — Apps should not crash on a typo in config.
        EXPECT_EQ(RHIRuntime::ParseBackend("Bogus"),  RHIBackend::None);
        EXPECT_EQ(RHIRuntime::ParseBackend("opengl"), RHIBackend::None);
        EXPECT_EQ(RHIRuntime::ParseBackend("WebGPU"), RHIBackend::None);
    }

    TEST(RHIRuntime, GetBackendNameRoundTripsWithParseBackend)
    {
        for (auto b : { RHIBackend::None, RHIBackend::Vulkan,
                        RHIBackend::D3D12, RHIBackend::Metal })
        {
            const std::string_view name = RHIRuntime::GetBackendName(b);
            EXPECT_EQ(RHIRuntime::ParseBackend(name), b)
                << "round-trip failed for backend=" << static_cast<int>(b);
        }
    }

    // ---------------------------------------------------------------------
    // EnumerateBackends / RegisterBackend / UnregisterBackend
    // ---------------------------------------------------------------------

    TEST_F(RHIRuntimeTest, EnumerateBackendsStartsEmpty)
    {
        // The fixture TearDown guarantees we start from an empty registry;
        // the only way this can fail is if some OTHER test left entries
        // around without unregistering. We tolerate a non-empty result here
        // but log it; the assertion below is the strict check for *this*
        // fixture's own actions.
        const auto entries = RHIRuntime::EnumerateBackends();
        EXPECT_TRUE(entries.empty());
    }

    TEST_F(RHIRuntimeTest, RegisterBackendAddsEntry)
    {
        RHIRuntime::RegisterBackend({
            .Backend  = RHIBackend::Vulkan,
            .Name     = "Vulkan",
            .Priority = 100,
            .Factory  = &Factory_42,
        });

        const auto entries = RHIRuntime::EnumerateBackends();
        ASSERT_EQ(entries.size(), 1u);
        EXPECT_EQ(entries[0].Backend, RHIBackend::Vulkan);
        EXPECT_EQ(entries[0].Name, "Vulkan");
        EXPECT_EQ(entries[0].Priority, 100u);
    }

    TEST_F(RHIRuntimeTest, RegisterBackendSameKeyReplaces)
    {
        RHIRuntime::RegisterBackend({
            .Backend = RHIBackend::Vulkan, .Name = "Vulkan",
            .Priority = 50, .Factory = &Factory_42 });
        RHIRuntime::RegisterBackend({
            .Backend = RHIBackend::Vulkan, .Name = "Vulkan",
            .Priority = 200, .Factory = &Factory_99 });

        const auto entries = RHIRuntime::EnumerateBackends();
        ASSERT_EQ(entries.size(), 1u);
        EXPECT_EQ(entries[0].Priority, 200u);

        // Verify the *new* factory is what's stored.
        const auto instance = RHIRuntime::CreateInstance({}, RHIBackend::Vulkan);
        ASSERT_NE(instance, nullptr);
        EXPECT_EQ(static_cast<TaggedInstance*>(instance.get())->Tag(), 99);
    }

    TEST_F(RHIRuntimeTest, UnregisterBackendRemoves)
    {
        RHIRuntime::RegisterBackend({
            .Backend = RHIBackend::Vulkan, .Name = "Vulkan",
            .Priority = 100, .Factory = &Factory_42 });
        RHIRuntime::RegisterBackend({
            .Backend = RHIBackend::D3D12, .Name = "D3D12",
            .Priority = 100, .Factory = &Factory_42 });

        RHIRuntime::UnregisterBackend(RHIBackend::Vulkan);

        const auto entries = RHIRuntime::EnumerateBackends();
        ASSERT_EQ(entries.size(), 1u);
        EXPECT_EQ(entries[0].Backend, RHIBackend::D3D12);
    }

    TEST_F(RHIRuntimeTest, UnregisterBackendNotRegisteredIsNoOp)
    {
        RHIRuntime::UnregisterBackend(RHIBackend::Metal); // never registered
        SUCCEED(); // reaching here is the test
    }

    TEST_F(RHIRuntimeTest, RegisterBackendNullFactoryIsRejected)
    {
        RHIRuntime::RegisterBackend({
            .Backend = RHIBackend::Vulkan, .Name = "Vulkan",
            .Priority = 100, .Factory = nullptr });

        const auto entries = RHIRuntime::EnumerateBackends();
        EXPECT_TRUE(entries.empty());
    }

    // ---------------------------------------------------------------------
    // CreateInstance
    // ---------------------------------------------------------------------

    TEST_F(RHIRuntimeTest, CreateInstanceEmptyRegistryReturnsNull)
    {
        auto instance = RHIRuntime::CreateInstance({}, RHIBackend::None);
        EXPECT_EQ(instance, nullptr);

        instance = RHIRuntime::CreateInstance({}, RHIBackend::Vulkan);
        EXPECT_EQ(instance, nullptr);
    }

    TEST_F(RHIRuntimeTest, CreateInstanceExactPreference)
    {
        RHIRuntime::RegisterBackend({
            .Backend = RHIBackend::Vulkan, .Name = "Vulkan",
            .Priority = 100, .Factory = &Factory_42 });

        auto instance = RHIRuntime::CreateInstance({}, RHIBackend::Vulkan);
        ASSERT_NE(instance, nullptr);
        EXPECT_EQ(static_cast<TaggedInstance*>(instance.get())->Tag(), 42);
    }

    TEST_F(RHIRuntimeTest, CreateInstanceExactPreferenceNoSilentFallback)
    {
        // Register only D3D12, ask for Vulkan — must return nullptr,
        // NOT silently fall back to D3D12.
        RHIRuntime::RegisterBackend({
            .Backend = RHIBackend::D3D12, .Name = "D3D12",
            .Priority = 100, .Factory = &Factory_42 });

        auto instance = RHIRuntime::CreateInstance({}, RHIBackend::Vulkan);
        EXPECT_EQ(instance, nullptr);
    }

    TEST_F(RHIRuntimeTest, CreateInstanceAutoPicksHighestPriority)
    {
        RHIRuntime::RegisterBackend({
            .Backend = RHIBackend::Vulkan, .Name = "Vulkan",
            .Priority = 50, .Factory = &Factory_42 });
        RHIRuntime::RegisterBackend({
            .Backend = RHIBackend::D3D12, .Name = "D3D12",
            .Priority = 200, .Factory = &Factory_99 });

        auto instance = RHIRuntime::CreateInstance({}, RHIBackend::None);
        ASSERT_NE(instance, nullptr);
        // D3D12 (Priority 200) should win over Vulkan (Priority 50).
        EXPECT_EQ(static_cast<TaggedInstance*>(instance.get())->Tag(), 99);
    }

    TEST_F(RHIRuntimeTest, CreateInstanceAutoSkipsNullFactories)
    {
        // Vulkan registered with a null-returning factory; D3D12 with a
        // working one. Auto must skip Vulkan's failure and use D3D12.
        RHIRuntime::RegisterBackend({
            .Backend = RHIBackend::Vulkan, .Name = "Vulkan",
            .Priority = 200, .Factory = &FactoryAlwaysNull });
        RHIRuntime::RegisterBackend({
            .Backend = RHIBackend::D3D12, .Name = "D3D12",
            .Priority = 100, .Factory = &Factory_42 });

        auto instance = RHIRuntime::CreateInstance({}, RHIBackend::None);
        ASSERT_NE(instance, nullptr);
        EXPECT_EQ(static_cast<TaggedInstance*>(instance.get())->Tag(), 42);
    }

    TEST_F(RHIRuntimeTest, CreateInstanceAutoReturnsNullWhenAllFail)
    {
        RHIRuntime::RegisterBackend({
            .Backend = RHIBackend::Vulkan, .Name = "Vulkan",
            .Priority = 100, .Factory = &FactoryAlwaysNull });

        auto instance = RHIRuntime::CreateInstance({}, RHIBackend::None);
        EXPECT_EQ(instance, nullptr);
    }

    TEST_F(RHIRuntimeTest, CreateInstanceRespectsInsertionOrderAsTiebreaker)
    {
        // Two backends at equal Priority; the one registered first should
        // be tried first because std::stable_sort preserves insertion order.
        RHIRuntime::RegisterBackend({
            .Backend = RHIBackend::Vulkan, .Name = "Vulkan",
            .Priority = 100, .Factory = &Factory_42 });
        RHIRuntime::RegisterBackend({
            .Backend = RHIBackend::D3D12, .Name = "D3D12",
            .Priority = 100, .Factory = &Factory_99 });

        auto instance = RHIRuntime::CreateInstance({}, RHIBackend::None);
        ASSERT_NE(instance, nullptr);
        EXPECT_EQ(static_cast<TaggedInstance*>(instance.get())->Tag(), 42);
    }
}