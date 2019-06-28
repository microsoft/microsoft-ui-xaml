// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "Converters.h"

CppWinRTActivatableClassWithBasicFactory(CornerRadiusFilterConverter)

CornerRadiusFilterConverter::CornerRadiusFilterConverter()
{
}

winrt::IInspectable CornerRadiusFilterConverter::Convert(
    winrt::IInspectable const& value,
    winrt::TypeName const& targetType,
    winrt::IInspectable const& parameter,
    winrt::hstring const& language)
{
    auto radius = unbox_value<winrt::CornerRadius>(value);
    auto filter = unbox_value<winrt::hstring>(parameter);

    winrt::CornerRadius result = radius;

    if (filter == L"Top")
    {
        result.BottomLeft = 0;
        result.BottomRight = 0;
    }
    else if (filter == L"Right")
    {
        result.TopLeft = 0;
        result.BottomLeft = 0;
    }
    else if (filter == L"Bottom")
    {
        result.TopLeft = 0;
        result.TopRight = 0;
    }
    else if (filter == L"Left")
    {
        result.TopRight = 0;
        result.BottomRight = 0;
    }

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
