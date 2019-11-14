// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ViewChange.h"
#include "InteractionTrackerAsyncOperation.h"
#include "Scroller.h"

namespace TypeLogging
{
    winrt::hstring ScrollEventTypeToString(const winrt::ScrollEventType& scrollEventType);
    winrt::hstring ScrollingIndicatorModeToString(const winrt::ScrollingIndicatorMode& indicatorMode);
    winrt::hstring ScrollBarVisibilityToString(const winrt::ScrollBarVisibility& scrollBarVisibility);
    winrt::hstring ChainingModeToString(const winrt::ChainingMode& chainingMode);
    winrt::hstring RailingModeToString(const winrt::RailingMode& railingMode);
    winrt::hstring ScrollModeToString(const winrt::ScrollMode& scrollMode);
    winrt::hstring ZoomModeToString(const winrt::ZoomMode& zoomMode);
    winrt::hstring InputKindToString(const winrt::InputKind& inputKind);
    winrt::hstring AnimationModeToString(const winrt::AnimationMode& animationMode);
    winrt::hstring SnapPointsModeToString(const winrt::SnapPointsMode& snapPointsMode);
    winrt::hstring ScrollerViewKindToString(ScrollerViewKind viewKind);
    winrt::hstring ScrollerViewChangeResultToString(ScrollerViewChangeResult result);
    winrt::hstring ScrollAmountToString(const winrt::ScrollAmount& scrollAmount);
    winrt::hstring ScrollOptionsToString(const winrt::ScrollOptions& options);
    winrt::hstring ZoomOptionsToString(const winrt::ZoomOptions& options);
    winrt::hstring InteractionTrackerAsyncOperationTypeToString(InteractionTrackerAsyncOperationType operationType);
    winrt::hstring InteractionTrackerAsyncOperationTriggerToString(InteractionTrackerAsyncOperationTrigger operationTrigger);
};

