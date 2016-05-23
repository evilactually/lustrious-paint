#pragma once

#include <cstdint>

template <typename BitType, typename MaskType = int>
class Flags
{
public:
    Flags()
    : m_mask(0)
    {
    }

    Flags(BitType bit)
    : m_mask(static_cast<uint32_t>(bit))
    {
    }

    Flags(Flags<BitType> const& rhs)
    : m_mask(rhs.m_mask)
    {
    }

    Flags<BitType> & operator=(Flags<BitType> const& rhs)
    {
        m_mask = rhs.m_mask;
        return *this;
    }

    Flags<BitType> & operator|=(Flags<BitType> const& rhs)
    {
        m_mask |= rhs.m_mask;
        return *this;
    }

    Flags<BitType> & operator&=(Flags<BitType> const& rhs)
    {
        m_mask &= rhs.m_mask;
        return *this;
    }

    Flags<BitType> & operator^=(Flags<BitType> const& rhs)
    {
        m_mask ^= rhs.m_mask;
        return *this;
    }

    Flags<BitType> operator|(Flags<BitType> const& rhs) const
    {
        Flags<BitType> result(*this);
        result |= rhs;
        return result;
    }

    Flags<BitType> operator&(Flags<BitType> const& rhs) const
    {
        Flags<BitType> result(*this);
        result &= rhs;
        return result;
    }

    Flags<BitType> operator^(Flags<BitType> const& rhs) const
    {
        Flags<BitType> result(*this);
        result ^= rhs;
        return result;
    }

    bool operator!() const
    {
        return !m_mask;
    }

    bool operator==(Flags<BitType> const& rhs) const
    {
        return m_mask == rhs.m_mask;
    }

    bool operator!=(Flags<BitType> const& rhs) const
    {
        return m_mask != rhs.m_mask;
    }

    explicit operator bool() const
    {
        return !!m_mask;
    }

    explicit operator MaskType() const
    {
        return m_mask;
    }

private:
    MaskType  m_mask;
};

template <typename BitType>
Flags<BitType> operator|(BitType bit, Flags<BitType> const& flags)
{
    return flags | bit;
}

template <typename BitType>
Flags<BitType> operator&(BitType bit, Flags<BitType> const& flags)
{
    return flags & bit;
}

template <typename BitType>
Flags<BitType> operator^(BitType bit, Flags<BitType> const& flags)
{
    return flags ^ bit;
}

template <typename BitType, typename MaskType>
bool CheckBit(Flags<BitType, MaskType> bits, BitType bit) {
    return (bits & bit) == bit;
}
