#pragma once

#include <XEngine/Asset/AssetId.h>

namespace XEngine
{
    template<typename T>
    class AssetHandle
    {
    public:
        AssetId GetId() const { return m_Id; }
        bool IsValid() const { return m_Id != 0; }

    private:
        AssetId m_Id = 0;
    };
}

