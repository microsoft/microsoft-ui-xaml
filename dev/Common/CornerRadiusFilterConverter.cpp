// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "CornerRadiusFilterConverter.h"

winrt::CornerRadius CornerRadiusFilterConverter::Convert(winrt::CornerRadius const& radius, winrt::CornerRadiusFilterKind const& filterKind)
{
    winrt::CornerRadius result = radius;

    switch (filterKind)
    {
    case winrt::CornerRadiusFilterKind::Top:
        result.BottomLeft = 0;
        result.BottomRight = 0;
        break;
    case winrt::CornerRadiusFilterKind::Right:
        result.TopLeft = 0;
        result.BottomLeft = 0;
        break;
    case winrt::CornerRadiusFilterKind::Bottom:
        result.TopLeft = 0;
        result.TopRight = 0;
        break;
    case winrt::CornerRadiusFilterKind::Left:
        result.TopRight = 0;
        result.BottomRight = 0;
        break;
    }

    return result; 
}

winrt::IInspectable CornerRadiusFilterConverter::Convert(
    winrt::IInspectable const& value,
    winrt::TypeName const& targetType,
    winrt::IInspectable const& parameter,
    winrt::hstring const& language)
{
    auto result = Convert(unbox_value<winrt::CornerRadius>(value), Filter());
    return box_value(result);
}

winrt::IInspectable CornerRadiusFilterConverter::ConvertBack(
    winrt::IInspectable const& value,
    winrt::TypeName const& targetType,
    winrt::IInspectable const& parameter,
    winrt::hstring const& language)
{
    winrt::throw_hresult(E_NOTIMPL);
}
