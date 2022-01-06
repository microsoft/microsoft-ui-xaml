// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
#include "pch.h"
#include "InfoBadgeImpl.h"
#include "InfoBadgeImpl/RustImpl.h"

namespace InfoBadgeImpl
{
#pragma region ControlOverrides
    winrt::Size InfoBadgeImpl::MeasureOverrideImpl(Size const& defaultDesiredSize)
    {
        if (defaultDesiredSize.Width < defaultDesiredSize.Height)
        {
            return { defaultDesiredSize.Height, defaultDesiredSize.Height };
        }
        return defaultDesiredSize;
    }
#pragma endregion 

#pragma region OnValuePropertyChanged
    void InfoBadgeImpl::ValidateValuePropertyImpl(int value)
    {
        if (value < -1)
        {
            throw winrt::hresult_out_of_bounds(L"Value must be equal to or greater than -1");
        }
    }
#pragma endregion 

#pragma region GoToAppropriateDisplayKindState
    InfoBadgeDisplayKindStates InfoBadgeImpl::CalculateAppropriateDisplayKindStateImpl(int value, bool iconExists, bool iconIsFontIcon)
    {
        if (value >= 0)
        {
            return InfoBadgeDisplayKindStates::Value;
        }

        return CalculateIconOrDotDisplayKindState(iconExists, iconIsFontIcon);
    }

    namespace
    {
        InfoBadgeDisplayKindStates CalculateIconOrDotDisplayKindState(bool iconExists, bool iconIsFontIcon)
        {
            if (!iconExists)
            {
                return InfoBadgeDisplayKindStates::Dot;
            }

            return CalculateIconDisplayKindState(iconIsFontIcon);
        }

        InfoBadgeDisplayKindStates CalculateIconDisplayKindState(bool iconIsFontIcon)
        {
            if (iconIsFontIcon)
            {
                return InfoBadgeDisplayKindStates::FontIcon;
            }

            return InfoBadgeDisplayKindStates::Icon;
        }
    }
#pragma endregion

#pragma region OnSizeChanged
    std::tuple<double, double, double, double> InfoBadgeImpl::GetFullyRoundedCornerRadiusValueImpl(double height)
    {
        auto const CornerRadius = get_fully_rounded_corner_radius_value_rust_impl(height);
        return std::make_tuple(CornerRadius.TopLeft, CornerRadius.TopRight, CornerRadius.BottomLeft, CornerRadius.BottomRight);
    }
#pragma endregion
};
