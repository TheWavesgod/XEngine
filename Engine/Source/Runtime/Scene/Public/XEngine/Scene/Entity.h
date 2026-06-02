#pragma once

namespace XEngine
{
    class Entity
    {
    public:
        explicit Entity(unsigned int id = 0) : m_Id(id) {}
        unsigned int GetId() const { return m_Id; }

    private:
        unsigned int m_Id = 0;
    };
}



