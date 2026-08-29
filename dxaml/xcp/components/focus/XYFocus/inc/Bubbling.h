// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <CDependencyObject.h>
#include "enumdefs.g.h"

namespace Focus { namespace XYFocusPrivate {

    CDependencyObject* GetDirectionOverride(
        _In_ CDependencyObject* element,
        _In_opt_ CDependencyObject* searchRoot,
        _In_ DirectUI::FocusNavigationDirection direction,
        _In_ bool ignoreFocusabililty = false);

    CDependencyObject* TryXYFocusBubble(
        _In_ CDependencyObject* element,
        _In_opt_ CDependencyObject* candidate,
        _In_opt_ CDependencyObject* searchRoot,
        _In_ DirectUI::FocusNavigationDirection direction);

    KnownPropertyIndex GetXYFocusPropertyIndex(
        _In_ CDependencyObject* const element,
        _In_ DirectUI::FocusNavigationDirection direction);

    CDependencyObject* GetDirectionOverrideRoot(
        _In_ CDependencyObject* const element,
        _In_opt_ CDependencyObject* searchRoot,
        _In_ const DirectUI::FocusNavigationDirection direction);

    DirectUI::XYFocusNavigationStrategy GetStrategy(
        _In_ CDependencyObject* element,
        _In_ DirectUI::FocusNavigationDirection direction,
        _In_ DirectUI::XYFocusNavigationStrategyOverride navigationStrategyOverride);

    KnownPropertyIndex GetXYFocusNavigationStrategyPropertyIndex(
        _In_ CDependencyObject* const element,
        _In_ DirectUI::FocusNavigationDirection direction);
}}
