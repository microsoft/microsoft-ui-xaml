// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "OffsetsChange.h"

class BringIntoViewOffsetsChange : public OffsetsChange
{
public:
    BringIntoViewOffsetsChange(
        const ITrackerHandleManager* owner,
        double zoomedHorizontalOffset,
        double zoomedVerticalOffset,
        ScrollPresenterViewKind offsetsKind,
        winrt::IInspectable const& options,
        winrt::UIElement const& element,
        winrt::Rect const& elementRect,
        double horizontalAlignmentRatio,
        double verticalAlignmentRatio,
        double horizontalOffset,
        double verticalOffset);
    ~BringIntoViewOffsetsChange();

    winrt::UIElement Element() const
    {
        return m_element.get();
    }

    winrt::Rect ElementRect() const
    {
        return m_elementRect;
    }

    double HorizontalAlignmentRatio() const
    {
        return m_horizontalAlignmentRatio;
    }

    double VerticalAlignmentRatio() const
    {
        return m_verticalAlignmentRatio;
    }

    double HorizontalOffset() const
    {
        return m_horizontalOffset;
    }

    double VerticalOffset() const
    {
        return m_verticalOffset;
    }

private:
    const ITrackerHandleManager* m_owner;
    tracker_ref<winrt::UIElement> m_element{ m_owner };
    winrt::Rect m_elementRect;
    double m_horizontalAlignmentRatio;
    double m_verticalAlignmentRatio;
    double m_horizontalOffset;
    double m_verticalOffset;
};

