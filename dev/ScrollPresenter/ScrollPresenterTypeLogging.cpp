// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ScrollerTypeLogging.h"
#include "Utils.h"

namespace TypeLogging
{

winrt::hstring ScrollBarVisibilityToString(const winrt::ScrollBarVisibility& scrollBarVisibility)
{
    switch (scrollBarVisibility)
    {
    case winrt::ScrollBarVisibility::Visible:
        return L"Visible";
    case winrt::ScrollBarVisibility::Hidden:
        return L"Hidden";
    case winrt::ScrollBarVisibility::Auto:
        return L"Auto";
    default:
        MUX_ASSERT(false);
        return L"";
    }
}

winrt::hstring ChainingModeToString(const winrt::ChainingMode& chainingMode)
{
    switch (chainingMode)
    {
    case winrt::ChainingMode::Always:
        return L"Always";
    case winrt::ChainingMode::Auto:
        return L"Auto";
    case winrt::ChainingMode::Never:
        return L"Never";
    default:
        MUX_ASSERT(false);
        return L"";
    }
}

winrt::hstring RailingModeToString(const winrt::RailingMode& railingMode)
{
    switch (railingMode)
    {
    case winrt::RailingMode::Disabled:
        return L"Disabled";
    case winrt::RailingMode::Enabled:
        return L"Enabled";
    default:
        MUX_ASSERT(false);
        return L"";
    }
}

winrt::hstring ScrollModeToString(const winrt::ScrollMode& scrollMode)
{
    switch (scrollMode)
    {
    case winrt::ScrollMode::Disabled:
        return L"Disabled";
    case winrt::ScrollMode::Enabled:
        return L"Enabled";
#ifdef USE_SCROLLMODE_AUTO
    case winrt::ScrollMode::Auto:
        return L"Auto";
#endif
    default:
        MUX_ASSERT(false);
        return L"";
    }
}

winrt::hstring ZoomModeToString(const winrt::ZoomMode& zoomMode)
{
    switch (zoomMode)
    {
    case winrt::ZoomMode::Disabled:
        return L"Disabled";
    case winrt::ZoomMode::Enabled:
        return L"Enabled";
    default:
        MUX_ASSERT(false);
        return L"";
    }
}

winrt::hstring InputKindToString(const winrt::InputKind& inputKind)
{
    switch (static_cast<int>(inputKind))
    {
    case static_cast<int>(winrt::InputKind::None) :
        return L"None";
    case static_cast<int>(winrt::InputKind::All) :
        return L"All";
    case static_cast<int>(winrt::InputKind::Touch):
        return L"Touch";
    case static_cast<int>(winrt::InputKind::Pen):
        return L"Pen";
    case static_cast<int>(winrt::InputKind::Keyboard):
        return L"Keyboard";
    case static_cast<int>(winrt::InputKind::Gamepad):
        return L"Gamepad";
    default:
        return L"InputKind combination";
    }
}

winrt::hstring AnimationModeToString(const winrt::AnimationMode& animationMode)
{
    switch (animationMode)
    {
    case winrt::AnimationMode::Disabled:
        return L"Disabled";
    case winrt::AnimationMode::Enabled:
        return L"Enabled";
    case winrt::AnimationMode::Auto:
        return L"Auto";
    default:
        MUX_ASSERT(false);
        return L"";
    }
}

winrt::hstring SnapPointsModeToString(const winrt::SnapPointsMode& snapPointsMode)
{
    switch (snapPointsMode)
    {
    case winrt::SnapPointsMode::Default:
        return L"Default";
    case winrt::SnapPointsMode::Ignore:
        return L"Ignore";
    default:
        MUX_ASSERT(false);
        return L"";
    }
}

winrt::hstring ScrollerViewKindToString(ScrollerViewKind viewKind)
{
    switch (viewKind)
    {
    case ScrollerViewKind::Absolute:
        return L"Absolute";
    case ScrollerViewKind::RelativeToCurrentView:
        return L"RelativeToCurrentView";
#ifdef ScrollerViewKind_RelativeToEndOfInertiaView
    case ScrollerViewKind::RelativeToEndOfInertiaView:
        return L"RelativeToEndOfInertiaView";
#endif
    default:
        MUX_ASSERT(false);
        return L"";
    }
}

winrt::hstring ScrollerViewChangeResultToString(ScrollerViewChangeResult result)
{
    switch (result)
    {
    case ScrollerViewChangeResult::Completed:
        return L"Completed";
    case ScrollerViewChangeResult::Ignored:
        return L"Ignored";
    case ScrollerViewChangeResult::Interrupted:
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

winrt::hstring ScrollOptionsToString(const winrt::ScrollOptions& options)
{
    if (options)
    {
        return StringUtil::FormatString(L"ScrollOptions[0x%1!p!]: AnimationMode: %2!s!, SnapPointsMode: %3!s!",
            options,
            AnimationModeToString(options.AnimationMode()).c_str(),
            SnapPointsModeToString(options.SnapPointsMode()).c_str());
    }
    else
    {
        return L"ScrollOptions[null]";
    }
}

winrt::hstring ZoomOptionsToString(const winrt::ZoomOptions& options)
{
    if (options)
    {
        return StringUtil::FormatString(L"ZoomOptions[0x%1!p!]: AnimationMode: %2!s!, SnapPointsMode: %3!s!",
            options,
            AnimationModeToString(options.AnimationMode()).c_str(),
            SnapPointsModeToString(options.SnapPointsMode()).c_str());
    }
    else
    {
        return L"ZoomOptions[null]";
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
    case InteractionTrackerAsyncOperationTrigger::MouseWheel:
        return L"MouseWheel";
    default:
        MUX_ASSERT(false);
        return L"";
    }
}

}
