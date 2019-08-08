// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

struct auto_cast
{
    explicit auto_cast(winrt::IInspectable const& value) : m_value(value)
    {
    }

    template <typename T>
    operator T() const
    {
        return m_value.as<T>();
    }

private:
    winrt::IInspectable const& m_value;
};
