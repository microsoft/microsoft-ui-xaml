// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "CornerRadiusFilterConverter.h"

using FilterKind = winrt::CornerRadiusFilterKind;

winrt::CornerRadius CornerRadiusFilterConverter::Convert(winrt::CornerRadius const& radius, winrt::CornerRadiusFilterKind const& filterKind)
{
    winrt::CornerRadius result { 0,0,0,0 };

    if ((filterKind & FilterKind::TopLeft) == FilterKind::TopLeft)
    {
        result.TopLeft = radius.TopLeft;
    }

    if ((filterKind & FilterKind::TopRight) == FilterKind::TopRight)
    {
        result.TopRight = radius.TopRight;
    }

    if ((filterKind & FilterKind::BottomLeft) == FilterKind::BottomLeft)
    {
        result.BottomLeft = radius.BottomLeft;
    }

    if ((filterKind & FilterKind::BottomRight) == FilterKind::BottomRight)
    {
        result.BottomRight = radius.BottomRight;
    }

    return result;
}

double CornerRadiusFilterConverter::GetDoubleValue(winrt::CornerRadius const& radius, winrt::CornerRadiusFilterKind const& filterKind)
{
    switch (filterKind)
    {
    case FilterKind::TopLeft:
        return radius.TopLeft;

    case FilterKind::TopRight:
        return radius.TopRight;

    case FilterKind::BottomLeft:
        return radius.BottomLeft;

    case FilterKind::BottomRight:
        return radius.BottomRight;
    }
    return 0;
}

winrt::IInspectable CornerRadiusFilterConverter::Convert(
    winrt::IInspectable const& value,
    winrt::TypeName const& targetType,
    winrt::IInspectable const& parameter,
    winrt::hstring const& language)
{
    auto cornerRadius = unbox_value<winrt::CornerRadius>(value);
    auto filterType = Filter();
    if ((filterType & FilterKind::ValueOnly) == FilterKind::ValueOnly)
    {
        return box_value(GetDoubleValue(cornerRadius, Filter()));
    }

    return box_value(Convert(cornerRadius, Filter()));
}

winrt::IInspectable CornerRadiusFilterConverter::ConvertBack(
    winrt::IInspectable const& value,
    winrt::TypeName const& targetType,
    winrt::IInspectable const& parameter,
    winrt::hstring const& language)
{
    winrt::throw_hresult(E_NOTIMPL);
}
