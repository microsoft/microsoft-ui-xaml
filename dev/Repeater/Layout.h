// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Layout.g.h"
#include "NonVirtualizingLayout.g.h"

class Layout :
    public winrt::implementation::LayoutT<Layout, winrt::composable>
{
public:
#pragma region ILayout
    // For debugging purposes only.
    winrt::hstring LayoutId();
    void LayoutId(winrt::hstring const& state);

    void InitializeForContext(winrt::LayoutContext const& context);
    void UninitializeForContext(winrt::LayoutContext const& context);
    
    winrt::Size Measure(winrt::LayoutContext const& context, winrt::Size const& availableSize);
    winrt::Size Arrange(winrt::LayoutContext const& context, winrt::Size const& finalSize);

    winrt::event_token MeasureInvalidated(winrt::TypedEventHandler<winrt::Layout, winrt::IInspectable> const& value);
    void MeasureInvalidated(winrt::event_token const& token);

    winrt::event_token ArrangeInvalidated(winrt::TypedEventHandler<winrt::Layout, winrt::IInspectable> const& value);
    void ArrangeInvalidated(winrt::event_token const& token);
#pragma endregion

#pragma region ILayoutProtected
     void InvalidateMeasure();
     void InvalidateArrange();
#pragma endregion


private:
     event<winrt::TypedEventHandler<winrt::Layout, winrt::IInspectable>> m_measureInvalidatedEventSource{ };
     event<winrt::TypedEventHandler<winrt::Layout, winrt::IInspectable>> m_arrangeInvalidatedEventSource { };

     // TODO: This is for debugging purposes only. It should be removed when 
     // the Layout.LayoutId API is removed.
     winrt::hstring m_layoutId;
};