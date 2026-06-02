#pragma once

#include <XEngine/Engine/Subsystem.h>

namespace XEngine
{
    class EditorSystem : public ISubsystem
    {
    public:
        void OnCreate() override;
        void OnDestroy() override;
    };
}
