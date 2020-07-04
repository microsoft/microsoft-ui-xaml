// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "CornerRadiusToFilteredMultipliedThicknessConverter.g.h"
#include "CornerRadiusToFilteredMultipliedThicknessConverter.properties.h"

class CornerRadiusToFilteredMultipliedThicknessConverter :
    public winrt::implementation::CornerRadiusToFilteredMultipliedThicknessConverterT<CornerRadiusToFilteredMultipliedThicknessConverter>,
    public CornerRadiusToFilteredMultipliedThicknessConverterProperties
{
public:
    winrt::Thickness Convert(
        winrt::CornerRadius const& radius,
        winrt::CornerRadiusToThicknessSide const& side,
        double multiplier);

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


