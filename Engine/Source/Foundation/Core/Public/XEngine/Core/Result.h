#pragma once

#include <string>
#include <utility>

namespace XEngine
{
    struct Result
    {
        bool Success = true;
        std::string Message;

        static Result Ok()
        {
            return {};
        }

        static Result Failure(std::string message)
        {
            return {false, std::move(message)};
        }

        explicit operator bool() const
        {
            return Success;
        }
    };
}
