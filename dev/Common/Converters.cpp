// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "Converters.h"
#include <common.h>
#include <pch.h>

CppWinRTActivatableClassWithBasicFactory(CornerRadiusFilterConverter)

CornerRadiusFilterConverter::CornerRadiusFilterConverter()
= default;

winrt::CornerRadius CornerRadiusFilterConverter::Convert(winrt::CornerRadius const& radius, FilterType const& filter)
{
    winrt::CornerRadius result = radius;

    switch (filter)
    {
    case FilterType::Top:
        result.BottomLeft = 0;
        result.BottomRight = 0;
        break;
    case FilterType::Right:
        result.TopLeft = 0;
        result.BottomLeft = 0;
        break;
    case FilterType::Bottom:
        result.TopLeft = 0;
        result.TopRight = 0;
        break;
    case FilterType::Left:
        result.TopRight = 0;
        result.BottomRight = 0;
        break;
    }

    return result;
}

winrt::IInspectable CornerRadiusFilterConverter::Convert(
    winrt::IInspectable const& value,
    winrt::TypeName const&  /*targetType*/,
    winrt::IInspectable const& parameter,
    winrt::hstring const&  /*language*/)
{
    auto radius = unbox_value<winrt::CornerRadius>(value);
    auto filter = unbox_value<winrt::hstring>(parameter);
    FilterType filterType;
    if (filter == L"Top")
    {
        filterType = FilterType::Top;
    }
    else if (filter == L"Right")
    {
        filterType = FilterType::Right;
    }
    else if (filter == L"Bottom")
    {
        filterType = FilterType::Bottom;
    }
    else if (filter == L"Left")
    {
        filterType = FilterType::Left;
    }
    else
    {
        winrt::throw_hresult(OSS_BAD_ARG);
    }
    auto result = Convert(radius, filterType);
    return box_value(result);
}

winrt::IInspectable CornerRadiusFilterConverter::ConvertBack(
    winrt::IInspectable const&  /*value*/,
    winrt::TypeName const&  /*targetType*/,
    winrt::IInspectable const&  /*parameter*/,
    winrt::hstring const&  /*language*/)
{
    winrt::throw_hresult(E_NOTIMPL);
}
