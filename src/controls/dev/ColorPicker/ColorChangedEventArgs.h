// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ColorChangedEventArgs.g.h"

class ColorChangedEventArgs :
    public winrt::implementation::ColorChangedEventArgsT<ColorChangedEventArgs>
{
public:
    // IColorChangedEventArgs overrides
    winrt::Color OldColor();
    winrt::Color NewColor();

    void OldColor(winrt::Color const& value);
    void NewColor(winrt::Color const& value);

private:
    winrt::Color m_oldColor{};
    winrt::Color m_newColor{};
};
