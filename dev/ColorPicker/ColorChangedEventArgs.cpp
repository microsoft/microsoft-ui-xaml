// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ColorChangedEventArgs.h"
#include "common.h"

winrt::Color ColorChangedEventArgs::OldColor()
{
    return m_oldColor;
}

winrt::Color ColorChangedEventArgs::NewColor()
{
    return m_newColor;
}

void ColorChangedEventArgs::OldColor(winrt::Color const& value)
{
    m_oldColor = value;
}

void ColorChangedEventArgs::NewColor(winrt::Color const& value)
{
    m_newColor = value;
}