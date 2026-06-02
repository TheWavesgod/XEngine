#include <XEngine/Engine/SubsystemManager.h>

namespace XEngine
{
    void SubsystemManager::AddSubsystem(std::unique_ptr<ISubsystem> subsystem)
    {
        m_Subsystems.push_back(std::move(subsystem));
    }

    void SubsystemManager::CreateAll()
    {
        for (const auto& subsystem : m_Subsystems)
        {
            subsystem->OnCreate();
        }
    }

    void SubsystemManager::DestroyAll()
    {
        for (const auto& subsystem : m_Subsystems)
        {
            subsystem->OnDestroy();
        }
    }
}

