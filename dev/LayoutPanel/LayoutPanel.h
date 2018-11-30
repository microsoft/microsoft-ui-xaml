// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "LayoutPanel.g.h"

class LayoutPanel : 
    public ReferenceTracker<LayoutPanel, winrt::implementation::LayoutPanelT>
{
public:

    winrt::Layout Layout();
    void Layout(winrt::Layout const& value);

    winrt::Brush BorderBrush();
    void BorderBrush(winrt::Brush const& value);

    winrt::Thickness BorderThickness();
    void BorderThickness(winrt::Thickness const& value);

    winrt::Thickness Padding();
    void Padding(winrt::Thickness const& value);

    winrt::CornerRadius CornerRadius();
    void CornerRadius(winrt::CornerRadius const& value);

    static winrt::DependencyProperty LayoutProperty() { return s_layoutProperty; }
    static winrt::DependencyProperty BorderBrushProperty() { return s_borderBrushProperty; }
    static winrt::DependencyProperty BorderThicknessProperty() { return s_borderThicknessProperty; }
    static winrt::DependencyProperty CornerRadiusProperty() { return s_cornerRadiusProperty; }
    static winrt::DependencyProperty PaddingProperty() { return s_paddingProperty; }

    static GlobalDependencyProperty s_layoutProperty;
    static GlobalDependencyProperty s_borderBrushProperty;
    static GlobalDependencyProperty s_borderThicknessProperty;
    static GlobalDependencyProperty s_cornerRadiusProperty;
    static GlobalDependencyProperty s_paddingProperty;

    winrt::IInspectable LayoutState() { return m_layoutState.get(); }
    void LayoutState(winrt::IInspectable const& value) { m_layoutState.set(value); }

    static void EnsureProperties();
    static void ClearProperties();

    static void OnPropertyChanged(
        winrt::DependencyObject const& sender,
        winrt::DependencyPropertyChangedEventArgs const& args);

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

CppWinRTActivatableClassWithDPFactory(LayoutPanel);