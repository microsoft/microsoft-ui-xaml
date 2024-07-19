// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <FocusMovement.h>

#include <CControl.h>
#include <FocusMgr.h>

#include <DXamlTypes.h>
#include <RuntimeEnabledFeatures.h>
#include <DependencyLocator.h>
#include <EventArgs.h>
#include "FocusProperties.h"
#include "FocusedElementRemovedEventArgs.h"
#include "GettingFocusEventArgs.h"
#include "LosingFocusEventArgs.h"
#include "FocusSelection.h"
#include "NoFocusCandidateFoundEventArgs.h"
#include <ErrorHelper.h>
#include <CoreP.h>

using namespace DirectUI;
using namespace Focus;
using namespace RuntimeFeatureBehavior;

FocusMovement::FocusMovement(
    _In_ XYFocusOptions& xyFocusOptions,
    _In_ FocusNavigationDirection direction,
    _In_opt_ CDependencyObject* pTarget)
    : xyFocusOptions(&xyFocusOptions),
    direction(direction),
    pTarget(pTarget)
{
    forceBringIntoView = true;
    UuidCreate(&correlationId);
    focusState = DirectUI::FocusState::Programmatic;
    if (direction == DirectUI::FocusNavigationDirection::Down ||
        direction == DirectUI::FocusNavigationDirection::Left ||
        direction == DirectUI::FocusNavigationDirection::Right ||
        direction == DirectUI::FocusNavigationDirection::Up)
    {
        focusState = DirectUI::FocusState::Keyboard;
    }
}

