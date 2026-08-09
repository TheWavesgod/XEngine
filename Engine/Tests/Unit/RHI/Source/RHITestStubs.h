// Shared RHI test stubs for XEngineRHIUnitTests.
//
// Why this header exists (audit trail):
//   Each unit-test translation unit previously defined its own copy of
//   RHIInstance / RHIDevice / RHIQueue / RHICommandList / RHIFence /
//   RHISemaphore stubs inside `namespace XEngine { namespace { ... } }`.
//   MSVC's debug-info emission and the linker have been observed picking a
//   sibling-TU definition whose layout differs from the one used to allocate
//   the stack object, which corrupts the stack on destruction (RHIQueue.Submit*).
//
// How this header avoids the problem:
//   * One canonical definition per common stub class, in the named namespace
//     `XEngine::Test` (NOT an anonymous namespace).
//   * Every member function is defined inside the class body, so it is
//     implicitly `inline` and ODR-safe across all consuming TUs.
//   * Resource-bearing stubs (StubBuffer / StubTexture / ...) stay local to
//     the test file that needs them — only the layout-neutral base stubs are
//     shared, because those are the ones whose duplication triggered the bug.
//
// Usage:
//   #include "RHITestStubs.h"
//   namespace { using StubInstance = XEngine::Test::StubInstance;
//              using StubDevice   = XEngine::Test::StubDevice; }
//   ...
//   StubInstance instance;
//   StubDevice   device(instance);
//   Test::StubQueue queue(device, RHIBackend::Vulkan, RHIQueueType::Graphics);
//   Test::StubCommandList cmdList(device);
//   queue.Submit(&cmdList);
//   EXPECT_EQ(queue.GetLastCmdList(), &cmdList);
//
// Specialization (when a test needs extra state):
//   class RecordingCommandList final : public Test::StubCommandList { ... };
//   class BufferTestDevice      final : public Test::StubDevice
//   {
//       RHIBuffer* CreateBufferImpl(const RHIBufferDesc& desc) override { ... }
//   };
//   The shared factory hooks remain `virtual` and non-`final` so this works.
//
// The factory methods of StubDevice (CreateBufferImpl, CreateTextureImpl, ...)
// all return nullptr by default. Override only the one(s) your test needs.

#pragma once

#include <XEngine/RHI/Base.h>
#include <XEngine/RHI/RHIObject.h>
#include <XEngine/RHI/RHIInstance.h>
#include <XEngine/RHI/RHIDevice.h>
#include <XEngine/RHI/RHIQueue.h>
#include <XEngine/RHI/RHICommandList.h>
#include <XEngine/RHI/RHIFence.h>
#include <XEngine/RHI/RHISemaphore.h>
#include <XEngine/RHI/RHIDescriptors.h>
#include <XEngine/RHI/RHIEnums.h>

#include <memory>
#include <span>
#include <vector>

namespace XEngine::Test
{
    // ---------------------------------------------------------------------
    // StubInstance — neutral RHIInstance. EnumerateAdapters() returns an
    // empty vector; CreateDeviceImpl() returns nullptr. Subclass to inject
    // canned adapters or a real device-creation hook.
    // ---------------------------------------------------------------------
    class StubInstance : public RHIInstance
    {
    public:
        explicit StubInstance(RHIBackend backend = RHIBackend::Vulkan)
            : RHIInstance(RHIInstanceDesc{}, backend)
        {
        }

        std::vector<std::unique_ptr<RHIAdapter>> EnumerateAdapters() override
        {
            return {};
        }

        std::unique_ptr<RHIDevice> CreateDeviceImpl(
            RHIAdapter& /*adapter*/,
            const RHIDeviceDesc& /*desc*/) override
        {
            return nullptr;
        }
    };

    // ---------------------------------------------------------------------
    // StubDevice — neutral RHIDevice. All factory hooks return nullptr;
    // override only the one your test exercises. Capabilities, frames-in-
    // flight, and enabled features return sensible neutral defaults.
    // ---------------------------------------------------------------------
    class StubDevice : public RHIDevice
    {
    public:
        explicit StubDevice(RHIInstance& owner)
            : RHIDevice(owner, RHIBackend::Vulkan)
        {
        }

        StubDevice(RHIInstance& owner, RHIBackend backend)
            : RHIDevice(owner, backend)
            , m_Backend(backend)
        {
        }

        void WaitIdle() override {}

        RHIBackend GetBackend() const noexcept override { return m_Backend; }

        const RHICapabilities& GetCapabilities() const noexcept override
        {
            return m_Caps;
        }

        u32 GetMaxFramesInFlight() const noexcept override { return 2; }

        RHIFeature GetEnabledFeatures() const noexcept override
        {
            return RHIFeature::None;
        }

        RHIQueue* GetQueue(RHIQueueType /*type*/) const override
        {
            return nullptr;
        }

        RHIBuffer* CreateBufferImpl(const RHIBufferDesc& /*desc*/) override
        {
            return nullptr;
        }

