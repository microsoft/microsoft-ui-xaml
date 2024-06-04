// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#ifndef _TYPE_BITS_H_
#define _TYPE_BITS_H_

// list of the different kinds of "built-in" type functionality
// recognized by the XAML language system.
enum BoolTypeBits
{
    btbNone                                 = 0x00000000,
    btbString                               = 0x00000001,
    btbConstructible                        = 0x00000002,
    btbCollection                           = 0x00000004,
    btbDictionary                           = 0x00000008,
    btbMarkupExtension                      = 0x00000010,
    btbTemplate                             = 0x00000020,
    btbTrimSurroundingWhitespace            = 0x00000040,
    btbWhitespaceSignificantCollection      = 0x00000080,
    btbIsPublic                             = 0x00000100,
    btbIsDirective                          = 0x00000200,

    // possibly not pure in the sense that this flag
    // doesn't represent an abstract XAML concept, but we don't
    // have two sets of type flags so this will live here.
    btbIsISupportInitialize                 = 0x00000400
};

enum NonBoolTypeValidBits
{
    nbtbNone                        = 0x0000,
    nbtbBaseType                    = 0x0001
};

enum BoolPropertyBits
{
    bpbNone                         = 0x0000,
    bpbIsBrowsable                  = 0x0001,
    bpbIsAmbient                    = 0x0002,
    bpbIsObsolete                   = 0x0004,
    bpbIsPublic                     = 0x0008,
    bpbIsReadOnly                   = 0x0010,
    bpbIsStatic                     = 0x0020,
    bpbIsAttachable                 = 0x0040,
    bpbIsEvent                      = 0x0080,
};

template<typename TEnum>
class XamlBitSet
{
public:
    XamlBitSet()
        : m_Bits(0)
    {
    }

    XamlBitSet(XUINT32 inBits)
        : m_Bits(inBits)
    {
    }

    XamlBitSet& operator=(_In_ const XamlBitSet<TEnum>& other)
    {
        m_Bits = other.m_Bits;
        return *this;
    }

    bool IsBitSet(const TEnum& inBitMask) const
    {
        return (m_Bits & (XUINT32)inBitMask) != 0;
    }

    bool AreBitsSet(const XamlBitSet<TEnum>& inRhs) const
    {
        return (m_Bits & inRhs.m_Bits) == inRhs.m_Bits;
    }

    void SetBit(const TEnum& inBitMask)
    {
        m_Bits |= (XUINT32)inBitMask;
    }

    void SetBits(const XamlBitSet<TEnum>& inRhs)
    {
        m_Bits |= inRhs.m_Bits;
    }

    void ClearBit(const TEnum& inBitMask)
    {
        m_Bits &= ~(XUINT32)inBitMask;
    }

    void ClearBits(const XamlBitSet<TEnum>& inRhs)
    {
        m_Bits &= ~inRhs.m_Bits;
    }

    void SetAllBits()
    {
        m_Bits = ~(XUINT32)0;
    }

    void ClearAllBits()
    {
        m_Bits = 0;
    }

    XamlBitSet<TEnum> And(const XamlBitSet<TEnum>& inRhs) const
    {
        return XamlBitSet<TEnum>(m_Bits & inRhs.m_Bits);
    }

    XamlBitSet<TEnum> Or(const XamlBitSet<TEnum>& inRhs) const
    {
        return XamlBitSet<TEnum>(m_Bits | inRhs.m_Bits);
    }

    bool AreAllZero() const
    {
        return m_Bits == 0;
    }

    operator XUINT32() const
    {
        return m_Bits;
    }

private:
    XUINT32 m_Bits;
};


#endif
