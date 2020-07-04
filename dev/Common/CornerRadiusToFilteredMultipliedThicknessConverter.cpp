// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "CornerRadiusToFilteredMultipliedThicknessConverter.h"

winrt::Thickness CornerRadiusToFilteredMultipliedThicknessConverter::Convert(
    winrt::CornerRadius const& radius,
    winrt::CornerRadiusToThicknessSide const& side,
    double multiplier)
{
    auto result = winrt::Thickness{};

    switch (side)
    {
    case winrt::CornerRadiusToThicknessSide::Left:
        result.Left = radius.BottomLeft * multiplier;
        result.Right = 0;
        result.Top = 0;
        result.Bottom = 0;
        break;
    case winrt::CornerRadiusToThicknessSide::Top:
        result.Left = 0;
        result.Top = radius.TopLeft * multiplier;
        result.Right = 0;
        result.Bottom = 0;
        break;
    case winrt::CornerRadiusToThicknessSide::Right:
        result.Left = 0;
        result.Top = 0;
        result.Right = radius.TopRight * multiplier;
        result.Bottom = 0;
        break;
    case winrt::CornerRadiusToThicknessSide::Bottom:
        result.Left = 0;
        result.Right = 0;
        result.Top = 0;
        result.Bottom = radius.BottomRight * multiplier;
        break;
    case winrt::CornerRadiusToThicknessSide::All:
        result.Left = radius.BottomLeft * multiplier;
        result.Top = radius.TopLeft * multiplier;
        result.Right = radius.TopRight * multiplier;
        result.Bottom = radius.BottomRight * multiplier;
        break;
    }

    return result;
}

winrt::IInspectable CornerRadiusToFilteredMultipliedThicknessConverter::Convert(
    winrt::IInspectable const& value,
    winrt::TypeName const& targetType,
    winrt::IInspectable const& parameter,
    winrt::hstring const& language)
{

    auto radius = unbox_value<winrt::CornerRadius>(value);
    return box_value(Convert(radius, Side(), Multiplier()));
}

winrt::IInspectable CornerRadiusToFilteredMultipliedThicknessConverter::ConvertBack(
    winrt::IInspectable const& value,
    winrt::TypeName const& targetType,
    winrt::IInspectable const& parameter,
    winrt::hstring const& language)
{
    winrt::throw_hresult(E_NOTIMPL);
}
