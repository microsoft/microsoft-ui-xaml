// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <memory>
#include "sp_ids.h"
#include <SimpleProperty.h>

template <typename T>
struct BigStruct
{
    constexpr BigStruct()
    {
    }

    constexpr BigStruct(T value)
        : m_value(value)
    {
    }

    BigStruct(BigStruct&& from) noexcept
    {
        m_value = from.m_value;
        from.m_value = -1;
    }

    constexpr BigStruct(const BigStruct& from)
    {
        m_value = from.m_value;
    }

    BigStruct& operator=(BigStruct&& from) noexcept
    {
        if (this != &from)
        {
            m_value = from.m_value;
            from.m_value = -1;
        }
        return *this;
    }

    BigStruct& operator=(const BigStruct& from)
    {
        if (this != &from)
        {
            m_value = from.m_value;
        }
        return *this;
    }

    T m_value = (T)65;
};

template <typename T>
inline bool operator==(const BigStruct<T>& rhs, const BigStruct<T>& lhs)
{
    return rhs.m_value == lhs.m_value;
}

template <typename T>
inline bool operator!=(const BigStruct<T>& rhs, const BigStruct<T>& lhs)
{
    return rhs.m_value != lhs.m_value;
}

class GroupStorage
{
    int m_valueInField = 23;
    std::shared_ptr<BigStruct<int>> m_smartInField;
    BigStruct<int> m_bigImmutableInField;
    BigStruct<char> m_bigMutableInField;

public:
    int Get_valueInField() const
    {
        return m_valueInField;
    }

    void Set_valueInField(int value)
    {
        m_valueInField = value;
    }

    const std::shared_ptr<BigStruct<int>>& Get_smartInField() const
    {
        return m_smartInField;
    }

    void Set_smartInField(const std::shared_ptr<BigStruct<int>>& value)
    {
        m_smartInField = value;
    }

    const BigStruct<int>& Get_bigImmutableInField() const
    {
        return m_bigImmutableInField;
    }

    BigStruct<int> Set_bigImmutableInField(BigStruct<int>&& value)
    {
        auto old = std::move(m_bigImmutableInField);
        m_bigImmutableInField = std::move(value);
        return old;
    }

    BigStruct<char>& Get_bigMutableInField() const
    {
        return const_cast<GroupStorage*>(this)->m_bigMutableInField;
    }

    BigStruct<char> Set_bigMutableInField(BigStruct<char>&& value)
    {
        auto old = std::move(m_bigMutableInField);
        m_bigMutableInField = std::move(value);
        return old;
    }
};

class SomeClass
{
    int m_valueInField = 0;
    std::shared_ptr<BigStruct<int>> m_smartInField;
    BigStruct<int> m_bigImmutableInField;
    BigStruct<char> m_bigMutableInField;

    mutable SimpleProperty::propertyid_numeric_t m_used = 0;

public:
    ~SomeClass();

    int Get_valueInField() const
    {
        return m_valueInField;
    }

    void Set_valueInField(int value)
    {
        m_valueInField = value;
    }

    const std::shared_ptr<BigStruct<int>>& Get_smartInField() const
    {
        return m_smartInField;
    }

    void Set_smartInField(const std::shared_ptr<BigStruct<int>>& value)
    {
        m_smartInField = value;
    }

    const BigStruct<int>& Get_bigImmutableInField() const
    {
        return m_bigImmutableInField;
    }

    BigStruct<int> Set_bigImmutableInField(BigStruct<int>&& value)
    {
        auto old = std::move(m_bigImmutableInField);
        m_bigImmutableInField = std::move(value);
        return old;
    }

    BigStruct<char>& Get_bigMutableInField() const
    {
        return const_cast<SomeClass*>(this)->m_bigMutableInField;
    }

    BigStruct<char> Set_bigMutableInField(BigStruct<char>&& value)
    {
        auto old = std::move(m_bigMutableInField);
        m_bigMutableInField = std::move(value);
        return old;
    }

    bool IsSimpleSparseSet(SimpleProperty::PID pid) const
    {
        return (m_used & (1 << static_cast<SimpleProperty::propertyid_numeric_t>(pid))) != 0;
    }

    void SetSimpleSparseFlag(SimpleProperty::PID pid, bool value)
    {
        m_used &= ~(1 << static_cast<SimpleProperty::propertyid_numeric_t>(pid));
        m_used |= value << static_cast<SimpleProperty::propertyid_numeric_t>(pid);
    }
};