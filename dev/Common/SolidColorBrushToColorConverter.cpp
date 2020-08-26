// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "SolidColorBrushToColorConverter.h"

winrt::IInspectable SolidColorBrushToColorConverter::Convert(
    winrt::IInspectable const& value,
    winrt::TypeName const& targetType,
    winrt::IInspectable const& parameter,
    winrt::hstring const& language)
{
    if (const auto brush = unbox_value<winrt::SolidColorBrush>(value))
    {
        return box_value(brush.Color());
    }
    else
    {
        return nullptr;
    }
}

winrt::IInspectable SolidColorBrushToColorConverter::ConvertBack(
    winrt::IInspectable const& value,
    winrt::TypeName const& targetType,
    winrt::IInspectable const& parameter,
    winrt::hstring const& language)
{
    winrt::throw_hresult(E_NOTIMPL);
}
