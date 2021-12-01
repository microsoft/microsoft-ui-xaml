// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "LayoutPanel.g.h"
#include "LayoutPanel.properties.h"

class LayoutPanel : 
    public ReferenceTracker<LayoutPanel, winrt::implementation::LayoutPanelT>,
    public LayoutPanelProperties
{
public:

    winrt::IInspectable LayoutState() { return m_layoutState.get(); }
    void LayoutState(winrt::IInspectable const& value) { m_layoutState.set(value); }

    void OnPropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args);

    winrt::Size MeasureOverride(winrt::Size const& availableSize);
    winrt::Size ArrangeOverride(winrt::Size const& finalSize);

private:
    winrt::LayoutContext m_layoutContext{ nullptr };

    tracker_ref<winrt::IInspectable> m_layoutState{ this };
    tracker_ref<winrt::Layout, TrackerRefFallback::FallbackToComPtrBeforeRS4> m_layout{ this };

    winrt::Layout::MeasureInvalidated_revoker m_measureInvalidated{};
    winrt::Layout::ArrangeInvalidated_revoker m_arrangeInvalidated{};

    void OnLayoutChanged(winrt::Layout const& oldValue, winrt::Layout const& newValue);

    void InvalidateMeasureForLayout(winrt::Layout const& sender, winrt::IInspectable const& args);
    void InvalidateArrangeForLayout(winrt::Layout const& sender, winrt::IInspectable const& args);
};
