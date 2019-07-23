// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

template <typename T, typename U>
T safe_cast(const U& value)
{
    if (value)
    {
        return value.as<T>();
    }
    return nullptr;
}

template <typename T, typename U>
T safe_try_cast(const U& value)
{
    if (value)
    {
        return value.try_as<T>();
    }
    return nullptr;
}

struct auto_cast
{
    explicit auto_cast(winrt::IInspectable const& value) : m_value(value)
    {
    }

    template <typename T>
    explicit operator T() const
    {
        return safe_cast<T>(m_value);
    }

private:
    winrt::IInspectable const& m_value;
};