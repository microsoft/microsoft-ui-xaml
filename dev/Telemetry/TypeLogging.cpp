// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "TypeLogging.h"
#include "Utils.h"

#pragma region Common section

winrt::hstring TypeLogging::PointerPointToString(const winrt::PointerPoint& pointerPoint, bool verbose)
{
    if (verbose)
    {
        return StringUtil::FormatString(L"PointerPoint: PointerId: %1!u!, Position: (%2!u!, %3!u!), IsInContact: %4!u!, PointerDevice: %5!u!",
            pointerPoint.PointerId(), static_cast<uint32_t>(pointerPoint.Position().X), static_cast<uint32_t>(pointerPoint.Position().Y), 
            pointerPoint.IsInContact(), pointerPoint.PointerDevice());
    }
    else
    {
        return StringUtil::FormatString(L"PointerPoint: PointerId: %1!u!, Position: (%2!u!, %3!u!)",
            pointerPoint.PointerId(), static_cast<uint32_t>(pointerPoint.Position().X), static_cast<uint32_t>(pointerPoint.Position().Y));
    }
}

winrt::hstring TypeLogging::RectToString(const winrt::Rect& rect)
{
    return StringUtil::FormatString(L"Rect: X: %1!i!, Y: %2!i!, W: %3!u!, H: %4!u!",
        static_cast<int32_t>(rect.X), static_cast<int32_t>(rect.Y), static_cast<uint32_t>(rect.Width), static_cast<uint32_t>(rect.Height));
}

winrt::hstring TypeLogging::Float2ToString(const winrt::float2& v2)
{
    return StringUtil::FormatString(L"(%1!i!, %2!i!)", static_cast<int32_t>(v2.x), static_cast<int32_t>(v2.y));
}

winrt::hstring TypeLogging::NullableFloatToString(const winrt::IReference<float>& nf)
{
    if (nf)
    {
        return StringUtil::FormatString(L"%1!i!", static_cast<int32_t>(nf.Value()));
    }
    else
    {
        return L"null";
    }
}

winrt::hstring TypeLogging::NullableFloat2ToString(const winrt::IReference<winrt::float2>& nv2)
{
    if (nv2)
    {
        return Float2ToString(nv2.Value());
    }
    else
    {
        return L"null";
    }
}

winrt::hstring TypeLogging::OrientationToString(const winrt::Orientation& orientation)
{
    return orientation == winrt::Orientation::Horizontal ? L"Horizontal" : L"Vertical";
}

winrt::hstring TypeLogging::ScrollEventTypeToString(const winrt::ScrollEventType& scrollEventType)
{
    switch (scrollEventType)
    {
    case winrt::ScrollEventType::First:
        return L"First";
    case winrt::ScrollEventType::Last:
        return L"Last";
    case winrt::ScrollEventType::SmallDecrement:
        return L"SmallDecrement";
    case winrt::ScrollEventType::SmallIncrement:
        return L"SmallIncrement";
    case winrt::ScrollEventType::LargeDecrement:
        return L"LargeDecrement";
    case winrt::ScrollEventType::LargeIncrement:
        return L"LargeIncrement";
    case winrt::ScrollEventType::ThumbPosition:
        return L"ThumbPosition";
    case winrt::ScrollEventType::ThumbTrack:
        return L"ThumbTrack";
    case winrt::ScrollEventType::EndScroll:
        return L"EndScroll";
    default:
        MUX_ASSERT(false);
        return L"";
    }
}

winrt::hstring TypeLogging::ScrollingIndicatorModeToString(const winrt::ScrollingIndicatorMode& indicatorMode)
{
    switch (indicatorMode)
    {
    case winrt::ScrollingIndicatorMode::None:
        return L"None";
    case winrt::ScrollingIndicatorMode::MouseIndicator:
        return L"MouseIndicator";
    case winrt::ScrollingIndicatorMode::TouchIndicator:
        return L"TouchIndicator";
    default:
        MUX_ASSERT(false);
        return L"";
    }
}

#pragma endregion

#pragma region ScrollViewer-specific section

#ifndef BUILD_LEAN_MUX_FOR_THE_STORE_APP
#ifndef BUILD_WINDOWS
winrt::hstring TypeLogging::ScrollBarVisibilityToString(const winrt::ScrollBarVisibility& scrollBarVisibility)
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
#endif
#endif

