// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ViewChange.h"
#include "InteractionTrackerAsyncOperation.h"
#include "Scroller.h"

namespace TypeLogging
{
    static winrt::hstring ScrollEventTypeToString(const winrt::ScrollEventType& scrollEventType);
    static winrt::hstring ScrollingIndicatorModeToString(const winrt::ScrollingIndicatorMode& indicatorMode);
    static winrt::hstring ScrollBarVisibilityToString(const winrt::ScrollBarVisibility& scrollBarVisibility);
    static winrt::hstring ChainingModeToString(const winrt::ChainingMode& chainingMode);
    static winrt::hstring RailingModeToString(const winrt::RailingMode& railingMode);
    static winrt::hstring ScrollModeToString(const winrt::ScrollMode& scrollMode);
    static winrt::hstring ZoomModeToString(const winrt::ZoomMode& zoomMode);
    static winrt::hstring InputKindToString(const winrt::InputKind& inputKind);
    static winrt::hstring AnimationModeToString(const winrt::AnimationMode& animationMode);
    static winrt::hstring SnapPointsModeToString(const winrt::SnapPointsMode& snapPointsMode);
    static winrt::hstring ScrollerViewKindToString(ScrollerViewKind viewKind);
    static winrt::hstring ScrollerViewChangeResultToString(ScrollerViewChangeResult result);
    static winrt::hstring ScrollAmountToString(const winrt::ScrollAmount& scrollAmount);
    static winrt::hstring ScrollOptionsToString(const winrt::ScrollOptions& options);
    static winrt::hstring ZoomOptionsToString(const winrt::ZoomOptions& options);
    static winrt::hstring InteractionTrackerAsyncOperationTypeToString(InteractionTrackerAsyncOperationType operationType);
    static winrt::hstring InteractionTrackerAsyncOperationTriggerToString(InteractionTrackerAsyncOperationTrigger operationTrigger);
};

