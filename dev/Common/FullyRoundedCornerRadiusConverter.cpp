// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "FullyRoundedCornerRadiusConverter.h"


winrt::IInspectable FullyRoundedCornerRadiusConverter::Convert(
    winrt::IInspectable const& value,
    winrt::TypeName const& targetType,
    winrt::IInspectable const& parameter,
    winrt::hstring const& language)
{
    auto const height = unbox_value<double>(value);
    if (!isnan(height))
    {
        auto const cornerRadius = height / 2;
        if (targetType.Name == L"Object")
        {
            return box_value(winrt::CornerRadius{ cornerRadius, cornerRadius, cornerRadius, cornerRadius });
        }
        else if (targetType.Name == L"Double")
        {
            return box_value(cornerRadius);
        }
    }
    return nullptr;
}

winrt::IInspectable FullyRoundedCornerRadiusConverter::ConvertBack(
    winrt::IInspectable const& value,
    winrt::TypeName const& targetType,
    winrt::IInspectable const& parameter,
    winrt::hstring const& language)
{
    winrt::throw_hresult(E_NOTIMPL);
}
