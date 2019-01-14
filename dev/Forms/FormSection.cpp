// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "FormSection.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"
#include "Vector.h"
#include "VectorIterator.h"

FormSection::FormSection()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_Forms);

}

winrt::Size FormSection::MeasureOverride(winrt::Size const& availableSize)
{
    // ### temporary -- these would be properties
    const winrt::Thickness itemPadding = winrt::Thickness{ 0, 4, 0, 4 };

    auto children = Children();
    float height = 0.0;

    for (unsigned i = 0u; i < children.Size(); ++i)
    {
        auto child = children.GetAt(i);
        child.Measure(availableSize);

        // ### this is obviously wrong because it's not taking buddies into account but who cares for now...
        height += child.DesiredSize().Height + (float)itemPadding.Top + (float)itemPadding.Bottom;
    }

    // ### for now
    return winrt::Size(500.0f, height);
}

winrt::Size FormSection::ArrangeOverride(winrt::Size const& finalSize)
{
    //### probably need this sort of thing eventually
    //if (m_isLayoutInProgress)
    //{
    //    throw winrt::hresult_error(E_FAIL, L"Reentrancy detected during layout.");
    //}

    // ### temporary -- these would be properties
    const winrt::Thickness itemPadding = winrt::Thickness{ 0, 4, 0, 4 };

    auto children = Children();
    float y = 0.0;

    for (unsigned i = 0u; i < children.Size(); ++i)
    {
        auto child = children.GetAt(i);
        int numBuddies = GetBuddies(child);

        if (numBuddies <= 0)
        {
            float width = child.DesiredSize().Width;
            float height = child.DesiredSize().Height;

            winrt::GridLength gl = GetLength(child);
            if (gl.GridUnitType == winrt::GridUnitType::Star)
            {
                // ### that's not how this works
                width = 500.0f;
            }
            // ### obviously need to handle all cases

            child.Arrange(winrt::Rect(0.0f + (float)itemPadding.Left, y + (float)itemPadding.Top, width, height));

            y += height + (float)itemPadding.Top + (float)itemPadding.Bottom;
        }
        else
        {
            std::array<float, 100> widths; //### magic max number
            float numStars = 0;

            // calculate required spacing between items
            float requiredWidth = (float)numBuddies * 8.0f; //### magic number

            // first pass: all Auto or fixed width elements
            for (int n = 0; n <= numBuddies; n++)
            {
                auto child = children.GetAt(i + n);
                winrt::GridLength gl = GetLength(child);

                if (gl.GridUnitType == winrt::GridUnitType::Auto)
                {
                    widths[n] = child.DesiredSize().Width;
                    requiredWidth += widths[n];
                }
                else if (gl.GridUnitType == winrt::GridUnitType::Pixel)
                {
                    widths[n] = (float)gl.Value;
                }
                else if (gl.GridUnitType == winrt::GridUnitType::Star)
                {
                    numStars += (float)gl.Value;
                }
            }

            // ### magic 500px
            float remainingWidth = 500.0f - requiredWidth;
            float starWidth = remainingWidth / numStars;

            // second pass: * elements
            for (int n = 0; n <= numBuddies; n++)
            {
                auto child = children.GetAt(i + n);
                winrt::GridLength gl = GetLength(child);

                if (gl.GridUnitType == winrt::GridUnitType::Star)
                {
                    widths[n] = starWidth * (float)gl.Value;
                }
            }

            // third pass: call arrange
            float yMax = 0.0f;
            float x = (float)itemPadding.Left;

            for (int n = 0; n <= numBuddies; n++)
            {
                auto child = children.GetAt(i + n);

                float height = child.DesiredSize().Height;
                float width = widths[n];

                child.Arrange(winrt::Rect(x, y + (float)itemPadding.Top, width, height));

                x += width + 8.0f; //### same magic number

                if (height > yMax)
                {
                    yMax = height;
                }
            }

            y += yMax + (float)itemPadding.Top + (float)itemPadding.Bottom;
            i += numBuddies;
        }
    }

    // ### for now
    return finalSize;
}

void FormSection::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty property = args.Property();
    
    // TODO: Implement
}

void FormSection::OnLengthPropertyChanged(
    const winrt::DependencyObject& sender,
    const winrt::DependencyPropertyChangedEventArgs& args)
{
    // TODO: Implement
}

void FormSection::OnBuddiesPropertyChanged(
    const winrt::DependencyObject& sender,
    const winrt::DependencyPropertyChangedEventArgs& args)
{
    // TODO: Implement
}
