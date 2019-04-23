// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "NonVirtualizingLayout.h"
#include "FlexboxLayout.g.h"
#include "FlexboxLayout.properties.h"

class FlexboxLayout :
    public ReferenceTracker<FlexboxLayout, winrt::implementation::FlexboxLayoutT, NonVirtualizingLayout>,
    public FlexboxLayoutProperties
{
public:
    FlexboxLayout();

    winrt::FlexboxWrap Wrap();
    void Wrap(winrt::FlexboxWrap const& value);

    winrt::FlexboxDirection Direction();
    void Direction(winrt::FlexboxDirection const& value);

    winrt::FlexboxJustifyContent JustifyContent();
    void JustifyContent(winrt::FlexboxJustifyContent const& value);

    winrt::FlexboxAlignItems AlignItems();
    void AlignItems(winrt::FlexboxAlignItems const& value);

    winrt::FlexboxAlignContent AlignContent();
    void AlignContent(winrt::FlexboxAlignContent const& value);

#pragma region INonVirtualizingLayoutOverrides
    void InitializeForContextCore(winrt::LayoutContext const& context);
    void UninitializeForContextCore(winrt::LayoutContext const& context);

    winrt::Size MeasureOverride(winrt::LayoutContext const& context, winrt::Size const& availableSize);
    winrt::Size ArrangeOverride(winrt::LayoutContext const& context, winrt::Size const& finalSize);
#pragma endregion

    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

private:
    bool IsHorizontal();
    bool IsReversed();
    bool IsWrapping();
    float MainAxis(winrt::Size const& value);
    float CrossAxis(winrt::Size const& value);
    winrt::Size CreateSize(float mainAxis, float crossAxis);
    winrt::Point CreatePoint(float mainAxis, float crossAxis);
    std::vector<winrt::UIElement> ChildrenSortedByOrder(winrt::NonVirtualizingLayoutContext const& context);

    winrt::FlexboxWrap m_wrap;
    winrt::FlexboxDirection m_direction;
    winrt::FlexboxJustifyContent m_justifyContent;
    winrt::FlexboxAlignItems m_alignItems;
    winrt::FlexboxAlignContent m_alignContent;
};
