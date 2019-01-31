// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "FormRow.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"

FormRow::FormRow()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_FormRow);
}

winrt::Size FormRow::MeasureOverride(winrt::Size const& availableSize)
{
    // ### shouldn't need this I think
    //const winrt::Thickness itemPadding = winrt::Thickness{ 0, 4, 0, 4 };

    auto children = Children();
    float maxHeight = 0.0;

    for (unsigned i = 0u; i < children.Size(); ++i)
    {
        auto child = children.GetAt(i);
        child.Measure(availableSize);

        float height = child.DesiredSize().Height;
        if (height > maxHeight)
        {
            maxHeight = height;
        }
    }

    // ### for now
    return winrt::Size(500.0f, maxHeight);
}

winrt::Size FormRow::ArrangeOverride(winrt::Size const& finalSize)
{
    //### probably need this sort of thing eventually
    //if (m_isLayoutInProgress)
    //{
    //    throw winrt::hresult_error(E_FAIL, L"Reentrancy detected during layout.");
    //}

    // ### temporary -- these would be properties
    const winrt::Thickness itemPadding = winrt::Thickness{ 0, 4, 0, 4 };

    auto children = Children();

    std::array<float, 100> widths; //### magic max number
    float numStars = 0;

    // calculate required spacing between items
    float requiredWidth = (float)(children.Size() - 1) * 8.0f; //### magic number

    // first pass: all Auto or fixed width elements
    for (unsigned int i = 0; i < children.Size(); i++)
    {
        auto child = children.GetAt(i);
        winrt::GridLength gl = GetLength(child);

        if (gl.GridUnitType == winrt::GridUnitType::Auto)
        {
            widths[i] = child.DesiredSize().Width;
            requiredWidth += widths[i];
        }
        else if (gl.GridUnitType == winrt::GridUnitType::Pixel)
        {
            widths[i] = (float)gl.Value;
            requiredWidth += widths[i];
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
    for (unsigned int i = 0; i < children.Size(); i++)
    {
        auto child = children.GetAt(i);
        winrt::GridLength gl = GetLength(child);

        if (gl.GridUnitType == winrt::GridUnitType::Star)
        {
            widths[i] = starWidth * (float)gl.Value;
        }
    }

    // third pass: call arrange
    float yMax = 0.0f;
    float x = (float)itemPadding.Left;

    for (unsigned int i = 0; i < children.Size(); i++)
    {
        auto child = children.GetAt(i);

        float height = child.DesiredSize().Height;
        float width = widths[i];

        child.Arrange(winrt::Rect(x, 0, width, height));

        x += width + 8.0f; //### same magic number

        if (height > yMax)
        {
            yMax = height;
        }
    }

    // ### for now
    return finalSize;
}

void FormRow::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty property = args.Property();
    
    // TODO: Implement
}

void FormRow::OnLengthPropertyChanged(
    const winrt::DependencyObject& sender,
    const winrt::DependencyPropertyChangedEventArgs& args)
{
    // TODO: Implement
}
