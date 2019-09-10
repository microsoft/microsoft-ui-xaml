// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "ThicknessFilterConverter.h"

winrt::Thickness ThicknessFilterConverter::Convert(winrt::CornerRadius const& radius, winrt::ThicknessFilterConverterKind const& filterKind)
{
    auto result = winrt::Thickness{};

    switch (filterKind)
    {
    case winrt::ThicknessFilterConverterKind::Top:
        result.Left = radius.TopLeft;
        result.Right = radius.TopRight;
        result.Top = 0;
        result.Bottom = 0;
        break;
    case winrt::ThicknessFilterConverterKind::Bottom:
        result.Left = radius.BottomLeft;
        result.Right = radius.BottomRight;
        result.Top = 0;
        result.Bottom = 0;
        break;
    case winrt::ThicknessFilterConverterKind::Left:
        result.Left = 0;
        result.Right = 0;
        result.Top = radius.TopLeft;
        result.Bottom = radius.BottomLeft;
        break;
    case winrt::ThicknessFilterConverterKind::Right:
        result.Left = 0;
        result.Right = 0;
        result.Top = radius.TopRight;
        result.Bottom = radius.BottomRight;
        break;
    }

    return result;
}

winrt::IInspectable ThicknessFilterConverter::Convert(
    winrt::IInspectable const& value,
    winrt::TypeName const& targetType,
    winrt::IInspectable const& parameter,
    winrt::hstring const& language)
{
    auto radius = unbox_value<winrt::CornerRadius>(value);

    return box_value(Convert(radius, Filter()));
}

winrt::IInspectable ThicknessFilterConverter::ConvertBack(
    winrt::IInspectable const& value,
    winrt::TypeName const& targetType,
    winrt::IInspectable const& parameter,
    winrt::hstring const& language)
{
    winrt::throw_hresult(E_NOTIMPL);
}