#pragma endregion

#pragma region Scroller-specific section

winrt::hstring TypeLogging::ChainingModeToString(const winrt::ChainingMode& chainingMode)
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

winrt::hstring TypeLogging::RailingModeToString(const winrt::RailingMode& railingMode)
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

winrt::hstring TypeLogging::ScrollModeToString(const winrt::ScrollMode& scrollMode)
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

winrt::hstring TypeLogging::ZoomModeToString(const winrt::ZoomMode& zoomMode)
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

winrt::hstring TypeLogging::InputKindToString(const winrt::InputKind& inputKind)
{
    switch (static_cast<int>(inputKind))
    {
    case static_cast<int>(winrt::InputKind::All):
        return L"All";
    case static_cast<int>(winrt::InputKind::Touch):
        return L"Touch";
    case static_cast<int>(winrt::InputKind::Pen):
        return L"Pen";
    case static_cast<int>(winrt::InputKind::Touch | winrt::InputKind::MouseWheel):
        return L"Touch|MouseWheel";
    case static_cast<int>(winrt::InputKind::Touch | winrt::InputKind::Pen):
        return L"Touch|Pen";
    case static_cast<int>(winrt::InputKind::Pen | winrt::InputKind::MouseWheel) :
        return L"Pen|MouseWheel";
    case static_cast<int>(winrt::InputKind::Touch | winrt::InputKind::Pen | winrt::InputKind::MouseWheel) :
        return L"Touch|Pen|MouseWheel";
    default:
        MUX_ASSERT(false);
        return L"";
    }
}

winrt::hstring TypeLogging::AnimationModeToString(const winrt::AnimationMode& animationMode)
{
    switch (animationMode)
    {
    case winrt::AnimationMode::Disabled:
        return L"Disabled";
    case winrt::AnimationMode::Enabled:
        return L"Enabled";
    default:
        MUX_ASSERT(false);
        return L"";
    }
}

winrt::hstring TypeLogging::SnapPointsModeToString(const winrt::SnapPointsMode& snapPointsMode)
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

winrt::hstring TypeLogging::ScrollerViewKindToString(const winrt::ScrollerViewKind& viewKind)
{
    switch (viewKind)
    {
    case winrt::ScrollerViewKind::Absolute:
        return L"Absolute";
    case winrt::ScrollerViewKind::RelativeToCurrentView:
        return L"RelativeToCurrentView";
    case winrt::ScrollerViewKind::RelativeToEndOfInertiaView:
        return L"RelativeToEndOfInertiaView";
    default:
        MUX_ASSERT(false);
        return L"";
    }
}

winrt::hstring TypeLogging::ScrollerViewChangeKindToString(const winrt::ScrollerViewChangeKind& viewChangeKind)
{
    switch (viewChangeKind)
    {
    case winrt::ScrollerViewChangeKind::AllowAnimation:
        return L"AllowAnimation";
    case winrt::ScrollerViewChangeKind::DisableAnimation:
        return L"DisableAnimation";
    default:
        MUX_ASSERT(false);
        return L"";
    }
}

winrt::hstring TypeLogging::ScrollerViewChangeSnapPointRespectToString(const winrt::ScrollerViewChangeSnapPointRespect& snapPointRespect)
{
    switch (snapPointRespect)
    {
    case winrt::ScrollerViewChangeSnapPointRespect::IgnoreSnapPoints:
        return L"IgnoreSnapPoints";
    case winrt::ScrollerViewChangeSnapPointRespect::RespectSnapPoints:
        return L"RespectSnapPoints";
    default:
        assert(false);
        return L"";
    }
}

winrt::hstring TypeLogging::ScrollerViewChangeResultToString(const winrt::ScrollerViewChangeResult& result)
{
    switch (result)
    {
    case winrt::ScrollerViewChangeResult::Completed:
        return L"Completed";
    case winrt::ScrollerViewChangeResult::Ignored:
        return L"Ignored";
    case winrt::ScrollerViewChangeResult::Interrupted:
        return L"Interrupted";
    default:
        MUX_ASSERT(false);
        return L"";
    }
}

winrt::hstring TypeLogging::ScrollAmountToString(const winrt::ScrollAmount& scrollAmount)
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

