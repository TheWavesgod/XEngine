#pragma once

#include <XEngine/Engine/Subsystem.h>

#include <memory>
#include <vector>

namespace XEngine
{
    class SubsystemManager
    {
    public:
        void AddSubsystem(std::unique_ptr<ISubsystem> subsystem);
        void CreateAll();
        void DestroyAll();

    private:
        std::vector<std::unique_ptr<ISubsystem>> m_Subsystems;
    };
}
