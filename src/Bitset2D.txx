#pragma once
#include <bitset>

template <size_t R, size_t C>
class Bitset2D
{
    typedef std::bitset<R*C>::reference BitsetReference;

    public:
        Bitset2D() : m_Bitset() { }
        bool operator()(size_t r, size_t c) const
        {
            return m_Bitset[C*r+c];
        }
        BitsetReference operator()(size_t r, size_t c) 
        {
            return m_Bitset[C*r+c];
        }
        Bitset2D<R, C>& reset()
        {
            m_Bitset.reset();
            return *this;
        }
    private:
        std::bitset<R*C> m_Bitset;

};
