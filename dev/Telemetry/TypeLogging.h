// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace TypeLogging
{
    static winrt::hstring KeyRoutedEventArgsToString(const winrt::KeyRoutedEventArgs& eventArgs);
    static winrt::hstring PointerPointToString(const winrt::PointerPoint& pointerPoint, bool verbose = false);
    static winrt::hstring RectToString(const winrt::Rect& rect);
    static winrt::hstring Float2ToString(const winrt::float2& v2);
    static winrt::hstring NullableFloatToString(const winrt::IReference<float>& nf);
    static winrt::hstring NullableFloat2ToString(const winrt::IReference<winrt::float2>& nv2);
    static winrt::hstring OrientationToString(const winrt::Orientation& orientation);
    static winrt::hstring ScrollEventTypeToString(const winrt::ScrollEventType& scrollEventType);
    static winrt::hstring ScrollingIndicatorModeToString(const winrt::ScrollingIndicatorMode& indicatorMode);
}

