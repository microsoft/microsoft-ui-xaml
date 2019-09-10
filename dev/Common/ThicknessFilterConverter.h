// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "ThicknessFilterConverter.g.h"
#include "ThicknessFilterConverter.properties.h"

class ThicknessFilterConverter :
    public winrt::implementation::ThicknessFilterConverterT<ThicknessFilterConverter>,
    public ThicknessFilterConverterProperties
{
public:
    winrt::Thickness Convert(
        winrt::CornerRadius const& radius,
        winrt::ThicknessFilterConverterKind const& filterKind);

    winrt::IInspectable Convert(
        winrt::IInspectable const& value,
        winrt::TypeName const& targetType,
        winrt::IInspectable const& parameter,
        winrt::hstring const& language);

    winrt::IInspectable ConvertBack(
        winrt::IInspectable const& value,
        winrt::TypeName const& targetType,
        winrt::IInspectable const& parameter,
        winrt::hstring const& language);
};
