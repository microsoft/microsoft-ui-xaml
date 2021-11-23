#pragma once
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
//#include "pch.h"

#include "InfoBadgeTemplatePartHelpers.h"
using namespace ::winrt::Windows::Foundation;

namespace InfoBadgeImpl
{
#pragma region ControlOverrides
    static winrt::Size MeasureOverrideImpl(Size const& defaultDesiredSize)
    {
        if (defaultDesiredSize.Width < defaultDesiredSize.Height)
        {
            return { defaultDesiredSize.Height, defaultDesiredSize.Height };
        }
        return defaultDesiredSize;
    }
#pragma endregion 

#pragma region OnValuePropertyChanged
    static void ValidateValuePropertyImpl(int value);
#pragma endregion 

#pragma region GoToAppropriateDisplayKindState
    static InfoBadgeDisplayKindStates CalculateIconDisplayKindState(bool iconIsFontIcon);
    static InfoBadgeDisplayKindStates CalculateIconOrDotDisplayKindState(bool iconExists, bool iconIsFontIcon);
    static InfoBadgeDisplayKindStates CalculateAppropriateDisplayKindStateImpl(int value, bool iconExists, bool iconIsFontIcon);
#pragma endregion

#pragma region OnSizeChanged
    static std::tuple<double, double, double, double> GetFullyRoundedCornerRadiusValueImpl(double height)
    {
        auto const cornerRadiusValue = height / 2;
        return std::make_tuple(cornerRadiusValue, cornerRadiusValue, cornerRadiusValue, cornerRadiusValue);
    }
#pragma endregion
};
