// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "SolidColorBrushToColorConverter.g.h"

class SolidColorBrushToColorConverter :
    public winrt::implementation::SolidColorBrushToColorConverterT<SolidColorBrushToColorConverter>
{
public:
    winrt::IInspectable Convert(
        winrt::IInspectable const& value,
        winrt::TypeName const& targetType,
        winrt::IInspectable const& parameter,
        winrt::hstring const& language);

    winrt::IInspectable ConvertBack(
        winrt::IInspectable const& value,
        winrt::TypeName const& targetType,
        winrt::IInspectable const& parameter,
        winrt::hstring const& language);
};
