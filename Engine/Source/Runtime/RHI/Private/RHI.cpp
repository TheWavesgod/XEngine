// New RHI library translation unit.
//
// Per Docs/AI helper/prompts.md, the new XEngineRHI target is a minimal
// skeleton. Backend protocols, instance/adapter/device creation,
// resource creation, and submission are deliberately deferred to later
// stages. This file exists so that XEngineRHI is a real, linkable
// library target even before any concrete RHI types are implemented.

#include <XEngine/RHI/Base.h>

// Version probes. Defined at file scope (not inside the XEngine namespace)
// so that callers using `extern "C"` linkage can resolve them as plain C
// symbols. The values come from the XEngine:: namespace constants declared
// in XEngine/RHI/Base.h.
extern "C" XENGINE_RHI_API std::uint32_t XEngineRHI_GetVersionMajor()
{
    return XEngine::RHIVersionMajor;
}

extern "C" XENGINE_RHI_API std::uint32_t XEngineRHI_GetVersionMinor()
{
    return XEngine::RHIVersionMinor;
}

extern "C" XENGINE_RHI_API std::uint32_t XEngineRHI_GetVersionPatch()
{
    return XEngine::RHIVersionPatch;
}
