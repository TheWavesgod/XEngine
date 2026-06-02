#include <XEngine/Engine/SubsystemManager.h>

#include <XEngine/Logging/Log.h>

namespace XEngine
{
    SubsystemManager::~SubsystemManager()
    {
        DestroyAll();
    }

    void SubsystemManager::CreateAll()
    {
        if (m_Created)
        {
            return;
        }

        XENGINE_LOG_INFO("Creating subsystems");

        for (const auto& subsystem : m_Subsystems)
        {
            subsystem->OnCreate();
        }

        m_Created = true;
        XENGINE_LOG_INFO("Subsystems created");
    }

    void SubsystemManager::DestroyAll()
    {
        if (!m_Created)
        {
            return;
        }

        XENGINE_LOG_INFO("Destroying subsystems");

        for (auto it = m_Subsystems.rbegin(); it != m_Subsystems.rend(); ++it)
        {
            (*it)->OnDestroy();
        }

        m_Created = false;
        m_SubsystemLookup.clear();
        m_Subsystems.clear();

        XENGINE_LOG_INFO("Subsystems destroyed");
    }

    void SubsystemManager::BeginFrame()
    {
        for (const auto& subsystem : m_Subsystems)
        {
            subsystem->OnBeginFrame();
        }
    }

    void SubsystemManager::Update(float deltaTime)
    {
        for (const auto& subsystem : m_Subsystems)
        {
            subsystem->OnUpdate(deltaTime);
        }
    }

    void SubsystemManager::EndFrame()
    {
        for (const auto& subsystem : m_Subsystems)
        {
            subsystem->OnEndFrame();
        }
    }

    bool SubsystemManager::IsCreated() const
    {
        return m_Created;
    }
}
