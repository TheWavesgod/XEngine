#pragma once

#include <XEngine/Engine/Subsystem.h>

#include <memory>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace XEngine
{
    class SubsystemManager
    {
    public:
        SubsystemManager() = default;
        ~SubsystemManager();

        SubsystemManager(const SubsystemManager&) = delete;
        SubsystemManager& operator=(const SubsystemManager&) = delete;

        template<typename T, typename... Args>
        T& AddSubsystem(Args&&... args)
        {
            static_assert(std::is_base_of_v<ISubsystem, T>, "T must derive from ISubsystem");

            auto subsystem = std::make_unique<T>(std::forward<Args>(args)...);
            T* rawSubsystem = subsystem.get();

            m_SubsystemLookup[typeid(T)] = rawSubsystem;
            m_Subsystems.emplace_back(std::move(subsystem));

            return *rawSubsystem;
        }

        template<typename T>
        T* GetSubsystem()
        {
            auto it = m_SubsystemLookup.find(typeid(T));
            if (it == m_SubsystemLookup.end())
            {
                return nullptr;
            }

            return static_cast<T*>(it->second);
        }

        void CreateAll();
        void DestroyAll();

        void BeginFrame();
        void Update(float deltaTime);
        void EndFrame();

        bool IsCreated() const;

    private:
        std::vector<std::unique_ptr<ISubsystem>> m_Subsystems;
        std::unordered_map<std::type_index, ISubsystem*> m_SubsystemLookup;

        bool m_Created = false;
    };
}