winrt::hstring TypeLogging::ScrollOptionsToString(const winrt::ScrollOptions& options)
{
    return StringUtil::FormatString(L"ScrollOptions[0x%1!p!]: AnimationMode: %2!s!, SnapPointsMode: %3!s!",
        options,
        AnimationModeToString(options.AnimationMode()).c_str(),
        SnapPointsModeToString(options.SnapPointsMode()).c_str());
}

winrt::hstring TypeLogging::ZoomOptionsToString(const winrt::ZoomOptions& options)
{
    return StringUtil::FormatString(L"ZoomOptions[0x%1!p!]: AnimationMode: %2!s!, SnapPointsMode: %3!s!",
        options,
        AnimationModeToString(options.AnimationMode()).c_str(),
        SnapPointsModeToString(options.SnapPointsMode()).c_str());
}

winrt::hstring TypeLogging::ScrollerChangeOffsetsOptionsToString(const winrt::ScrollerChangeOffsetsOptions& options)
{
    return StringUtil::FormatString(L"ScrollerChangeOffsetsOptions[0x%1!p!]: HorizontalOffset: %2!i!, VerticalOffset: %3!i!, OffsetsKind: %4!s!, ViewChangeKind: %5!s!, SnapPointRespect: %6!s!",
        options,
        static_cast<int32_t>(options.HorizontalOffset()),
        static_cast<int32_t>(options.VerticalOffset()),
        ScrollerViewKindToString(options.OffsetsKind()).c_str(),
        ScrollerViewChangeKindToString(options.ViewChangeKind()).c_str(),
        ScrollerViewChangeSnapPointRespectToString(options.SnapPointRespect()).c_str());
}

winrt::hstring TypeLogging::ScrollerChangeOffsetsWithAdditionalVelocityOptionsToString(const winrt::ScrollerChangeOffsetsWithAdditionalVelocityOptions& options)
{
    return StringUtil::FormatString(L"ScrollerChangeOffsetsWithAdditionalVelocityOptions[0x%1!p!]: AdditionalVelocity: (%2!i!, %3!i!), 1000*InertiaDecayRate: (%4!i!, %5!i!)",
        options,
        static_cast<int32_t>(options.AdditionalVelocity().x),
        static_cast<int32_t>(options.AdditionalVelocity().y),
        static_cast<int32_t>(options.InertiaDecayRate() ? 1000.0f * options.InertiaDecayRate().Value().x : -1.0f),
        static_cast<int32_t>(options.InertiaDecayRate() ? 1000.0f * options.InertiaDecayRate().Value().y : -1.0f));
}

winrt::hstring TypeLogging::ScrollerChangeZoomFactorOptionsToString(const winrt::ScrollerChangeZoomFactorOptions& options)
{
    return StringUtil::FormatString(L"ScrollerChangeZoomFactorOptions[0x%1!p!]: 1000*ZoomFactor: %2!u!, CenterPoint: %3!s!, ZoomFactorKind: %4!s!, ViewChangeKind: %5!s!",
        options,
        static_cast<uint32_t>(options.ZoomFactor() * 1000.0f),
        Float2ToString(options.CenterPoint()).c_str(),
        ScrollerViewKindToString(options.ZoomFactorKind()).c_str(),
        ScrollerViewChangeKindToString(options.ViewChangeKind()).c_str());
}

winrt::hstring TypeLogging::ScrollerChangeZoomFactorWithAdditionalVelocityOptionsToString(const winrt::ScrollerChangeZoomFactorWithAdditionalVelocityOptions& options)
{
    return StringUtil::FormatString(L"ScrollerChangeZoomFactorWithAdditionalVelocityOptions[0x%1!p!]: AdditionalVelocity: %2!i!, 1000*InertiaDecayRate: %3!i!, CenterPoint: %4!s!",
        options,
        static_cast<int32_t>(options.AdditionalVelocity()),
        static_cast<int32_t>(options.InertiaDecayRate() ? 1000.0f * options.InertiaDecayRate().Value() : -1.0f),
        Float2ToString(options.CenterPoint()).c_str());
}

winrt::hstring TypeLogging::InteractionTrackerAsyncOperationTypeToString(InteractionTrackerAsyncOperationType operationType)
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

winrt::hstring TypeLogging::InteractionTrackerAsyncOperationTriggerToString(InteractionTrackerAsyncOperationTrigger operationTrigger)
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
#pragma endregion
