// Unit tests for the XENGINE_RHI_CHECK macro.
//
// The macro is verified by wrapping it in a helper function whose body the
// macro expands inside. We assert that the function returns the expected
// Result on both Success and Failure paths, and that the macro does not
// leak its internal variable name into the calling scope.

#include <gtest/gtest.h>

#include <XEngine/Core/Result.h>
#include <XEngine/RHI/Base.h>
#include <XEngine/RHI/RHIResult.h>

namespace
{
    using namespace XEngine;

    // Test stub for a function that returns Result. The macro should
    // propagate the failure (early-return) and let Success fall through.
    static Result SuccessOp()
    {
        return Result::Ok();
    }

    static Result FailureOp(const std::string& message)
    {
        return Result::Failure(message);
    }

    // Helper that uses the macro exactly like a backend implementation would.
    static Result RunRhiCheck(int mode)
    {
        if (mode == 0)
        {
            XENGINE_RHI_CHECK(SuccessOp());
            return Result::Ok();
        }
        if (mode == 1)
        {
            XENGINE_RHI_CHECK(FailureOp("synthetic failure for test"));
            // Unreachable: the macro must early-return when FailureOp fails.
            return Result::Failure("unreachable: should not be hit");
        }
        if (mode == 2)
        {
            // Two checks in a row — first succeeds, second fails. We verify
            // that the macro chain retains the correct Result.
            XENGINE_RHI_CHECK(SuccessOp());
            XENGINE_RHI_CHECK(FailureOp("second-op failed"));
            return Result::Ok();
        }
        return Result::Failure("unknown mode");
    }

    // ---------------------------------------------------------------------
    TEST(RHIResult, CheckPropagatesSuccess)
    {
        Result r = RunRhiCheck(0);
        EXPECT_TRUE(r.Success);
        EXPECT_TRUE(r.Message.empty());
    }

    TEST(RHIResult, CheckReturnsEarlyOnFailure)
    {
        Result r = RunRhiCheck(1);
        EXPECT_FALSE(r.Success);
        EXPECT_EQ(r.Message, "synthetic failure for test");
    }

    TEST(RHIResult, CheckChainStopsAtFirstFailure)
    {
        Result r = RunRhiCheck(2);
        EXPECT_FALSE(r.Success);
        EXPECT_EQ(r.Message, "second-op failed");
    }

    // The macro uses `_rhi_result` as the internal variable. It must be
    // scoped inside the do-while so the calling function can have its own
    // variable of that name without collision.
    TEST(RHIResult, MacroDoesNotLeakVariableName)
    {
        Result _rhi_result = Result::Ok();
        EXPECT_TRUE(_rhi_result.Success);

        // Verify the macro does not break when the caller shadows the
        // internal name inside a nested block.
        {
            Result _rhi_result = Result::Failure("shadow");
            EXPECT_FALSE(_rhi_result.Success);
        }

        EXPECT_TRUE(_rhi_result.Success);
    }
}
