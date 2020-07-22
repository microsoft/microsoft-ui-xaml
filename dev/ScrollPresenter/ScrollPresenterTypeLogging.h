// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ViewChange.h"
#include "InteractionTrackerAsyncOperation.h"
#include "ScrollPresenter.h"

namespace TypeLogging
{
    winrt::hstring ScrollEventTypeToString(const winrt::ScrollEventType& scrollEventType);
    winrt::hstring ScrollingIndicatorModeToString(const winrt::ScrollingIndicatorMode& indicatorMode);
    winrt::hstring ScrollBarVisibilityToString(const winrt::ScrollingScrollBarVisibility& scrollBarVisibility);
    winrt::hstring ChainingModeToString(const winrt::ScrollingChainMode& chainingMode);
    winrt::hstring RailingModeToString(const winrt::ScrollingRailMode& railingMode);
    winrt::hstring ScrollModeToString(const winrt::ScrollingScrollMode& scrollMode);
    winrt::hstring ZoomModeToString(const winrt::ScrollingZoomMode& zoomMode);
    winrt::hstring InputKindToString(const winrt::ScrollingInputKinds& inputKind);
    winrt::hstring AnimationModeToString(const winrt::ScrollingAnimationMode& animationMode);
    winrt::hstring SnapPointsModeToString(const winrt::ScrollingSnapPointsMode& snapPointsMode);
    winrt::hstring ScrollPresenterViewKindToString(ScrollPresenterViewKind viewKind);
    winrt::hstring ScrollPresenterViewChangeResultToString(ScrollPresenterViewChangeResult result);
    winrt::hstring ScrollAmountToString(const winrt::ScrollAmount& scrollAmount);
    winrt::hstring ScrollOptionsToString(const winrt::ScrollingScrollOptions& options);
    winrt::hstring ZoomOptionsToString(const winrt::ScrollingZoomOptions& options);
    winrt::hstring InteractionTrackerAsyncOperationTypeToString(InteractionTrackerAsyncOperationType operationType);
    winrt::hstring InteractionTrackerAsyncOperationTriggerToString(InteractionTrackerAsyncOperationTrigger operationTrigger);
};

