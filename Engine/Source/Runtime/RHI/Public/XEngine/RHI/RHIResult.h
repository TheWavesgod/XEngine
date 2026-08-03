// RHIResult helpers — RHICHECK macro.
//
// XEngine::Result (Foundation/Core/Result.h) is the engine-wide success /
// failure carrier. The RHI layer reuses it directly via `XEngine::RHIResult`
// (already aliased in XEngine/RHI/Base.h) and adds the XENGINE_RHI_CHECK
// macro for the common pattern:
//
//   XEngine::Result CreateBufferImpl(const RHIBufferDesc& desc) {
//       XENGINE_RHI_CHECK(device.AllocateMemory(...));
//       XENGINE_RHI_CHECK(device.BindBufferMemory(...));
//       return XEngine::Result::Ok();
//   }
//
// The macro logs the result message (if any) and returns the failing Result
// early. The enclosing function MUST return XEngine::Result (or implicitly
// convertible to it).

#pragma once

#include <XEngine/Core/Result.h>
#include <XEngine/RHI/Base.h>
#include <XEngine/Logging/Log.h>

// RHICHECK: evaluate `expr`, log + return `_rhi_result` if not Success.
// The enclosing function MUST return XEngine::Result.
//
// We use a do-while-0 wrapper so the macro can be used in any single-statement
// context (if/else, for-body, ...) without syntax surprises. The
// `if (init; cond)` form (C++17) lets us keep the result variable scoped to
// the macro body.
//
// `XENGINE_LOG_ERROR` takes a std::string_view; we forward the message
// verbatim. If the result has no message we log the literal expression text
// so failure paths are still identifiable in the log.
#define XENGINE_RHI_CHECK(expr)                                                            \
    do                                                                                     \
    {                                                                                      \
        if (auto _rhi_result = (expr); !_rhi_result)                                       \
        {                                                                                  \
            if (_rhi_result.Message.empty())                                               \
            {                                                                              \
                XENGINE_LOG_ERROR("RHI: " #expr);                                          \
            }                                                                              \
            else                                                                           \
            {                                                                              \
                XENGINE_LOG_ERROR("RHI: " #expr " -> " + _rhi_result.Message);             \
            }                                                                              \
            return _rhi_result;                                                            \
        }                                                                                  \
    } while (false)
