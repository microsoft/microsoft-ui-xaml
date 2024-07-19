﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "CornerRadiusToThicknessConverter.g.h"
#include "CornerRadiusToThicknessConverter.properties.h"

class CornerRadiusToThicknessConverter :
    public winrt::implementation::CornerRadiusToThicknessConverterT<CornerRadiusToThicknessConverter>,
    public CornerRadiusToThicknessConverterProperties
{
public:
    winrt::Thickness Convert(winrt::CornerRadius const& radius,
        winrt::CornerRadiusToThicknessConverterKind const& filterKind,
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
