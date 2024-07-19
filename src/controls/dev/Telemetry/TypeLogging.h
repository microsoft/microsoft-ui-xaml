﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace TypeLogging
{
    winrt::hstring KeyRoutedEventArgsToString(const winrt::KeyRoutedEventArgs& eventArgs);
    winrt::hstring PointerPointToString(const winrt::PointerPoint& pointerPoint, bool verbose = false);
    winrt::hstring PointerDeviceTypeToString(const winrt::PointerDeviceType& pointerDeviceKind);
    winrt::hstring PointerUpdateKindToString(const winrt::PointerUpdateKind& pointerUpdateKind);
    winrt::hstring RectToString(const winrt::Rect& rect);
    winrt::hstring SizeToString(const winrt::Size& size);
    winrt::hstring Float2ToString(const winrt::float2& v2);
    winrt::hstring NullableFloatToString(const winrt::IReference<float>& nf);
    winrt::hstring NullableFloat2ToString(const winrt::IReference<winrt::float2>& nv2);
    winrt::hstring OrientationToString(const winrt::Orientation& orientation);
    winrt::hstring ScrollEventTypeToString(const winrt::ScrollEventType& scrollEventType);
    winrt::hstring ScrollingIndicatorModeToString(const winrt::ScrollingIndicatorMode& indicatorMode);    
    winrt::hstring FocusStateToString(const winrt::FocusState& focusState);
    winrt::hstring ItemContainerInteractionTriggerToString(const winrt::ItemContainerInteractionTrigger& interactionTrigger);
    winrt::hstring ItemContainerMultiSelectModeToString(const winrt::ItemContainerMultiSelectMode& multiSelectMode);
}

