// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ViewChange.h"
#include "InteractionTrackerAsyncOperation.h"
#include "Scroller.h"

class TypeLogging
{
public:
#pragma region Common section
    static winrt::hstring PointerPointToString(const winrt::PointerPoint& pointerPoint, bool verbose = false);
    static winrt::hstring RectToString(const winrt::Rect& rect);
    static winrt::hstring Float2ToString(const winrt::float2& v2);
    static winrt::hstring NullableFloatToString(const winrt::IReference<float>& nf);
    static winrt::hstring NullableFloat2ToString(const winrt::IReference<winrt::float2>& nv2);
    static winrt::hstring OrientationToString(const winrt::Orientation& orientation);
    static winrt::hstring ScrollEventTypeToString(const winrt::ScrollEventType& scrollEventType);
    static winrt::hstring ScrollingIndicatorModeToString(const winrt::ScrollingIndicatorMode& indicatorMode);
#pragma endregion

#pragma region ScrollViewer-specific section
#ifndef BUILD_LEAN_MUX_FOR_THE_STORE_APP
static winrt::hstring ScrollBarVisibilityToString(const winrt::ScrollBarVisibility& scrollBarVisibility);
#endif
#pragma endregion

#pragma region Scroller-specific section
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
#pragma endregion
};

