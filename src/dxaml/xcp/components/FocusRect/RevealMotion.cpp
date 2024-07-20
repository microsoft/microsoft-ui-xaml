// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "EnumDefs.g.h"
#include "DoubleUtil.h"
#include "RevealFocusSource.h"
#include "RevealFocusDefaultValue.h"
#include "framework.h"
#include "DoPointerCast.h"

namespace FocusRect { namespace RevealMotion {

    DirectUI::FocusNavigationDirection CalculateFrom(const XRECTF& bounds)
    {
        // If the element is further "below" then it is to the side, we'll have the traveling
        // focus move upwards
        if (std::abs(bounds.Y) >= std::abs(bounds.X))
        {
            if (DirectUI::DoubleUtil::LessThan(bounds.Y, 0.0f))
            {
                return DirectUI::FocusNavigationDirection::Down;
            }
            else if (DirectUI::DoubleUtil::LessThan(0.0f, bounds.Y))
            {
                return DirectUI::FocusNavigationDirection::Up;
            }
        }
        else
        {
            if (DirectUI::DoubleUtil::LessThan(bounds.X, 0.0f))
            {
                return DirectUI::FocusNavigationDirection::Right;
            }
            else if (DirectUI::DoubleUtil::LessThan(0.0f, bounds.X))
            {
                return DirectUI::FocusNavigationDirection::Left;
            }
        }

        // This is acceptable, although probably not expected, it just means focus hasn't actually changed.
        return DirectUI::FocusNavigationDirection::None;
    }

    float GetStartX(DirectUI::FocusNavigationDirection direction, const RevealFocusSource& source)
    {
        auto targetFE = do_pointer_cast<CFrameworkElement>(source.GetTarget());
        const float elementWidth = targetFE ? targetFE->GetActualWidth() : 0.0f;
        const float spotlightStartOffsetFactor = RevealFocus::GetDefaultValue(RevealFocus::DefaultValue::SpotLightStartOffsetFactor);
        switch (direction)
        {
        case DirectUI::FocusNavigationDirection::Up:
        case DirectUI::FocusNavigationDirection::Down:
            return elementWidth/2;
        case DirectUI::FocusNavigationDirection::Left:
            return source.GetSpotLightSize() * spotlightStartOffsetFactor + elementWidth;
        case DirectUI::FocusNavigationDirection::Right:
            return -1*source.GetSpotLightSize() * spotlightStartOffsetFactor;
        case DirectUI::FocusNavigationDirection::None:

        case DirectUI::FocusNavigationDirection::Previous:
        case DirectUI::FocusNavigationDirection::Next:
        default:
            ASSERT(false); // Shouldn't get here. Previous/Next should be replaced with the correct direction
            return 0.0f;
        }
    }

    float GetStartY(DirectUI::FocusNavigationDirection direction, const RevealFocusSource& source)
    {
        auto targetFE = do_pointer_cast<CFrameworkElement>(source.GetTarget());
        const float elementHeight = targetFE ? targetFE->GetActualHeight() : 0.0f;
        const float spotlightStartOffsetFactor = RevealFocus::GetDefaultValue(RevealFocus::DefaultValue::SpotLightStartOffsetFactor);
        switch (direction)
        {
        case DirectUI::FocusNavigationDirection::Left:
        case DirectUI::FocusNavigationDirection::Right:
            return elementHeight/2;
        case DirectUI::FocusNavigationDirection::Up:
            return source.GetSpotLightSize() * spotlightStartOffsetFactor + elementHeight;
        case DirectUI::FocusNavigationDirection::Down:
            return -1*source.GetSpotLightSize() * spotlightStartOffsetFactor;
        case DirectUI::FocusNavigationDirection::None:

        case DirectUI::FocusNavigationDirection::Previous:
        case DirectUI::FocusNavigationDirection::Next:
        default:
            ASSERT(false); // Shouldn't get here. Previous/Next should be replaced with the correct direction
            return 0.0f;
        }
    }

    float GetDistanceX(const RevealFocusSource& source)
    {
        auto targetFE = do_pointer_cast<CFrameworkElement>(source.GetTarget());
        const float elementWidth = targetFE ? targetFE->GetActualWidth() : 0.0f;
        return elementWidth / 2;
    }

    float GetDistanceY(const RevealFocusSource& source)
    {
        auto targetFE = do_pointer_cast<CFrameworkElement>(source.GetTarget());
        const float elementHeight = targetFE ? targetFE->GetActualHeight() : 0.0f;
        return elementHeight / 2;
    }

}}