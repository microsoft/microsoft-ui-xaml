// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "ThicknessFilterConverter.h"

winrt::Thickness ThicknessFilterConverter::Convert(winrt::Thickness const& thickness, winrt::ThicknessFilterKind const& filterKind)
{
    winrt::Thickness result = thickness;

    switch (filterKind)
    {
    case winrt::ThicknessFilterKind::Top:
        result.Top = 0;
        break;
    case winrt::ThicknessFilterKind::Right:
        result.Right = 0;
        break;
    case winrt::ThicknessFilterKind::Bottom:
        result.Bottom = 0;
        break;
    case winrt::ThicknessFilterKind::Left:
        result.Left = 0;
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
    auto thickness = unbox_value<winrt::Thickness>(value);
    const auto filterType = Filter();
    return box_value(Convert(thickness, Filter()));
}

winrt::IInspectable ThicknessFilterConverter::ConvertBack(
    winrt::IInspectable const& value,
    winrt::TypeName const& targetType,
    winrt::IInspectable const& parameter,
    winrt::hstring const& language)
{
    winrt::throw_hresult(E_NOTIMPL);
}
