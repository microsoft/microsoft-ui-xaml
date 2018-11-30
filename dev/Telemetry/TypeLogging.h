// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "InteractionTrackerAsyncOperation.h"

class TypeLogging
{
public:
#pragma region Common section
    static winrt::hstring PointerPointToString(const winrt::PointerPoint& pointerPoint, bool verbose = false);
    static winrt::hstring RectToString(const winrt::Rect& rect);
    static winrt::hstring Float2ToString(const winrt::float2& v2);
    static winrt::hstring OrientationToString(const winrt::Orientation& orientation);
    static winrt::hstring ScrollEventTypeToString(const winrt::ScrollEventType& scrollEventType);
    static winrt::hstring ScrollingIndicatorModeToString(const winrt::ScrollingIndicatorMode& indicatorMode);
#pragma endregion

#pragma region ScrollerView-specific section
#ifndef BUILD_LEAN_MUX_FOR_THE_STORE_APP
    static winrt::hstring ScrollerViewScrollControllerVisibilityToString(const winrt::ScrollerViewScrollControllerVisibility& scrollControllerVisibility);
#endif
#pragma endregion

#pragma region Scroller-specific section
    static winrt::hstring ScrollerChainingModeToString(const winrt::ScrollerChainingMode& chainingMode);
    static winrt::hstring ScrollerRailingModeToString(const winrt::ScrollerRailingMode& railingMode);
    static winrt::hstring ScrollerScrollModeToString(const winrt::ScrollerScrollMode& scrollMode);
    static winrt::hstring ScrollerZoomModeToString(const winrt::ScrollerZoomMode& scrollMode);
    static winrt::hstring ScrollerInputKindToString(const winrt::ScrollerInputKind& inputKind);
    static winrt::hstring ScrollerViewKindToString(const winrt::ScrollerViewKind& offsetKind);
    static winrt::hstring ScrollerViewChangeKindToString(const winrt::ScrollerViewChangeKind& viewChangeKind);
    static winrt::hstring ScrollerViewChangeSnapPointRespectToString(const winrt::ScrollerViewChangeSnapPointRespect& snapPointRespect);
    static winrt::hstring ScrollerViewChangeResultToString(const winrt::ScrollerViewChangeResult& result);
    static winrt::hstring ScrollAmountToString(const winrt::ScrollAmount& scrollAmount);
    static winrt::hstring ScrollerChangeOffsetsOptionsToString(const winrt::ScrollerChangeOffsetsOptions& options);
    static winrt::hstring ScrollerChangeOffsetsWithAdditionalVelocityOptionsToString(const winrt::ScrollerChangeOffsetsWithAdditionalVelocityOptions& options);
    static winrt::hstring ScrollerChangeZoomFactorOptionsToString(const winrt::ScrollerChangeZoomFactorOptions& options);
    static winrt::hstring ScrollerChangeZoomFactorWithAdditionalVelocityOptionsToString(const winrt::ScrollerChangeZoomFactorWithAdditionalVelocityOptions& options);
    static winrt::hstring InteractionTrackerAsyncOperationTypeToString(InteractionTrackerAsyncOperationType operationType);
    static winrt::hstring InteractionTrackerAsyncOperationTriggerToString(InteractionTrackerAsyncOperationTrigger operationTrigger);
#pragma endregion
};

