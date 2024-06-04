// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "TypeLogging.h"
#include "Utils.h"

namespace TypeLogging
{
#pragma region Common section

winrt::hstring PointerPointToString(const winrt::PointerPoint& pointerPoint, bool verbose)
{
    if (verbose)
    {
        return StringUtil::FormatString(L"PointerPoint: PointerId: %1!u!, Position: (%2!u!, %3!u!), IsInContact: %4!u!, PointerDeviceType: %5!u!",
            pointerPoint.PointerId(), static_cast<uint32_t>(pointerPoint.Position().X), static_cast<uint32_t>(pointerPoint.Position().Y), 
            pointerPoint.IsInContact(), pointerPoint.PointerDeviceType());
    }
    else
    {
        return StringUtil::FormatString(L"PointerPoint: PointerId: %1!u!, Position: (%2!u!, %3!u!)",
            pointerPoint.PointerId(), static_cast<uint32_t>(pointerPoint.Position().X), static_cast<uint32_t>(pointerPoint.Position().Y));
    }
}

winrt::hstring RectToString(const winrt::Rect& rect)
{
    return StringUtil::FormatString(L"Rect: X: %1!i!, Y: %2!i!, W: %3!u!, H: %4!u!",
        static_cast<int32_t>(rect.X), static_cast<int32_t>(rect.Y), static_cast<uint32_t>(rect.Width), static_cast<uint32_t>(rect.Height));
}

winrt::hstring SizeToString(const winrt::Size& size)
{
    return StringUtil::FormatString(L"Size: W: %1!u!, H: %2!u!",
        static_cast<uint32_t>(size.Width), static_cast<uint32_t>(size.Height));
}

winrt::hstring Float2ToString(const winrt::float2& v2)
{
    return StringUtil::FormatString(L"(%1!i!, %2!i!)", static_cast<int32_t>(v2.x), static_cast<int32_t>(v2.y));
}

winrt::hstring NullableFloatToString(const winrt::IReference<float>& nf)
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

winrt::hstring NullableFloat2ToString(const winrt::IReference<winrt::float2>& nv2)
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

winrt::hstring OrientationToString(const winrt::Orientation& orientation)
{
    return orientation == winrt::Orientation::Horizontal ? L"Horizontal" : L"Vertical";
}

winrt::hstring PointerDeviceTypeToString(const winrt::PointerDeviceType& pointerDeviceKind)
{
    switch (pointerDeviceKind)
    {
    case winrt::PointerDeviceType::Touch:
        return L"Touch";
    case winrt::PointerDeviceType::Pen:
        return L"Pen";
    case winrt::PointerDeviceType::Mouse:
        return L"Mouse";
    default:
        MUX_ASSERT(false);
        return L"";
    }
}

winrt::hstring PointerUpdateKindToString(const winrt::PointerUpdateKind& pointerUpdateKind)
{
    switch (pointerUpdateKind)
    {
    case winrt::PointerUpdateKind::LeftButtonPressed:
        return L"LeftButtonPressed";
    case winrt::PointerUpdateKind::LeftButtonReleased:
        return L"LeftButtonReleased";
    case winrt::PointerUpdateKind::MiddleButtonPressed:
        return L"MiddleButtonPressed";
    case winrt::PointerUpdateKind::MiddleButtonReleased:
        return L"MiddleButtonReleased";
    case winrt::PointerUpdateKind::Other:
        return L"Other";
    case winrt::PointerUpdateKind::RightButtonPressed:
        return L"RightButtonPressed";
    case winrt::PointerUpdateKind::RightButtonReleased:
        return L"RightButtonReleased";
    case winrt::PointerUpdateKind::XButton1Pressed:
        return L"XButton1Pressed";
    case winrt::PointerUpdateKind::XButton1Released:
        return L"XButton1Released";
    case winrt::PointerUpdateKind::XButton2Pressed:
        return L"XButton2Pressed";
    case winrt::PointerUpdateKind::XButton2Released:
        return L"XButton2Released";
    default:
        MUX_ASSERT(false);
        return L"";
    }
}

winrt::hstring ScrollEventTypeToString(const winrt::ScrollEventType& scrollEventType)
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

winrt::hstring ScrollingIndicatorModeToString(const winrt::ScrollingIndicatorMode& indicatorMode)
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

winrt::hstring FocusStateToString(const winrt::FocusState& focusState)
{
    switch (focusState)
    {
    case winrt::FocusState::Keyboard:
        return L"Keyboard";
    case winrt::FocusState::Pointer:
        return L"Pointer";
    case winrt::FocusState::Programmatic:
        return L"Programmatic";
    case winrt::FocusState::Unfocused:
        return L"Unfocused";
    default:
        MUX_ASSERT(false);
        return L"";
    }
}

winrt::hstring ItemContainerInteractionTriggerToString(const winrt::ItemContainerInteractionTrigger& interactionTrigger)
{
    switch (interactionTrigger)
    {
    case winrt::ItemContainerInteractionTrigger::PointerPressed:
        return L"PointerPressed";
    case winrt::ItemContainerInteractionTrigger::PointerReleased:
        return L"PointerReleased";
    case winrt::ItemContainerInteractionTrigger::Tap:
        return L"Tap";
    case winrt::ItemContainerInteractionTrigger::DoubleTap:
        return L"DoubleTap";
    case winrt::ItemContainerInteractionTrigger::EnterKey:
        return L"EnterKey";
    case winrt::ItemContainerInteractionTrigger::SpaceKey:
        return L"SpaceKey";
    default:
        MUX_ASSERT(false);
        return L"";
    }
}

winrt::hstring ItemContainerMultiSelectModeToString(const winrt::ItemContainerMultiSelectMode& multiSelectMode)
{
    std::vector<winrt::hstring> modes;
    winrt::hstring str;

    if (static_cast<int>(multiSelectMode & winrt::ItemContainerMultiSelectMode::Auto) != 0)
    {
        modes.push_back(L"Auto");
    }

    if (static_cast<int>(multiSelectMode & winrt::ItemContainerMultiSelectMode::Single) != 0)
    {
        modes.push_back(L"Single");
    }

    if (static_cast<int>(multiSelectMode & winrt::ItemContainerMultiSelectMode::Extended) != 0)
    {
        modes.push_back(L"Extended");
    }

    if (static_cast<int>(multiSelectMode & winrt::ItemContainerMultiSelectMode::Multiple) != 0)
    {
        modes.push_back(L"Multiple");
    }

    if (modes.empty())
    {
        MUX_ASSERT(false);
        return L"";
    }

    str = modes[0];

    // If more than one, concatenate strings with "|".
    for (size_t i = 1; i < modes.size(); i++)
    {
        str = str + L"|" + modes[i];
    }

    return str;
}

winrt::hstring KeyRoutedEventArgsToString(const winrt::KeyRoutedEventArgs& eventArgs)
{
    return StringUtil::FormatString(L"KeyRoutedEventArgs: Handled: %1!u!, Key: %2!u!, OriginalKey: %3!u!",
        static_cast<uint32_t>(eventArgs.Handled()), static_cast<uint32_t>(eventArgs.Key()), static_cast<uint32_t>(eventArgs.OriginalKey()));
}

#pragma endregion

}
