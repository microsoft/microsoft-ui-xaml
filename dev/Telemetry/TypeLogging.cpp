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

winrt::hstring PointToString(const winrt::Point& point)
{
    return StringUtil::FormatString(L"Point: X: %1!i!, Y: %2!i!",
        static_cast<int32_t>(point.X), static_cast<int32_t>(point.Y));
}

winrt::hstring RectToString(const winrt::Rect& rect)
{
    return StringUtil::FormatString(L"Rect: X: %1!i!, Y: %2!i!, W: %3!u!, H: %4!u!",
        static_cast<int32_t>(rect.X), static_cast<int32_t>(rect.Y), static_cast<uint32_t>(rect.Width), static_cast<uint32_t>(rect.Height));
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

winrt::hstring KeyRoutedEventArgsToString(const winrt::KeyRoutedEventArgs& eventArgs)
{
    return StringUtil::FormatString(L"KeyRoutedEventArgs: Handled: %1!u!, Key: %2!u!, OriginalKey: %3!u!",
        static_cast<uint32_t>(eventArgs.Handled()), static_cast<uint32_t>(eventArgs.Key()), static_cast<uint32_t>(eventArgs.OriginalKey()));
}

#pragma endregion

}
