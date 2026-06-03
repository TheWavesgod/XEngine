#pragma once

#include <XEngine/Engine/Subsystem.h>

namespace XEngine
{
    class EditorSystem : public ISubsystem
    {
    public:
        void OnCreate(const SubsystemContext& context) override;
        void OnDestroy() override;
    };
}
