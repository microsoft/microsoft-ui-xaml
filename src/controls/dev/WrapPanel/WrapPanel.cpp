// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "WrapPanel.h"

#include "WrapPanel.properties.cpp"

void WrapPanel::OnPropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args)
{
    const winrt::IDependencyProperty dependencyProperty = args.Property();

    if (dependencyProperty == s_StretchChildProperty ||
        dependencyProperty == s_PaddingProperty ||
        dependencyProperty == s_HorizontalSpacingProperty ||
        dependencyProperty == s_VerticalSpacingProperty ||
        dependencyProperty == s_OrientationProperty)
    {
        InvalidateMeasure();
        InvalidateArrange();
    }
}


winrt::Size WrapPanel::MeasureOverride(winrt::Size const& availableSize)
{
    const auto padding = Padding();
    const winrt::Size childAvailableSize{
        availableSize.Width - (float)padding.Left - (float)padding.Right,
        availableSize.Height - (float)padding.Top - (float)padding.Bottom
    };

    for (auto const& child : Children())
    {
        child.Measure(childAvailableSize);
    }

    const auto requiredSize = UpdateRows(availableSize);
    return requiredSize;
}

winrt::Size WrapPanel::ArrangeOverride(winrt::Size const& finalSize)
{
    auto orientation = Orientation();
    if ((orientation == winrt::Orientation::Horizontal && finalSize.Width < DesiredSize().Width) ||
        (orientation == winrt::Orientation::Vertical && finalSize.Height < DesiredSize().Height))
    {
        // We haven't received our desired size. We need to refresh the rows.
        UpdateRows(finalSize);
    }

    auto children = Children();
    uint32_t childIndex = 0;
    // Now that we have all the data, we do the actual arrange pass
    for (auto const& row : m_rows)
    {
        for (auto const& uvRect : row.ChildrenRects)
        {
            auto child = children.GetAt(childIndex);
            ++childIndex;
            // Skip any child that is collapsed.
            while (child.Visibility() == winrt::Visibility::Collapsed)
            {
                // Collapsed children are not added into the rows,
                // we skip them.
                child = children.GetAt(childIndex);
                ++childIndex;
            }

            UvRect arrangeRect;
            arrangeRect.Position = uvRect.Position;
            arrangeRect.Size = UvMeasure(uvRect.Size.U, row.Size.V);
            auto finalRect = arrangeRect.ToRect(orientation);
            child.Arrange(finalRect);
        }
    }

    return finalSize;
}

winrt::Size WrapPanel::UpdateRows(winrt::Size availableSize)
{
    m_rows.clear();
    const auto orientation = Orientation();
    const auto padding = Padding();
    UvMeasure paddingStart(orientation, padding.Left, padding.Top);
    UvMeasure paddingEnd(orientation, padding.Right, padding.Bottom);

    auto children = Children();
    if (children.Size() == 0)
    {
        auto emptySize = paddingStart.Add(paddingEnd.U, paddingEnd.V).ToSize(orientation);
        return emptySize;
    }

    UvMeasure parentMeasure(orientation, availableSize.Width, availableSize.Height);
    UvMeasure spacingMeasure(orientation, HorizontalSpacing(), VerticalSpacing());
    UvMeasure position(orientation, padding.Left, padding.Top);

    Row currentRow;
    UvMeasure finalMeasure(0.0, 0.0);

    auto arrange = [&](winrt::UIElement const& child, const bool isLast = false)
        {
            if (child.Visibility() == winrt::Visibility::Collapsed)
            {
                return; // if an item is collapsed, avoid adding the spacing
            }

            UvMeasure desiredMeasure(orientation, child.DesiredSize());
            if ((desiredMeasure.U + position.U + paddingEnd.U) > parentMeasure.U)
            {
                // next row!
                position.U = paddingStart.U;
                position.V += currentRow.Size.V + spacingMeasure.V;
                m_rows.push_back(currentRow);
                currentRow = Row();
            }

            // Stretch the last item to fill the available space
            if (isLast)
            {
                desiredMeasure.U = parentMeasure.U - position.U;
            }

            currentRow.Add(position, desiredMeasure);

            position.U += desiredMeasure.U + spacingMeasure.U;
            finalMeasure.U = std::max(finalMeasure.U, position.U);
        };

    if (children.Size() > 0)
    {
        uint32_t lastIndex = children.Size() - 1;
        
        // Arrange all children except the last one
        for (uint32_t i = 0; i < lastIndex; i++)
        {
            arrange(children.GetAt(i));
        }
        
        // Arrange the last child (which may be the only child)
        arrange(children.GetAt(lastIndex), StretchChild() == winrt::StretchChild::Last);
    }

    if (!currentRow.ChildrenRects.empty())
    {
        m_rows.push_back(currentRow);
    }

    if (m_rows.empty())
    {
        auto emptySize = paddingStart.Add(paddingEnd.U, paddingEnd.V).ToSize(orientation);
        return emptySize;
    }

    // Get max V here before computing final rect
    auto lastRowRect = m_rows.back().Rect();
    finalMeasure.V = lastRowRect.Position.V + lastRowRect.Size.V;
    auto finalRect = finalMeasure.Add(paddingEnd.U, paddingEnd.V).ToSize(orientation);
    return finalRect;
}
