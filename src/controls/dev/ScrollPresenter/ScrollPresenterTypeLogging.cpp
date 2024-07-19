﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ScrollPresenterTypeLogging.h"
#include "Utils.h"

namespace TypeLogging
{

winrt::hstring ScrollBarVisibilityToString(const winrt::ScrollingScrollBarVisibility& scrollBarVisibility)
{
    switch (scrollBarVisibility)
    {
    case winrt::ScrollingScrollBarVisibility::Visible:
        return L"Visible";
    case winrt::ScrollingScrollBarVisibility::Hidden:
        return L"Hidden";
    case winrt::ScrollingScrollBarVisibility::Auto:
        return L"Auto";
    default:
        MUX_ASSERT(false);
        return L"";
    }
}

winrt::hstring ChainingModeToString(const winrt::ScrollingChainMode& chainingMode)
{
    switch (chainingMode)
    {
    case winrt::ScrollingChainMode::Always:
        return L"Always";
    case winrt::ScrollingChainMode::Auto:
        return L"Auto";
    case winrt::ScrollingChainMode::Never:
        return L"Never";
    default:
        MUX_ASSERT(false);
        return L"";
    }
}

winrt::hstring RailingModeToString(const winrt::ScrollingRailMode& railingMode)
{
    switch (railingMode)
    {
    case winrt::ScrollingRailMode::Disabled:
        return L"Disabled";
    case winrt::ScrollingRailMode::Enabled:
        return L"Enabled";
    default:
        MUX_ASSERT(false);
        return L"";
    }
}

winrt::hstring ScrollModeToString(const winrt::ScrollingScrollMode& scrollMode)
{
    switch (scrollMode)
    {
    case winrt::ScrollingScrollMode::Disabled:
        return L"Disabled";
    case winrt::ScrollingScrollMode::Enabled:
        return L"Enabled";
    case winrt::ScrollingScrollMode::Auto:
        return L"Auto";
    default:
        MUX_ASSERT(false);
        return L"";
    }
}

winrt::hstring ZoomModeToString(const winrt::ScrollingZoomMode& zoomMode)
{
    switch (zoomMode)
    {
    case winrt::ScrollingZoomMode::Disabled:
        return L"Disabled";
    case winrt::ScrollingZoomMode::Enabled:
        return L"Enabled";
    default:
        MUX_ASSERT(false);
        return L"";
    }
}

winrt::hstring InputKindToString(const winrt::ScrollingInputKinds& inputKind)
{
    switch (static_cast<int>(inputKind))
    {
    case static_cast<int>(winrt::ScrollingInputKinds::None) :
        return L"None";
    case static_cast<int>(winrt::ScrollingInputKinds::All) :
        return L"All";
    case static_cast<int>(winrt::ScrollingInputKinds::Touch):
        return L"Touch";
    case static_cast<int>(winrt::ScrollingInputKinds::Pen):
        return L"Pen";
    case static_cast<int>(winrt::ScrollingInputKinds::Keyboard):
        return L"Keyboard";
    case static_cast<int>(winrt::ScrollingInputKinds::Gamepad):
        return L"Gamepad";
    default:
        return L"InputKind combination";
    }
}

winrt::hstring AnimationModeToString(const winrt::ScrollingAnimationMode& animationMode)
{
    switch (animationMode)
    {
    case winrt::ScrollingAnimationMode::Disabled:
        return L"Disabled";
    case winrt::ScrollingAnimationMode::Enabled:
        return L"Enabled";
    case winrt::ScrollingAnimationMode::Auto:
        return L"Auto";
    default:
        MUX_ASSERT(false);
        return L"";
    }
}

winrt::hstring SnapPointsModeToString(const winrt::ScrollingSnapPointsMode& snapPointsMode)
{
    switch (snapPointsMode)
    {
    case winrt::ScrollingSnapPointsMode::Default:
        return L"Default";
    case winrt::ScrollingSnapPointsMode::Ignore:
        return L"Ignore";
    default:
        MUX_ASSERT(false);
        return L"";
    }
}

winrt::hstring ScrollPresenterViewKindToString(ScrollPresenterViewKind viewKind)
{
    switch (viewKind)
    {
    case ScrollPresenterViewKind::Absolute:
        return L"Absolute";
    case ScrollPresenterViewKind::RelativeToCurrentView:
        return L"RelativeToCurrentView";
#ifdef ScrollPresenterViewKind_RelativeToEndOfInertiaView
    case ScrollPresenterViewKind::RelativeToEndOfInertiaView:
        return L"RelativeToEndOfInertiaView";
#endif
    default:
        MUX_ASSERT(false);
        return L"";
    }
}

winrt::hstring ScrollPresenterViewChangeResultToString(ScrollPresenterViewChangeResult result)
{
    switch (result)
    {
    case ScrollPresenterViewChangeResult::Completed:
        return L"Completed";
    case ScrollPresenterViewChangeResult::Ignored:
        return L"Ignored";
    case ScrollPresenterViewChangeResult::Interrupted:
        return L"Interrupted";
    default:
        MUX_ASSERT(false);
        return L"";
    }
}

winrt::hstring ScrollAmountToString(const winrt::ScrollAmount& scrollAmount)
{
    switch (scrollAmount)
    {
    case winrt::ScrollAmount::LargeDecrement:
        return L"LargeDecrement";
    case winrt::ScrollAmount::LargeIncrement:
        return L"LargeIncrement";
    case winrt::ScrollAmount::SmallDecrement:
        return L"SmallDecrement";
    case winrt::ScrollAmount::SmallIncrement:
        return L"SmallIncrement";
    case winrt::ScrollAmount::NoAmount:
        return L"NoAmount";
    default:
        MUX_ASSERT(false);
        return L"";
    }
}

winrt::hstring ScrollOptionsToString(const winrt::ScrollingScrollOptions& options)
{
    if (options)
    {
        return StringUtil::FormatString(L"ScrollingScrollOptions[0x%1!p!]: AnimationMode: %2!s!, SnapPointsMode: %3!s!",
            options,
            AnimationModeToString(options.AnimationMode()).c_str(),
            SnapPointsModeToString(options.SnapPointsMode()).c_str());
    }
    else
    {
        return L"ScrollingScrollOptions[null]";
    }
}

winrt::hstring ZoomOptionsToString(const winrt::ScrollingZoomOptions& options)
{
    if (options)
    {
        return StringUtil::FormatString(L"ScrollingZoomOptions[0x%1!p!]: AnimationMode: %2!s!, SnapPointsMode: %3!s!",
            options,
            AnimationModeToString(options.AnimationMode()).c_str(),
            SnapPointsModeToString(options.SnapPointsMode()).c_str());
    }
    else
    {
        return L"ScrollingZoomOptions[null]";
    }
}

winrt::hstring InteractionTrackerAsyncOperationTypeToString(InteractionTrackerAsyncOperationType operationType)
{
    switch (operationType)
    {
    case InteractionTrackerAsyncOperationType::None:
        return L"None";
    case InteractionTrackerAsyncOperationType::TryUpdatePosition:
        return L"TryUpdatePosition";
    case InteractionTrackerAsyncOperationType::TryUpdatePositionBy:
        return L"TryUpdatePositionBy";
    case InteractionTrackerAsyncOperationType::TryUpdatePositionWithAdditionalVelocity:
        return L"TryUpdatePositionWithAdditionalVelocity";
    case InteractionTrackerAsyncOperationType::TryUpdatePositionWithAnimation:
        return L"TryUpdatePositionWithAnimation";
    case InteractionTrackerAsyncOperationType::TryUpdateScale:
        return L"TryUpdateScale";
    case InteractionTrackerAsyncOperationType::TryUpdateScaleWithAdditionalVelocity:
        return L"TryUpdateScaleWithAdditionalVelocity";
    case InteractionTrackerAsyncOperationType::TryUpdateScaleWithAnimation:
        return L"TryUpdateScaleWithAnimation";
    default:
        MUX_ASSERT(false);
        return L"";
    }
}

winrt::hstring InteractionTrackerAsyncOperationTriggerToString(InteractionTrackerAsyncOperationTrigger operationTrigger)
{
    switch (operationTrigger)
    {
    case InteractionTrackerAsyncOperationTrigger::DirectViewChange:
        return L"DirectViewChange";
    case InteractionTrackerAsyncOperationTrigger::HorizontalScrollControllerRequest:
        return L"HorizontalScrollControllerRequest";
    case InteractionTrackerAsyncOperationTrigger::VerticalScrollControllerRequest:
        return L"VerticalScrollControllerRequest";
    case InteractionTrackerAsyncOperationTrigger::BringIntoViewRequest:
        return L"BringIntoViewRequest";
    default:
        MUX_ASSERT(false);
        return L"";
    }
}

}
