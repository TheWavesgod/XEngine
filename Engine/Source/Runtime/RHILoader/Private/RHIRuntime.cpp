// RHIRuntime — registry implementation.
//
// Holds the in-process list of registered RHI backends. The list is
// thread-safe (single std::mutex around an std::vector<RHIBackendFactoryEntry>).
//
// Hot-path note: CreateInstance takes a snapshot of the registry under
// the lock, releases the lock, then invokes the factory outside the lock.
// This avoids holding a mutex while user code (often heavy with allocations
// and dynamic library calls) runs, and also prevents accidental deadlock
// if a factory callback re-enters the registry.

#include <XEngine/RHI/RHIRuntime.h>
#include <XEngine/Logging/Log.h>

#include <algorithm>
#include <mutex>
#include <string>
#include <utility>

namespace XEngine
{
    namespace
    {
        // Case-insensitive ASCII equality for short strings (no wide / locale).
        bool EqualsIgnoreCase(std::string_view a, std::string_view b) noexcept
        {
            if (a.size() != b.size())
            {
                return false;
            }
            for (size_t i = 0; i < a.size(); ++i)
            {
                const char ca = static_cast<char>(
                    (a[i] >= 'A' && a[i] <= 'Z') ? (a[i] + ('a' - 'A')) : a[i]);
                const char cb = static_cast<char>(
                    (b[i] >= 'A' && b[i] <= 'Z') ? (b[i] + ('a' - 'A')) : b[i]);
                if (ca != cb)
                {
                    return false;
                }
            }
            return true;
        }
    }

    // -------------------------------------------------------------------------
    // Internal registry state. Anonymous namespace = file-local linkage, single
    // instance per process (the registry is process-wide).
    // -------------------------------------------------------------------------
    namespace
    {
        std::mutex              g_RegistryMutex;
        std::vector<RHIBackendFactoryEntry>& g_Registry()
        {
            // Meyers singleton — constructed on first call, destroyed at exit.
            // No static init order dependency: nothing else runs before the
            // first Register/Unregister/CreateInstance call from App main().
            static std::vector<RHIBackendFactoryEntry> s_Registry;
            return s_Registry;
        }
    }

    // -------------------------------------------------------------------------
    void RHIRuntime::RegisterBackend(const RHIBackendFactoryEntry& entry) noexcept
    {
        // Validate before taking the lock.
        if (entry.Factory == nullptr)
        {
            XENGINE_LOG_WARN("RHIRuntime::RegisterBackend: refusing entry with null Factory.");
            return;
        }

        std::lock_guard<std::mutex> lock(g_RegistryMutex);
        auto& reg = g_Registry();

        const auto it = std::find_if(reg.begin(), reg.end(),
            [&](const RHIBackendFactoryEntry& e) { return e.Backend == entry.Backend; });

        if (it == reg.end())
        {
            reg.push_back(entry);
        }
        else
        {
            // Replace — Register is idempotent by Backend enum. The most
            // recent caller wins. App / DLL loaders rely on this for
            // override semantics.
            *it = entry;
        }
    }

    void RHIRuntime::UnregisterBackend(RHIBackend backend) noexcept
    {
        std::lock_guard<std::mutex> lock(g_RegistryMutex);
        auto& reg = g_Registry();

        reg.erase(std::remove_if(reg.begin(), reg.end(),
            [&](const RHIBackendFactoryEntry& e) { return e.Backend == backend; }), reg.end());
    }

    std::unique_ptr<RHIInstance> RHIRuntime::CreateInstance(
        const RHIInstanceDesc& desc,
        RHIBackend             preference) noexcept
    {
        // Snapshot under lock; invoke factory outside lock.
        std::vector<RHIBackendFactoryEntry> snapshot;
        {
            std::lock_guard<std::mutex> lock(g_RegistryMutex);
            snapshot = g_Registry();
        }

        if (snapshot.empty())
        {
            return nullptr;
        }

        // Exact match path.
        if (preference != RHIBackend::None)
        {
            for (const auto& e : snapshot)
            {
                if (e.Backend == preference)
                {
                    return e.Factory(desc);
                }
            }
            // Preference specified but no matching registration. No silent
            // fallback — App asked for one specific backend and didn't get it.
            return nullptr;
        }

        // Auto path: highest Priority first. stable_sort preserves insertion
        // order within equal Priority so manual Register order matters as
        // a tiebreaker.
        std::stable_sort(snapshot.begin(), snapshot.end(),
            [](const RHIBackendFactoryEntry& a, const RHIBackendFactoryEntry& b)
            {
                return a.Priority > b.Priority;
            });

        for (const auto& e : snapshot)
        {
            if (auto inst = e.Factory(desc))
            {
                return inst;
            }
        }
        return nullptr;
    }

    std::vector<RHIBackendFactoryEntry> RHIRuntime::EnumerateBackends() noexcept
    {
        std::lock_guard<std::mutex> lock(g_RegistryMutex);
        return g_Registry();
    }

    RHIBackend RHIRuntime::ParseBackend(std::string_view name) noexcept
    {
        // Trim whitespace would be nice, but the XEngine logging API takes
        // string_view; callers usually pass argv-style non-whitespace
        // strings, so we skip trim to keep ParseBackend zero-allocation.

        if (name.empty()
            || EqualsIgnoreCase(name, "auto")
            || EqualsIgnoreCase(name, "default"))
        {
            return RHIBackend::None;
        }
        if (EqualsIgnoreCase(name, "vulkan")
            || EqualsIgnoreCase(name, "vk"))
        {
            return RHIBackend::Vulkan;
        }
        if (EqualsIgnoreCase(name, "d3d12")
            || EqualsIgnoreCase(name, "dx12"))
        {
            return RHIBackend::D3D12;
        }
        if (EqualsIgnoreCase(name, "metal")
            || EqualsIgnoreCase(name, "mtl"))
        {
            return RHIBackend::Metal;
        }

        // Unknown — treat as Auto so App doesn't crash, but warn so App
        // can spot the typo in logs.
        XENGINE_LOG_WARN(
            std::string("RHIRuntime::ParseBackend: unknown backend name '")
            + std::string(name)
            + "'; falling back to Auto.");
        return RHIBackend::None;
    }

    std::string_view RHIRuntime::GetBackendName(RHIBackend backend) noexcept
    {
        switch (backend)
        {
            case RHIBackend::None:   return "none";
            case RHIBackend::Vulkan: return "vulkan";
            case RHIBackend::D3D12:  return "d3d12";
            case RHIBackend::Metal:  return "metal";
        }
        return "none";
    }
}