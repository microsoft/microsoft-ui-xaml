#pragma once
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
#include "pch.h"

#include "InfoBadgeTemplatePartHelpers.h"
namespace InfoBadgeImpl
{
#pragma region ControlOverrides
    winrt::Size MeasureOverrideImpl(winrt::Size const& defaultDesiredSize)
    {
        if (defaultDesiredSize.Width < defaultDesiredSize.Height)
        {
            return { defaultDesiredSize.Height, defaultDesiredSize.Height };
        }
        return defaultDesiredSize;
    }
#pragma endregion 

#pragma region OnValuePropertyChanged
    void ValidateValuePropertyImpl(int value)
    {
        if (value < -1)
        {
            throw winrt::hresult_out_of_bounds(L"Value must be equal to or greater than -1");
        }
    }
#pragma endregion 

#pragma region GoToAppropriateDisplayKindState
    namespace
    {
        extern InfoBadgeDisplayKindStates CalculateIconDisplayKindState(bool iconIsFontIcon)
        {
            if (iconIsFontIcon)
            {
                return InfoBadgeDisplayKindStates::FontIcon;
            }

            return InfoBadgeDisplayKindStates::Icon;
        }

        extern InfoBadgeDisplayKindStates CalculateIconOrDotDisplayKindState(bool iconExists, bool iconIsFontIcon)
        {
            if (!iconExists)
            {
                return InfoBadgeDisplayKindStates::Dot;
            }

            return CalculateIconDisplayKindState(iconIsFontIcon);
        }
    }

    InfoBadgeDisplayKindStates CalculateAppropriateDisplayKindStateImpl(int value, bool iconExists, bool iconIsFontIcon)
    {
        if (value >= 0)
        {
            return InfoBadgeDisplayKindStates::Value;
        }

        return CalculateIconOrDotDisplayKindState(iconExists, iconIsFontIcon);
    }
#pragma endregion

#pragma region OnSizeChanged
    winrt::CornerRadius GetFullyRoundedCornerRadiusValueImpl(double height)
    {
        auto const cornerRadiusValue = height / 2;
        return winrt::CornerRadius{ cornerRadiusValue, cornerRadiusValue, cornerRadiusValue, cornerRadiusValue };
    }
#pragma endregion
};