        RHITexture* CreateTextureImpl(const RHITextureDesc& /*desc*/) override
        {
            return nullptr;
        }

        RHITextureView* CreateTextureViewImpl(
            const RHITextureViewDesc& /*desc*/) override
        {
            return nullptr;
        }

        RHISampler* CreateSamplerImpl(const RHISamplerDesc& /*desc*/) override
        {
            return nullptr;
        }

        RHIFence* CreateFenceImpl(const RHIFenceDesc& /*desc*/) override
        {
            return nullptr;
        }

        RHISemaphore* CreateSemaphoreImpl(const RHISemaphoreDesc& /*desc*/) override
        {
            return nullptr;
        }

        RHICommandList* CreateCommandListImpl(
            const RHICommandListDesc& /*desc*/) override
        {
            return nullptr;
        }

    protected:
        // Subclasses (e.g., RHIDeviceTests' configurable device) can poke
        // these directly without going through a setter API.
        RHIBackend     m_Backend = RHIBackend::Vulkan;
        RHICapabilities m_Caps;
    };

    // ---------------------------------------------------------------------
    // StubQueue — RHIQueue that records the most recent Submit call so
    // tests can assert what was submitted. Owns the captured pointers'
    // lifetime is the caller's responsibility (the queue keeps non-owning
    // pointers, as the production RHIQueue contract does).
    // ---------------------------------------------------------------------
    class StubQueue : public RHIQueue
    {
    public:
        StubQueue(RHIDevice& owner, RHIBackend backend, RHIQueueType type)
            : RHIQueue(owner, backend)
            , m_Type(type)
        {
        }

        RHIQueueType GetType() const noexcept override { return m_Type; }

        void Submit(
            RHICommandList* commandList,
            RHIFence* signalFence = nullptr,
            std::span<RHISemaphore*> waitSemaphores = {},
            std::span<RHISemaphore*> signalSemaphores = {}) override
        {
            m_LastCmdList = commandList;
            m_LastSignalFence = signalFence;
            m_LastWaitSemaphores.assign(waitSemaphores.begin(),
                                        waitSemaphores.end());
            m_LastSignalSemaphores.assign(signalSemaphores.begin(),
                                          signalSemaphores.end());
        }

        RHICommandList* GetLastCmdList() const noexcept
        {
            return m_LastCmdList;
        }

        RHIFence* GetLastSignalFence() const noexcept
        {
            return m_LastSignalFence;
        }

        const std::vector<RHISemaphore*>& GetLastWaitSemaphores() const noexcept
        {
            return m_LastWaitSemaphores;
        }

        const std::vector<RHISemaphore*>& GetLastSignalSemaphores() const noexcept
        {
            return m_LastSignalSemaphores;
        }

    private:
        RHIQueueType m_Type;
        RHICommandList* m_LastCmdList = nullptr;
        RHIFence* m_LastSignalFence = nullptr;
        std::vector<RHISemaphore*> m_LastWaitSemaphores;
        std::vector<RHISemaphore*> m_LastSignalSemaphores;
    };

    // ---------------------------------------------------------------------
    // StubCommandList — neutral RHICommandList. All recording lifecycle
    // methods are no-ops. Subclass to count Begin/End or capture the last
    // TransitionTexture arguments.
    // ---------------------------------------------------------------------
    class StubCommandList : public RHICommandList
    {
    public:
        explicit StubCommandList(RHIDevice& owner)
            : RHICommandList(owner, owner.GetBackend())
        {
        }

        void Begin() override {}
        void End() override {}

        void TransitionTexture(
            RHITexture* /*texture*/,
            RHIImageLayout /*oldLayout*/,
            RHIImageLayout /*newLayout*/,
            RHIAccessFlags /*srcAccess*/,
            RHIAccessFlags /*dstAccess*/) override
        {
        }
    };

    // ---------------------------------------------------------------------
    // StubFence — neutral RHIFence. Always unsignaled, Wait() always
    // returns true. Subclass (e.g., RHIFenceTests) to add signaled state
    // and a wait counter.
    // ---------------------------------------------------------------------
    class StubFence : public RHIFence
    {
    public:
        explicit StubFence(RHIDevice& owner)
            : RHIFence(owner, owner.GetBackend())
        {
        }

        bool IsSignaled() const noexcept override { return false; }

        bool Wait(u64 /*timeoutNanoseconds*/ = UINT64_MAX) noexcept override
        {
            return true;
        }
    };

    // ---------------------------------------------------------------------
    // StubSemaphore — neutral RHISemaphore. The M6 surface is empty (no
    // CPU-side methods), so this is essentially a typed handle. The
    // constructor wires up the owner and backend like every other stub.
    // ---------------------------------------------------------------------
    class StubSemaphore : public RHISemaphore
    {
    public:
        explicit StubSemaphore(RHIDevice& owner)
            : RHISemaphore(owner, owner.GetBackend())
        {
        }
    };
} // namespace XEngine::Test
