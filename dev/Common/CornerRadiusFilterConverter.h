// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "CornerRadiusFilterConverter.g.h"
#include "CornerRadiusFilterConverter.properties.h"

class CornerRadiusFilterConverter :
    public winrt::implementation::CornerRadiusFilterConverterT<CornerRadiusFilterConverter>,
    public CornerRadiusFilterConverterProperties
{
public:
    winrt::CornerRadius Convert(
        winrt::CornerRadius const& radius,
        winrt::CornerRadiusFilterKind const& filterKind);

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

private:
    double GetDoubleValue(winrt::CornerRadius const& radius, winrt::CornerRadiusFilterKind const& filterKind);
};
