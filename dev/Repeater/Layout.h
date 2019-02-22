// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Layout.g.h"
#include "Layout.properties.h"
#include "NonVirtualizingLayout.g.h"

class Layout :
    public winrt::implementation::LayoutT<Layout, winrt::composable>,
    public LayoutProperties
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
#pragma endregion

#pragma region ILayoutProtected
     void InvalidateMeasure();
     void InvalidateArrange();
#pragma endregion


private:
     // TODO: This is for debugging purposes only. It should be removed when 
     // the Layout.LayoutId API is removed.
     winrt::hstring m_layoutId;
};
