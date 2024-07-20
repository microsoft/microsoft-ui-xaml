// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

///////////////////////////////////////////////////////////////////////////////
//
// unbox
//
///////////////////////////////////////////////////////////////////////////////

#include "CppWinRTHelpers.h"

using winrt::box_value;
using winrt::unbox_value;

struct auto_unbox
{
    explicit auto_unbox(winrt::IInspectable const& value) : m_value(value)
    {
    }

    template <typename T>
    operator T() const
    {
        return unbox_value<T>(m_value);
    }

private:
    winrt::IInspectable const& m_value;
};
