// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "CornerRadiusToThicknessConverter.h"

winrt::Thickness CornerRadiusToThicknessConverter::Convert(winrt::CornerRadius const& radius,
    winrt::CornerRadiusToThicknessConverterKind const& filterKind,
    double multiplier)
{
    auto result = winrt::Thickness{};

    switch (filterKind)
    {
    case winrt::CornerRadiusToThicknessConverterKind::FilterLeftAndRightFromTop:
        result.Left = radius.TopLeft * multiplier;
        result.Right = radius.TopRight * multiplier;
        result.Top = 0;
        result.Bottom = 0;
        break;
    case winrt::CornerRadiusToThicknessConverterKind::FilterLeftAndRightFromBottom:
        result.Left = radius.BottomLeft * multiplier;
        result.Right = radius.BottomRight * multiplier;
        result.Top = 0;
        result.Bottom = 0;
        break;
    case winrt::CornerRadiusToThicknessConverterKind::FilterTopAndBottomFromLeft:
        result.Left = 0;
        result.Right = 0;
        result.Top = radius.TopLeft * multiplier;
        result.Bottom = radius.BottomLeft * multiplier;
        break;
    case winrt::CornerRadiusToThicknessConverterKind::FilterTopAndBottomFromRight:
        result.Left = 0;
        result.Right = 0;
        result.Top = radius.TopRight * multiplier;
        result.Bottom = radius.BottomRight * multiplier;
        break;
    case winrt::CornerRadiusToThicknessConverterKind::FilterTopFromTopLeft:
        result.Left = 0;
        result.Right = 0;
        result.Top = radius.TopLeft * multiplier;
        result.Bottom = 0;
        break;
    case winrt::CornerRadiusToThicknessConverterKind::FilterTopFromTopRight:
        result.Left = 0;
        result.Right = 0;
        result.Top = radius.TopRight * multiplier;
        result.Bottom = 0;
        break;
    case winrt::CornerRadiusToThicknessConverterKind::FilterRightFromTopRight:
        result.Left = 0;
        result.Right = radius.TopRight * multiplier;
        result.Top = 0;
        result.Bottom = 0;
        break;
    case winrt::CornerRadiusToThicknessConverterKind::FilterRightFromBottomRight:
        result.Left = 0;
        result.Right = radius.BottomRight * multiplier;
        result.Top = 0;
        result.Bottom = 0;
        break;
    case winrt::CornerRadiusToThicknessConverterKind::FilterBottomFromBottomRight:
        result.Left = 0;
        result.Right = 0;
        result.Top = 0;
        result.Bottom = radius.BottomRight * multiplier;
        break;
    case winrt::CornerRadiusToThicknessConverterKind::FilterBottomFromBottomLeft:
        result.Left = 0;
        result.Right = 0;
        result.Top = 0;
        result.Bottom = radius.BottomLeft * multiplier;
        break;
    case winrt::CornerRadiusToThicknessConverterKind::FilterLeftFromBottomLeft:
        result.Left = radius.BottomLeft * multiplier;
        result.Right = 0;
        result.Top = 0;
        result.Bottom = 0;
        break;
    case winrt::CornerRadiusToThicknessConverterKind::FilterLeftFromTopLeft:
        result.Left = radius.TopLeft * multiplier;
        result.Right = 0;
        result.Top = 0;
        result.Bottom = 0;
        break;
    }

    return result;
}

winrt::IInspectable CornerRadiusToThicknessConverter::Convert(
    winrt::IInspectable const& value,
    winrt::TypeName const& targetType,
    winrt::IInspectable const& parameter,
    winrt::hstring const& language)
{
    auto radius = unbox_value<winrt::CornerRadius>(value);
    const auto multiplier = Multiplier();
    return box_value(Convert(radius, ConversionKind(),multiplier));
}

winrt::IInspectable CornerRadiusToThicknessConverter::ConvertBack(
    winrt::IInspectable const& value,
    winrt::TypeName const& targetType,
    winrt::IInspectable const& parameter,
    winrt::hstring const& language)
{
    winrt::throw_hresult(E_NOTIMPL);

}
