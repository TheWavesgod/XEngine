#pragma once

#include <XEngine/Scene/Entity.h>

namespace XEngine
{
    class Scene;
    struct TransformComponent;

    class TransformSystem
    {
    public:
        void Update(Scene& scene);

    private:
        void UpdateRecursive(
            Scene& scene,
            Entity entity,
            const TransformComponent* parentTransform);
    };
}
