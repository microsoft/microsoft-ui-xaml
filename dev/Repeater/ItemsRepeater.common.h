// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "RepeaterTrace.h"

// Use std::min and std::max instead.
#undef min
#undef max

// We cache these factories for perf reasons
class CachedVisualTreeHelpers : 
    public winrt::implements<CachedVisualTreeHelpers, winrt::IInspectable>
{
public:
    static winrt::Rect GetLayoutSlot(winrt::FrameworkElement const& element);
    static winrt::DependencyObject GetParent(winrt::DependencyObject const& child);

    static winrt::IDataTemplateComponent GetDataTemplateComponent(winrt::UIElement const& element);

private:
    winrt::ILayoutInformationStatics m_layoutInfo;
    winrt::IVisualTreeHelperStatics m_visualTreeHelper;
    winrt::IXamlBindingHelperStatics m_xamlBindingHelperStatics;
};
