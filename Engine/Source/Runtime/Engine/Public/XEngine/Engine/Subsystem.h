#pragma once

#include <XEngine/Engine/SubsystemContext.h>

namespace XEngine
{
    class ISubsystem
    {
    public:
        virtual ~ISubsystem() = default;

        virtual void OnCreate(const SubsystemContext& context) {}
        virtual void OnDestroy() {}

        virtual void OnBeginFrame() {}
        virtual void OnUpdate(float deltaTime) {}
        virtual void OnEndFrame() {}
    };
}
