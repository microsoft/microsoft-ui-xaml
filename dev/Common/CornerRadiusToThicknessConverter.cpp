// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "CornerRadiusToThicknessConverter.h"

winrt::Thickness CornerRadiusToThicknessConverter::Convert(winrt::CornerRadius const& radius, winrt::CornerRadiusToThicknessConverterKind const& filterKind)
{
    auto result = winrt::Thickness{};

    switch (filterKind)
    {
    case winrt::CornerRadiusToThicknessConverterKind::FilterLeftAndRightFromTop:
        result.Left = radius.TopLeft;
        result.Right = radius.TopRight;
        result.Top = 0;
        result.Bottom = 0;
        break;
    case winrt::CornerRadiusToThicknessConverterKind::FilterLeftAndRightFromBottom:
        result.Left = radius.BottomLeft;
        result.Right = radius.BottomRight;
        result.Top = 0;
        result.Bottom = 0;
        break;
    case winrt::CornerRadiusToThicknessConverterKind::FilterTopAndBottomFromLeft:
        result.Left = 0;
        result.Right = 0;
        result.Top = radius.TopLeft;
        result.Bottom = radius.BottomLeft;
        break;
    case winrt::CornerRadiusToThicknessConverterKind::FilterTopAndBottomFromRight:
        result.Left = 0;
        result.Right = 0;
        result.Top = radius.TopRight;
        result.Bottom = radius.BottomRight;
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

    return box_value(Convert(radius, ConversionKind()));
}

winrt::IInspectable CornerRadiusToThicknessConverter::ConvertBack(
    winrt::IInspectable const& value,
    winrt::TypeName const& targetType,
    winrt::IInspectable const& parameter,
    winrt::hstring const& language)
{
    winrt::throw_hresult(E_NOTIMPL);
}
