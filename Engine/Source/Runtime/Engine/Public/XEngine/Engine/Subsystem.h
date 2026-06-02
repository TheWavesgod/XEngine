#pragma once

namespace XEngine
{
    class ISubsystem
    {
    public:
        virtual ~ISubsystem() = default;

        virtual void OnCreate() {}
        virtual void OnDestroy() {}

        virtual void OnBeginFrame() {}
        virtual void OnUpdate(float deltaTime) {}
        virtual void OnEndFrame() {}
    };
}
