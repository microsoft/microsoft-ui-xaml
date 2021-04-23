// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "FullyRoundedCornerRadiusConverter.g.h"
#include "FullyRoundedCornerRadiusConverter.properties.h"

class FullyRoundedCornerRadiusConverter :
    public winrt::implementation::FullyRoundedCornerRadiusConverterT<FullyRoundedCornerRadiusConverter>,
    public FullyRoundedCornerRadiusConverterProperties
{
public:
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
};
